#include "board.h"
#include "play.h"
#include "eval.h"
#include "search.h"


typedef struct {
	Move move;
	int score;
} Node;

Node negamax(Board board, const int depth);


Move search(Board board, Settings settings) {
	Node best = negamax(board, settings.depth);

	return best.move;
}


Node negamax(Board board, const int depth) {
	Move moves[218];
	History history;

	Node node, best;

	int k, newDepth;

	if (depth == 0) {
		best.score = eval(board);
	} else {
		k = legalMoves(board, moves);
		newDepth = depth - 1;

		if (k == 0) {
			best.score = finalEval(board, depth);
		} else {
			best.score = -10001;

			for (int i = 0; i < k; i++) {
				makeMove(&board, moves[i], &history);
				node = negamax(board, newDepth);
				undoMove(&board, moves[i], history);

				if (-node.score > best.score) {
					best.score = -node.score;
					best.move = moves[i];
				}
			}
		}
	}

	return best;
}
