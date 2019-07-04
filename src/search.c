#include <assert.h>
#include <time.h>

#include "board.h"
#include "play.h"
#include "eval.h"
#include "search.h"
#include "hashtables.h"


Position alphabeta(Board board, const int depth, int alpha, const int beta);


// 8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -
int count = 0;

Move search(Board board, Settings settings) {
	Position best;
	Move move;

	struct timespec start, end;

	clock_gettime(CLOCK_MONOTONIC_RAW, &start);

	best = alphabeta(board, settings.depth, -MAX_SCORE - 1, MAX_SCORE + 1);
	move = decompressMove(board, best.move);

	clock_gettime(CLOCK_MONOTONIC_RAW, &end);

	printf("Time: %" PRIu64 "\n", (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000);
	printf("Score: %d\n", best.score);

	return move;
}

/*
 * A basic implementation of alpha-beta pruning
 * Checkmate in n moves is found on depth n+1
 * alpha is the value to maximize and beta the value to minimize.
 * The initial position must have >= 1 legal moves and
 * the initial depth must be >= 1.
 */
Position alphabeta(Board board, const int depth, int alpha, const int beta) {
	Move moves[218];
	History history;
	Position node, best;

	int k, index;

	if (depth == 0) {
		best.score = eval(board);
	} else {
		k = legalMoves(board, moves);

		if (k == 0) {
			best.score = finalEval(board, depth);
		} else {
			best.score = -2 * MAX_SCORE;

			for (int i = 0; i < k; i++) {
				makeMove(&board, moves[i], &history);
				updateBoardKey(&board, moves[i], history);

				index = board.key % HASHTABLE_MAX_SIZE;

				if (tt[index].key != board.key || tt[index].depth < depth) {
					node = alphabeta(board, depth - 1, -beta, -alpha);

					// Saves the node on the transposition table
					tt[index] = compressPosition(board.key, moves[i], -node.score, depth, 0);
				}

				// Updates the best value and pruns the tree when alpha >= beta
				if (tt[index].score > best.score) {
					best = tt[index];

					if (best.score >= beta) break;

					if (best.score > alpha)
						alpha = best.score;
				}

				updateBoardKey(&board, moves[i], history);
				undoMove(&board, moves[i], history);
			}
		}
	}

	return best;
}
