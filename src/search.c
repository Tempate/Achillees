#include "board.h"
#include "uci.h"
#include "moves.h"
#include "play.h"
#include "draw.h"
#include "eval.h"
#include "sort.h"
#include "search.h"
#include "hashtables.h"

#include <time.h>
#include <string.h>


static int qsearch(Board *board, int alpha, int beta);

static void timeManagement(const Board *board);

clock_t start;

Stats stats = (Stats){};

Move search(Board *board) {
	Move bestMove;

	stats.nodes = 0;

	initKillerMoves();

	timeManagement(board);
	start = clock();

	const int index = board->key % settings.tt_entries;

	int alpha = -INFINITY, beta = INFINITY, delta;
	int score;

	for (int depth = 1; depth <= settings.depth; ++depth) {

		// Aspiration window
		delta = 15;

		if (depth >= 6) {
			alpha = score - delta;
			beta = score + delta;
		}

		while (1) {
			score = pvSearch(board, depth, alpha, beta, 0);

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

		PV pv = probePV(*board);

		const long duration = 1000 * (clock() - start) / CLOCKS_PER_SEC;
		infoString(board, &pv, score, depth, duration, stats.nodes);
	}

	#ifdef DEBUG
	fprintf(stdout, "\nBeta-cutoff rate: %.4f\n", (float) stats.instCutoffs / stats.betaCutoffs);
	fprintf(stdout, "TT hits: %d\n\n", stats.ttHits);
	fflush(stdout);
	#endif

	assert(bestMove.to != bestMove.from);

	return bestMove;
}


int pvSearch(Board *board, int depth, int alpha, int beta, const int nullmove) {
	if (settings.stop)
		return 0;

	if (settings.movetime && depth > 2 && clock() - start > settings.movetime) {
		settings.stop = 1;
		return 0;
	}

	const int incheck = inCheck(board);

	if (incheck) 			// Check extensions
		++depth;
	else if (depth == 0) 	// Quiescence search
		return qsearch(board, alpha, beta);

	++stats.nodes;

	const int index = board->key % settings.tt_entries;

	// Transposition Table
	if (tt[index].key == board->key && tt[index].depth == depth) {
		
		#ifdef DEBUG
		++stats.ttHits;
		#endif

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
	const int pvNode = beta - alpha > 1;

	if (!nullmove && !incheck && !isEndgame(board)) {

		// Razoring
		if (depth == 1 && staticEval + pieceValues[ROOK] < alpha)
			return qsearch(board, alpha, beta);

		// Static null move pruning
		if (depth <= 6 && staticEval - pieceValues[PAWN] * depth > beta)
			return staticEval;

		// Null move pruning
		if (depth > R) {
			makeNullMove(board, &history);
			const int score = -pvSearch(board, depth - 1 - R, -beta, -beta + 1, 1);
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
			score = -pvSearch(board, newDepth, -beta, -alpha, nullmove);
		} else {
			int reduct = 0;

			// Late move reduction
			if (i > 4 && depth >= 3 && !incheck && moves[i].type == QUIET && !moves[i].promotion)
				++reduct;

			// PV search
			score = -pvSearch(board, newDepth - reduct, -alpha-1, -alpha, nullmove);

			if (score > alpha && score < beta)
				score = -pvSearch(board, newDepth, -beta, -alpha, nullmove);
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

