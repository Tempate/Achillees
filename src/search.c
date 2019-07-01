#include "board.h"
#include "play.h"
#include "eval.h"
#include "search.h"


typedef struct {
	Move move;
	int score;
} Node;

Node alphabeta(Board board, const int depth, int alpha, const int beta);


Move search(Board board, Settings settings) {
	Node best;

	best = alphabeta(board, settings.depth, -MAX_SCORE - 1, MAX_SCORE + 1);

	return best.move;
}

/*
 * A basic implementation of alpha-beta pruning
 * Checkmate in n moves is found on depth n+1
 * alpha is the value to maximize and beta the value to minimize
 */
Node alphabeta(Board board, const int depth, int alpha, const int beta) {
	Move moves[218];
	History history;

	Node node, best;

	int k;

	if (depth == 0) {
		best.score = eval(board);
	} else {
		k = legalMoves(board, moves);

		if (k == 0) {
			best.score = finalEval(board, depth);
		} else {
			best.score = -MAX_SCORE - 1;

			for (int i = 0; i < k; i++) {
				makeMove(&board, moves[i], &history);

				node = alphabeta(board, depth - 1, -beta, -alpha);

				// Updates the best value and pruns the tree when alpha >= beta
				if (-node.score > best.score) {
					best.score = -node.score;
					best.move = moves[i];

					if (best.score >= beta) break;

					if (best.score > alpha)
						alpha = best.score;
				}

				undoMove(&board, moves[i], history);
			}
		}
	}

	return best;
}
