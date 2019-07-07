#include <assert.h>
#include <time.h>

#include "board.h"
#include "play.h"
#include "eval.h"
#include "search.h"
#include "hashtables.h"
#include "uci.h"
#include "draw.h"


static int alphabeta(Board *board, const int depth, int alpha, const int beta);

static void timeManagement(const Board *board);

static void evaluateMoves(const Board *board, Move *moves, const int nMoves, const int depth);
static void updatePV(Board board, Move *pv, const int depth);

static int moveScore(const int victim);
static int compareMoves(const void *moveA, const void *moveB);

clock_t start;
uint64_t nodes;

Move pv[MAX_DEPTH];

Move search(Board *board) {
	Move bestMove;
	char moveText[6];

	timeManagement(board);
	start = clock();

	nodes = 0;

	for (int depth = 1; depth <= settings.depth; ++depth) {
		const int score = alphabeta(board, depth, -2 * MAX_SCORE, 2 * MAX_SCORE);

		if (settings.stop) break;

		updatePV(*board, &pv[0], depth);
		bestMove = pv[depth - 1];

		moveToText(bestMove, moveText);

		fprintf(stdout, "info score cp %d depth %d pv %s nodes %ld\n", score, depth, moveText, nodes);
		fflush(stdout);

		nodes = 0;
	}

	return bestMove;
}

/*
 * A basic implementation of alpha-beta pruning
 * Checkmate in n moves is found at depth n+1
 * alpha is the value to maximize and beta the value to minimize.
 * The initial position must have >= 1 legal moves and the initial depth must be >= 1.
 */
static int alphabeta(Board *board, const int depth, int alpha, const int beta) {
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

	// Uses the entry on the TT if one is found
	if (tt[index].key == board->key && tt[index].depth >= depth) {
		switch (tt[index].type) {
		case LOWER_BOUND:
			if (tt[index].score >= beta) {
				return tt[index].score;
			}
			break;
		case HIGHER_BOUND:
			if (tt[index].score <= alpha) {
				return tt[index].score;
			}
			break;
		case EXACT:
			return tt[index].score;
		}
	}

	Move moves[218];
	const int nMoves = legalMoves(board, moves);

	// It's a leaf node, there are no legal moves
	if (nMoves == 0)
		return finalEval(board, depth);

	evaluateMoves(board, moves, nMoves, depth);
	qsort(moves, nMoves, sizeof(Move), compareMoves);

	Move bestMove = moves[0];
	int bestScore = -2 * MAX_SCORE;
	int type = LOWER_BOUND;

	for (int i = 0; i < nMoves; ++i) {
		History history;

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
				type = EXACT;

				if (alpha >= beta) {
					type = HIGHER_BOUND;
					break;
				}
			}
		}
	}

	tt[index] = compressPosition(board->key, &bestMove, bestScore, depth, type);

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
 * Assigns a score to each move:
 * 	 1. Best move from the principal variation
 * 	 2. Most valuable victim
 */
static void evaluateMoves(const Board *board, Move *moves, const int nMoves, const int depth) {
	const int opColor = 1 ^ board->turn;
	const Move pvMove = pv[depth - 2];

	for (int i = 0; i < nMoves; ++i) {
		const uint64_t toBB = pow2[moves[i].to];

		if (depth >= 2 && moves[i].from == pvMove.from && moves[i].to == pvMove.to && moves[i].promotion == pvMove.promotion) {
			moves[i].score = 30000 - depth;
		} else if (moves[i].score == 0 && (toBB & board->players[opColor])) {
			// Most valuable victim
			for (int piece = PAWN; piece <= QUEEN; ++piece) {
				if (toBB & board->pieces[opColor][piece]) {
					moves[i].score = moveScore(piece);
					break;
				}
			}
		}
	}
}

static void updatePV(Board board, Move *pv, const int depth) {
	for (int i = depth - 1; i >= 0; --i) {
		History history;

		const int index = board.key % HASHTABLE_MAX_SIZE;
		Move move = decompressMove(&board, &tt[index].move);

		pv[i] = move;

		makeMove(&board, &move, &history);
		updateBoardKey(&board, &move, &history);
	}
}

/*
 * MVV-LVA would need to take into account the value of the attacker.
 * However, this is already done in the move generator, so it can be omitted here.
 */
static int moveScore(const int victim) {
	static const int pieceValues[6] = {100,200,200,300,400};

	assert(victim != KING);

	return pieceValues[victim];
}

static int compareMoves(const void *moveA, const void *moveB) {
	return ((Move *) moveB)->score - ((Move *) moveA)->score;
}
