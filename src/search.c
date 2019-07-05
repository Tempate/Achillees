#include <assert.h>
#include <time.h>

#include "board.h"
#include "play.h"
#include "eval.h"
#include "search.h"
#include "hashtables.h"
#include "uci.h"


static int alphabeta(Board board, const int depth, int alpha, const int beta);
static time_t timeManagement(Board board);

time_t endTime;
clock_t start;

Move search(Board board) {
	endTime = timeManagement(board);
	start = clock();

	for (int depth = 1; depth <= settings.depth && !settings.stop; ++depth) {
		alphabeta(board, depth, -2 * MAX_SCORE, 2 * MAX_SCORE);
	}

	const int index = board.key % HASHTABLE_MAX_SIZE;
	Move move = decompressMove(board, tt[index].move);

	return move;
}

/*
 * A basic implementation of alpha-beta pruning
 * Checkmate in n moves is found at depth n+1
 * alpha is the value to maximize and beta the value to minimize.
 * The initial position must have >= 1 legal moves and the initial depth must be >= 1.
 */
static int alphabeta(Board board, const int depth, int alpha, const int beta) {
	if (clock() - start > endTime)
		settings.stop = 1;

	if (settings.stop)
		return 0;

	if (depth == 0)
		return eval(board);

	const int index = board.key % HASHTABLE_MAX_SIZE;

	// Uses the entry on the TT if one is found
	if (tt[index].key == board.key && tt[index].depth >= depth)
		return tt[index].score;

	Move moves[218];
	const int nMoves = legalMoves(board, moves);

	// It's a leaf node, there are no legal moves
	if (nMoves == 0)
		return finalEval(board, depth);

	int bestScore = -2 * MAX_SCORE;
	Move bestMove;

	for (int i = 0; i < nMoves; ++i) {
		History history;

		makeMove(&board, moves[i], &history);
		updateBoardKey(&board, moves[i], history);

		int score = -alphabeta(board, depth-1, -beta, -alpha);

		updateBoardKey(&board, moves[i], history);
		undoMove(&board, moves[i], history);

		// Updates the best value and pruns the tree when alpha >= beta
		if (score > bestScore) {
			bestScore = score;
			bestMove = moves[i];

			if (bestScore > alpha)
				alpha = bestScore;

			if (alpha >= beta) break;
		}
	}

	if (settings.stop == 0) {
		tt[index] = compressPosition(board.key, bestMove, bestScore, depth, 0);
	}

	return bestScore;
}

static time_t timeManagement(Board board) {
	clock_t endTime = (board.turn == WHITE) ? settings.wtime : settings.btime;

	/*
	if (endTime == 0) {
		endTime = 0xffffffff;
	}
	*/

	// This is done so it can be compared against clocks
	endTime *= CLOCKS_PER_SEC / 1000;

	endTime /= 50;

	return endTime;
}
