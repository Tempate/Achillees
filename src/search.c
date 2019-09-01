#include <assert.h>
#include <time.h>
#include <string.h>

#include "headers/board.h"
#include "headers/uci.h"
#include "headers/moves.h"
#include "headers/play.h"
#include "headers/draw.h"
#include "headers/eval.h"
#include "headers/sort.h"
#include "headers/search.h"
#include "headers/hashtables.h"


static int qsearch(Board *board, int alpha, int beta);

static void timeManagement(const Board *board);

static PV generatePV(Board board);

clock_t start;

Stats stats = (Stats){};

Move search(Board *board) {
	Move bestMove;

	initKillerMoves();

	timeManagement(board);
	start = clock();

	const int index = board->key % HASHTABLE_MAX_SIZE;

	int alpha = -INFINITY, beta = INFINITY, delta = 15;
	int score;

	for (int depth = 1; depth <= settings.depth; ++depth) {

		// Aspiration window
		delta = 15;

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
		infoString(board, &pv, score, depth, duration, stats.nodes);
	}

	#ifdef DEBUG
	fprintf(stdout, "Beta-cutoff rate: %.4f\n", (float) stats.instCutoffs / stats.betaCutoffs);
	fflush(stdout);
	#endif

	assert(bestMove.to != bestMove.from);

	return bestMove;
}

/*
 * A basic implementation of alpha-beta pruning
 * Checkmate in n moves is found at depth n+1
 * alpha is the value to maximize and beta the value to minimize.
 * The initial position must have >= 1 legal moves and the initial depth must be >= 1.
 */
int alphabeta(Board *board, int depth, int alpha, int beta, const int nullmove) {
	if (settings.stop)
		return 0;

	if (settings.movetime && clock() - start > settings.movetime) {
		settings.stop = 1;
		return 0;
	}

	const int incheck = inCheck(board);

	if (incheck) 			// Check extensions
		++depth;
	else if (depth == 0) 	// Quiescence search
		return qsearch(board, alpha, beta);

	++stats.nodes;

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

	const int staticEval = eval(board);

	if (!nullmove && !incheck && !isEndgame(board)) {

		// Razoring
		if (depth == 1 && staticEval + pieceValues[ROOK] < alpha) {
			return qsearch(board, alpha, beta);
		}

		// Static null move pruning
		if (depth <= 6 && staticEval - pieceValues[PAWN] * depth > beta) {
			return staticEval;
		}

		// Null move pruning
		if (depth > R) {
			makeNullMove(board, &history);
			const int score = -alphabeta(board, depth - 1 - R, -beta, -beta + 1, 1);
			undoNullMove(board, &history);

			if (score >= beta)
				return beta;
		}

		// Reverse futility pruning
		if (beta <= MAX_SCORE) {
			if (depth == 1 && staticEval - pieceValues[KNIGHT] >= beta) return beta;
			if (depth == 2 && staticEval - pieceValues[ROOK]   >= beta) return beta;

			// Razoring tests where depth is decreased don't give any elo
			if (depth == 3 && staticEval - pieceValues[QUEEN]  >= beta) return beta;
		}
	}

	Move moves[MAX_MOVES];
	const int nMoves = legalMoves(board, moves);

	// It's a leaf node, there are no legal moves
	if (nMoves == 0)
		return finalEval(board, depth);

	sort(board, moves, nMoves);

	Move bestMove = moves[0];
	int bestScore = -INFINITY;

	const int origAlpha = alpha, origBeta = beta;
	const int newDepth = depth - 1;

	static const int futMargins[] = {200, 300, 500};
	const int futPrun = depth <= 3 && !incheck && staticEval + futMargins[depth-1] <= alpha;

	for (int i = 0; i < nMoves; ++i) {

		// Futility Pruning
		if (futPrun && i != 0 && moves[i].type == QUIET && !moves[i].promotion)
			continue;

		makeMove(board, &moves[i], &history);
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
			if (i > 4 && depth >= 3 && !incheck && moves[i].type == QUIET && !moves[i].promotion)
				++reduct;

			// PV search
			score = -alphabeta(board, newDepth - reduct, -alpha-1, -alpha, nullmove);

			if (score > alpha && score < beta)
				score = -alphabeta(board, newDepth, -beta, -alpha, nullmove);
		}

		freeKeyFromMemory();
		updateBoardKey(board, &moves[i], &history);
		undoMove(board, &moves[i], &history);

		// Updates the best value and prunes the tree when alpha >= beta
		if (score > bestScore) {
			bestScore = score;
			bestMove = moves[i];

			if (bestScore > alpha) {
				alpha = bestScore;

				if (alpha >= beta) {

					if (moves[i].type == QUIET) {
						// Killer moves are moves that produce a cutoff despite being quiet
						saveKillerMove(&moves[i], board->ply);
					}

					#ifdef DEBUG
					++stats.betaCutoffs;

					if (i == 0)
						++stats.instCutoffs;
					#endif

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

static int qsearch(Board *board, int alpha, int beta) {
	++stats.nodes;

	const int standPat = eval(board);

	if (standPat >= beta)
		return beta;

	// Delta pruning
	if (standPat + pieceValues[QUEEN] < alpha && !isEndgame(board))
		return alpha;
	else if (standPat > alpha)
		alpha = standPat;

	Move moves[MAX_MOVES];
	const int nMoves = legalMoves(board, moves);

	if (nMoves == 0)
		return alpha;

	sort(board, moves, nMoves);

	for (int i = 0; i < nMoves; ++i) {
		// Prunes quiet moves and moves with negative SEE
		if (moves[i].score < 60)
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
	if (!settings.movetime) {
		clock_t remaining, increment;

		if (board->turn == WHITE) {
			remaining = settings.wtime;
			increment = settings.winc;
		} else {
			remaining = settings.btime;
			increment = settings.binc;
		}

		if (remaining || increment) {
			const clock_t timeToMove = min(remaining >> 2, (remaining >> 5) + increment) - 20;
			settings.movetime = (timeToMove * CLOCKS_PER_SEC) / 1000;
		}
	} else {
		settings.movetime *= CLOCKS_PER_SEC / 1000;
	}
}

// Finds the PV line from the TT. Due to collisions, it sometimes can be incomplete.
static PV generatePV(Board board) {
	PV pv = (PV) {.count = 0};

	while (1) {
		const int index = board.key % HASHTABLE_MAX_SIZE;

		// There's been a collision
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
