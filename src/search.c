#include <assert.h>
#include <time.h>
#include <string.h>

#include "board.h"
#include "uci.h"
#include "moves.h"
#include "play.h"
#include "draw.h"
#include "eval.h"
#include "search.h"
#include "hashtables.h"


static int alphabeta(Board *board, const int depth, int alpha, int beta);
static PV generatePV(Board board);

static void timeManagement(const Board *board);

static void evaluateMoves(const Board *board, Move *moves, const int nMoves, const int depth);

static int moveScore(const int victim, const int attacker);
static int compareMovesScore(const void *moveA, const void *moveB);

clock_t start;
uint64_t nodes;

Move search(Board *board) {
	Move bestMove;

	timeManagement(board);
	start = clock();

	const int index = board->key % HASHTABLE_MAX_SIZE;

	for (int depth = 1; depth <= settings.depth; ++depth) {
		nodes = 0;

		const int score = alphabeta(board, depth, -2 * MAX_SCORE, 2 * MAX_SCORE);

		if (settings.stop) break;

		bestMove = decompressMove(board, &tt[index].move);

		PV pv = generatePV(*board);
		bestMove = pv.moves[0];

		infoString(board, &pv, score, depth, nodes);
	}

	return bestMove;
}

/*
 * A basic implementation of alpha-beta pruning
 * Checkmate in n moves is found at depth n+1
 * alpha is the value to maximize and beta the value to minimize.
 * The initial position must have >= 1 legal moves and the initial depth must be >= 1.
 */
static int alphabeta(Board *board, const int depth, int alpha, int beta) {
	if (settings.stop)
		return 0;

	if (settings.movetime && clock() - start > settings.movetime) {
		settings.stop = 1;
		return 0;
	}

	++nodes;

	if (depth == 0)
		return eval(board);

	const int index = board->key % HASHTABLE_MAX_SIZE;

	// Transposition Table
	if (tt[index].key == board->key && tt[index].depth >= depth) {
		switch (tt[index].flag) {
		case LOWER_BOUND:
			alpha = (alpha > tt[index].score) ? alpha : tt[index].score;
			break;
		case UPPER_BOUND:
			beta = (beta < tt[index].score) ? beta : tt[index].score;
			break;
		case EXACT:
			return tt[index].score;
		}

		if (alpha >= beta)
			return tt[index].score;
	}

	History history;

	/* Null Move Pruning
	if (depth > 2 && !inCheck(board)) {
		makeNullMove(board, &history);

		const int score = -alphabeta(board, depth-1 - R, -beta, -alpha);

		undoNullMove(board, &history);

		if (score >= beta)
			return score;
	}
	*/

	Move moves[218];
	const int nMoves = legalMoves(board, moves);

	// It's a leaf node, there are no legal moves
	if (nMoves == 0)
		return finalEval(board, depth);

	// Move ordering
	evaluateMoves(board, moves, nMoves, depth);
	qsort(moves, nMoves, sizeof(Move), compareMovesScore);

	Move bestMove = moves[0];
	int bestScore = -2 * MAX_SCORE;

	const int originalAlpha = alpha, originalBeta = beta;

	for (int i = 0; i < nMoves; ++i) {
		makeMove(board, &moves[i], &history);
		updateBoardKey(board, &moves[i], &history);
		saveKeyToMemory(board->key);

		int score;

		if (isDraw(board)) {
			score = 0;
		} else {
			score = -alphabeta(board, depth-1, -beta, -alpha);
		}

		freeKeyFromMemory();
		updateBoardKey(board, &moves[i], &history);
		undoMove(board, &moves[i], &history);

		// Updates the best value and pruns the tree when alpha >= beta
		if (score > bestScore) {
			bestScore = score;
			bestMove = moves[i];

			if (bestScore > alpha) {
				alpha = bestScore;

				if (alpha >= beta)
					break;
			}
		}
	}

	int flag = EXACT;

	if (bestScore <= originalAlpha) {
		flag = UPPER_BOUND;
	} else if (bestScore >= originalBeta) {
		flag = LOWER_BOUND;
	}

	tt[index] = compressEntry(board->key, &bestMove, bestScore, depth, flag);

	return bestScore;
}

static void timeManagement(const Board *board) {
	if (settings.movetime == 0) {
		settings.movetime = (board->turn == WHITE) ? settings.wtime : settings.btime;
	}

	// This is done so it can be compared against clocks
	settings.movetime *= CLOCKS_PER_SEC / 1000;
	settings.movetime /= 50;
}

/*
 * Assigns each move a score via the MVV-LVA technique.
 */
static void evaluateMoves(const Board *board, Move *moves, const int nMoves, const int depth) {
	const int opColor = 1 ^ board->turn;

	for (int i = 0; i < nMoves; ++i) {
		const uint64_t toBB = pow2[moves[i].to];

		if (moves[i].score == 0 && (toBB & board->players[opColor])) {
			for (int piece = PAWN; piece <= QUEEN; ++piece) {
				if (toBB & board->pieces[opColor][piece]) {
					moves[i].score = moveScore(piece, moves[i].piece);
					break;
				}
			}
		}
	}
}

static int moveScore(const int victim, const int attacker) {
	static const int pieceValues[6] = {100,200,200,300,400,500};

	return pieceValues[victim] - pieceValues[attacker] / 10;
}

/*
 *	Generates the PV line out of the moves from the TT.
 */
static PV generatePV(Board board) {
	PV pv = (PV) {.count = 0};

	while (1) {
		const int index = board.key % HASHTABLE_MAX_SIZE;

		if (board.key != tt[index].key)
			break;

		const Move move = decompressMove(&board, &tt[index].move);

		if (!isLegalMove(&board, &move))
			break;

		// Avoids getting into an infinite loop
		for (int i = 0; i < pv.count; ++i) {
			if (compareMoves(&pv.moves[i], &move))
				return pv;
		}

		pv.moves[pv.count] = move;
		++pv.count;

		History history;
		makeMove(&board, &move, &history);
		updateBoardKey(&board, &move, &history);
	}

	return pv;
}


// AUX

static int compareMovesScore(const void *moveA, const void *moveB) {
	return ((Move *) moveB)->score - ((Move *) moveA)->score;
}
