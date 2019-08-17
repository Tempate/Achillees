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


static PV generatePV(Board board);

static const int qsearch(Board *board, int alpha, const int beta);

static void timeManagement(const Board *board);

static void moveOrdering(const Board *board, Move *moves, const int nMoves);
static void insertionSort(Move *list, const int n);

clock_t start;
uint64_t nodes;

Move search(Board *board) {
	Move bestMove;

	timeManagement(board);

	start = clock();

	const int index = board->key % HASHTABLE_MAX_SIZE;

	for (int depth = 1; depth <= settings.depth; ++depth) {
		nodes = 0;

		const int score = alphabeta(board, depth, -2 * MAX_SCORE, 2 * MAX_SCORE, 0);

		if (settings.stop) break;

		bestMove = decompressMove(board, &tt[index].move);

		PV pv = generatePV(*board);

		const long duration = 1000 * (clock() - start) / CLOCKS_PER_SEC;
		infoString(board, &pv, score, depth, duration, nodes);
	}

	assert(bestMove.to != bestMove.from);

	return bestMove;
}

/*
 * A basic implementation of alpha-beta pruning
 * Checkmate in n moves is found at depth n+1
 * alpha is the value to maximize and beta the value to minimize.
 * The initial position must have >= 1 legal moves and the initial depth must be >= 1.
 */
const int alphabeta(Board *board, const int depth, int alpha, int beta, const int nullmove) {
	assert(beta >= alpha);
	assert(depth >= 0);

	if (settings.stop)
		return 0;

	if (settings.movetime && clock() - start > settings.movetime) {
		settings.stop = 1;
		return 0;
	}

	++nodes;

	if (depth == 0)
		return qsearch(board, alpha, beta);

	const int index = board->key % HASHTABLE_MAX_SIZE;

	// Transposition Table
	if (tt[index].key == board->key && tt[index].depth == depth) {
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

	// Null Move Pruning
	if (!nullmove && depth > R && !inCheck(board) && !isEndgame(board)) {
		makeNullMove(board, &history);

		const int score = -alphabeta(board, depth - 1 - R, -beta, -beta + 1, 1);

		undoNullMove(board, &history);

		if (score >= beta)
			return beta;
	}

	Move moves[MAX_MOVES];
	const int nMoves = legalMoves(board, moves);

	// It's a leaf node, there are no legal moves
	if (nMoves == 0)
		return finalEval(board, depth);

	moveOrdering(board, moves, nMoves);

	Move bestMove = moves[0];
	int bestScore = -2 * MAX_SCORE;

	const int origAlpha = alpha, origBeta = beta;

	for (int i = 0; i < nMoves; ++i) {
		makeMove(board, &moves[i], &history);
		updateBoardKey(board, &moves[i], &history);
		saveKeyToMemory(board->key);

		int score;

		if (isDraw(board)) {
			score = 0;
		} else {
			score = -alphabeta(board, depth-1, -beta, -alpha, nullmove);
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

	if (bestScore <= origAlpha) {
		flag = UPPER_BOUND;
	} else if (bestScore >= origBeta) {
		flag = LOWER_BOUND;
	}

	tt[index] = compressEntry(board->key, &bestMove, bestScore, depth, flag);

	return bestScore;
}

static const int qsearch(Board *board, int alpha, const int beta) {
	const int score = eval(board);

	if (score >= beta)
		return beta;

	if (score > alpha)
		alpha = score;

	Move moves[MAX_MOVES];
	const int nMoves = legalMoves(board, moves);

	moveOrdering(board, moves, nMoves);

	for (int i = 0; i < nMoves; ++i) {
		if (!moves[i].capture)
			continue;

		History history;

		makeMove(board, &moves[i], &history);
		const int score = -qsearch(board, -beta, -alpha);
		undoMove(board, &moves[i], &history);

		if (score >= beta)
			return beta;

		if (score > alpha)
			alpha = score;
	}

	return alpha;
}

static void timeManagement(const Board *board) {
	if (settings.movetime == 0) {
		settings.movetime = (board->turn == WHITE) ? settings.wtime : settings.btime;
		settings.movetime *= CLOCKS_PER_SEC / 50000;
	} else {
		settings.movetime *= CLOCKS_PER_SEC / 1000;
	}
}

/*
 * 1. Best move from the previous PV
 * 2. MVV-LVA for captures
 */
static void moveOrdering(const Board *board, Move *moves, const int nMoves) {
	static const int pieceValues[6] = {100,200,200,300,400,500};
	const int opColor = 1 ^ board->turn;

	const int index = board->key % HASHTABLE_MAX_SIZE;
	Move pvMove = decompressMove(board, &tt[index].move);

	for (int i = 0; i < nMoves; ++i) {
		if (board->key == tt[index].key && compareMoves(&pvMove, &moves[i])) {
			moves[i].score = 1000;
		} else if (moves[i].capture) {
			const uint64_t toBB = square[moves[i].to];

			for (int piece = PAWN; piece <= QUEEN; ++piece) {
				if (toBB & board->pieces[opColor][piece]) {
					moves[i].score = pieceValues[piece] - pieceValues[moves[i].piece] / 10;
					break;
				}
			}
		}
	}

	insertionSort(moves, nMoves);
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

		assert(move.to != move.from);

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

/*
 * Orders a list of moves by their score using insertion sort
 */
static void insertionSort(Move *list, const int n) {
	for (int i = 1; i < n; ++i) {
		const Move move = list[i];
		int j = i - 1;

		while (j >= 0 && list[j].score < move.score) {
			list[j+1] = list[j];
			--j;
		}

		list[j+1] = move;
	}
}
