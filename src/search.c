#include <assert.h>
#include <time.h>
#include <string.h>

#include "board.h"
#include "uci.h"
#include "moves.h"
#include "play.h"
#include "draw.h"
#include "eval.h"
#include "sort.h"
#include "search.h"
#include "hashtables.h"


static const int qsearch(Board *board, int alpha, const int beta);

static void timeManagement(const Board *board);

static PV generatePV(Board board);

clock_t start;
uint64_t nodes;

int instantBetaCutoffs = 0;
int betaCutoffs = 0;

Move search(Board *board) {
	Move bestMove;

	initKillerMoves();

	timeManagement(board);
	start = clock();

	const int index = board->key % HASHTABLE_MAX_SIZE;

	int alpha = -INFINITY, beta = INFINITY, delta = 15;
	int score;

	for (int depth = 1; depth <= settings.depth; ++depth) {
		nodes = 0;

		// Aspiration window
		if (depth >= 6) {
			alpha = score - delta;
			beta = score + delta;
		}

		while (1) {
			score = alphabeta(board, depth, alpha, beta, 0);

			if (settings.stop)
				break;

			if (score >= beta) {
				beta += delta;
			} else if (score <= alpha) {
				beta = (beta + alpha) / 2;
				alpha -= delta;
			} else {
				break;
			}

			delta += delta / 2;
		}

		if (settings.stop) break;

		bestMove = decompressMove(board, &tt[index].move);

		PV pv = generatePV(*board);

		const long duration = 1000 * (clock() - start) / CLOCKS_PER_SEC;
		infoString(board, &pv, score, depth, duration, nodes);
	}

	printf("Beta cutoff rate: %.4f\n", (float) instantBetaCutoffs / betaCutoffs);

	assert(bestMove.to != bestMove.from);

	return bestMove;
}

/*
 * A basic implementation of alpha-beta pruning
 * Checkmate in n moves is found at depth n+1
 * alpha is the value to maximize and beta the value to minimize.
 * The initial position must have >= 1 legal moves and the initial depth must be >= 1.
 */
const int alphabeta(Board *board, int depth, int alpha, int beta, const int nullmove) {
	assert(beta >= alpha);
	assert(depth >= 0);

	if (settings.stop)
		return 0;

	if (settings.movetime && clock() - start > settings.movetime) {
		settings.stop = 1;
		return 0;
	}

	++nodes;

	const int in_check = inCheck(board);

	// Check extensions
	if (in_check)
		++depth;

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

	const int pruning = !nullmove && !in_check && !isEndgame(board);

	// Null Move Pruning
	if (pruning && depth > R) {

		makeNullMove(board, &history);
		const int score = -alphabeta(board, depth - 1 - R, -beta, -beta + 1, 1);
		undoNullMove(board, &history);

		if (score >= beta) {
			return beta;
		}
	}

	// Reverse futility pruning
	if (pruning && depth <= 3 && beta <= MAX_SCORE) {
		const int score = eval(board);

		if ((depth == 1 && score - pieceValues[KNIGHT] > beta) ||
			(depth == 2 && score - pieceValues[ROOK]   > beta) ||
			(depth == 3 && score - pieceValues[QUEEN]  > beta) )
			return beta;
	}

	Move moves[MAX_MOVES];
	const int nMoves = legalMoves(board, moves);

	// It's a leaf node, there are no legal moves
	if (nMoves == 0)
		return finalEval(board, depth);

	moveOrdering(board, moves, nMoves);

	Move bestMove = moves[0];
	int bestScore = -INFINITY;

	const int origAlpha = alpha, origBeta = beta;
	const int newDepth = depth - 1;

	for (int i = 0; i < nMoves; ++i) {
		makeMove(board, &moves[i], &history);

		// Futility pruning
		/*
		if (newDepth == 1 && !moves[i].capture) {
			const int score = eval(board);
			const int gamma = pieceValues[KNIGHT];

			if (score + gamma < alpha) {
				undoMove(board, &moves[i], &history);
				continue;
			}
		}
		*/

		updateBoardKey(board, &moves[i], &history);
		saveKeyToMemory(board->key);

		int score;

		if (isDraw(board)) {
			score = 0;
		} else if (i == 0) {
			score = -alphabeta(board, newDepth, -beta, -alpha, nullmove);
		} else {
			int reduct = 0;

			// Late move reduction
			if (i > 4 && depth >= 3 && !in_check && !moves[i].capture && !moves[i].promotion)
				++reduct;

			// PV search
			score = -alphabeta(board, newDepth - reduct, -alpha-1, -alpha, nullmove);

			if (score > alpha && score < beta)
				score = -alphabeta(board, newDepth, -beta, -alpha, nullmove);
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

				if (alpha >= beta) {
					// There's has been a cutoff.
					++betaCutoffs;

					if (i == 0)
						++instantBetaCutoffs;

					// A quiet move is one that doesn't capture anything.
					if (!moves[i].capture) {
						saveKillerMove(&moves[i], board->ply);
					}

					break;
				}
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
	const int standPat = eval(board);

	if (standPat >= beta)
		return beta;

	// Delta pruning
	const int safety = pieceValues[QUEEN];

	if (standPat < alpha - safety && !isEndgame(board))
		return alpha;

	if (standPat > alpha)
		alpha = standPat;

	Move moves[MAX_MOVES];
	const int nMoves = legalMoves(board, moves);

	if (nMoves == 0)
		return alpha;

	moveOrdering(board, moves, nMoves);

	const int start = (moves[0].capture) ? 0 : 1;

	for (int i = start; i < nMoves; ++i) {
		if (!moves[i].capture)
			break;

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
 *	Generates the PV line out of the moves from the TT.ve_num < 4 || depth < 3 || in_check || move_type == CAPTURE || move_type == QUEEN_PROMO || move_type == QUEEN_PROMO_CAPTURE)
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


