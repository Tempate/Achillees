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

Stats stats;

Move search(Board *board) {
	Move bestMove;

	stats = (Stats){ 0 };

	initKillerMoves();

	timeManagement(board);
	start = clock();

	const int index = board->key % settings.tt_entries;

	int alpha = -INFINITY, beta = INFINITY, delta;
	int score, depth;

	for (depth = 1; depth <= settings.depth; ++depth) {

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

		Move pv[MAX_DEPTH];
		const int nPV = probePV(*board, pv);

		const long duration = 1000 * (clock() - start) / CLOCKS_PER_SEC;
		
		infoString(board, depth, score, stats.nodes, duration, pv, nPV);

		// Stop looking when the fastest mate has been found
		if (abs(score) == MAX_SCORE + depth / 2)
			break;
	}

	#ifdef DEBUG
	fprintf(stdout, "\n");

	if (stats.betaCutoffs > 0)
		fprintf(stdout, "Beta-cutoff rate: %.4f\n", (float) stats.instCutoffs / stats.betaCutoffs);
	
	fprintf(stdout, "TT hits: %d\n\n", stats.ttHits);
	fflush(stdout);
	#endif

	// Makes sure the bestMove has been initialized
	ASSERT(bestMove.to != bestMove.from);
	ASSERT(depth > 1);

	return bestMove;
}


int pvSearch(Board *board, int depth, int alpha, int beta, const int nullmove) {
	if (settings.stop)
		return 0;

	if (settings.movetime && stats.nodes % 4096 == 0 && clock() - start > settings.movetime) {
		settings.stop = 1;
		return 0;
	}

	const int incheck = inCheck(board);

	if (incheck) 			// Check extensions
		++depth;
	else if (depth <= 0) 	// Quiescence search
		return qsearch(board, alpha, beta);

	++stats.nodes;

	const int index = board->key % settings.tt_entries;
	const int ttHit = tt[index].key == board->key;

	// Transposition Table
	if (ttHit && tt[index].depth == depth) {
		
		#ifdef DEBUG
		++stats.ttHits;
		#endif

		switch (tt[index].flag) {
		case LOWER_BOUND:
			alpha = max(alpha, tt[index].score);
			break;
		case UPPER_BOUND:
			beta = min(beta, tt[index].score);
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
	const int endgame = isEndgame(board);
	const int safe = !incheck && !endgame;
	const int verySafe = safe && !nullmove;

	// Razoring
	if (depth == 1 && safe && staticEval + pieceValues[ROOK] < alpha)
		return qsearch(board, alpha, beta);

	// Reverse Futility Pruning
	if (depth <= 4 && staticEval - pieceValues[PAWN] * depth > beta)
		return staticEval;

	// Null move reduction
	if (verySafe) {
		static const int R = 3;
		const int bound = beta;

		makeNullMove(board, &history);
		const int score = -pvSearch(board, depth - R - 1, -bound, -bound + 1, 1);
		undoNullMove(board, &history);

		if (score >= bound)
			return pvSearch(board, depth - R, alpha, beta, 0);
	}

	Move moves[MAX_MOVES];
	const int nMoves = legalMoves(board, moves);

	if (nMoves == 0)
		return finalEval(board, depth);

	// IID
	if (!ttHit && depth >= 7 && pvNode) {
		pvSearch(board, depth - 2, alpha, beta, nullmove);

		ASSERT(tt[index].key == board->key);
	}

	sort(board, moves, nMoves);

	Move bestMove = moves[0];
	int bestScore = -INFINITY;

	const int prevAlpha = alpha;
	const int newDepth = depth - 1;

	static const int fMargins[] = {0, 200, 300, 500};
	const int fPrunning = depth <= 3 && !incheck && staticEval + fMargins[depth] <= alpha;

	for (int i = 0; i < nMoves; ++i) {

		const int quietMove = moves[i].type == QUIET && !moves[i].promotion;

		// Futility Pruning
		if (!pvNode && fPrunning && quietMove)
			continue;

		// Late move pruning 
		// Skip a move when it's score is bad and it has low depth
		if (newDepth <= 6 && moves[i].score < -10 * depth * depth)
			continue;

		makeMove(board, &moves[i], &history);
		updateBoardKey(board, &moves[i], &history);

		// The board's key is saved to check for 3fold repetition
		saveKeyToMemory(board->key);

		int score;

		if (isDraw(board))
			score = 0;
		
		else if (i == 0)
			score = -pvSearch(board, newDepth, -beta, -alpha, nullmove);
		
		else {
			int reduct = 0;

			// Late move reduction
			// Only quiet moves (excluding promotions) are reduced
			if (depth >= 2 && moves[i].score == 0 && !incheck)
				++reduct;

			// PV search
			// A search with a small window is used to see 
			// if the move is worth exploring further
			score = -pvSearch(board, newDepth - reduct, -alpha-1, -alpha, nullmove);

			// Research if the score is worth looking into
			if (score > alpha)
				score = -pvSearch(board, newDepth, -beta, -alpha, nullmove);
		}

		// The board's key is freed from the 3fold repetition list
		freeKeyFromMemory();

		updateBoardKey(board, &moves[i], &history);
		undoMove(board, &moves[i], &history);

		// Updates the best move
		if (score > bestScore) {
			bestScore = score;
			bestMove = moves[i];

			if (bestScore > alpha) {
				alpha = bestScore;

				// AlphaBeta prunning
				if (alpha >= beta) {

					// Killer moves are moves that produce a cutoff despite being quiet
					if (moves[i].type == QUIET)
						saveKillerMove(&moves[i], board->ply);

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

	if (bestScore <= prevAlpha)
		flag = UPPER_BOUND;
	else if (bestScore >= beta)
		flag = LOWER_BOUND;

	// Always replace the entry for the TT
	// UNLESS the position currently in there has a different key and goes deeper
	if (tt[index].key != board->key && tt[index].depth > depth) {
		return bestScore;
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
	const int delta = pieceValues[QUEEN];

	if (standPat + delta < alpha && !isEndgame(board))
		return alpha;
	
	if (standPat > alpha)
		alpha = standPat;

	Move moves[MAX_MOVES];
	const int nMoves = legalMoves(board, moves);

	if (nMoves == 0)
		return alpha;

	sort(board, moves, nMoves);

	const int incheck = inCheck(board);

	/*
	* 1. TT move
	* 2. Good captures
	* 3. Promotions
	* 4. Equal captures
	* 5. Killer moves
	*/
	for (int i = 0; i < nMoves; ++i) {

		if (moves[i].score < 40)
			break;

		// Futility pruning
		if (moves[i].type == CAPTURE && standPat + moves[i].score < alpha && 
			!incheck && !givesCheck(board, &moves[i]))
			continue;

		History history;

		makeMove(board, &moves[i], &history);
		const int score = -qsearch(board, -beta, -alpha);
		undoMove(board, &moves[i], &history);

		if (score >= beta) {
			#ifdef DEBUG
			++stats.betaCutoffs;

			if (i == 0)
				++stats.instCutoffs;
			#endif

			return beta;
		}

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

		if (settings.movestogo || increment) {
			if (settings.movestogo && settings.movestogo < 8)
				settings.movetime = min(remaining >> 1, remaining / (settings.movestogo + 12) + (clock_t)((double) increment * .4));
			else
				settings.movetime = min(remaining >> 2, remaining / 27 + (clock_t)((double)increment * .95));
        } else {
            settings.movetime = remaining / 41;
        }
	}

	settings.movetime *= CLOCKS_PER_SEC / 1000;
}

