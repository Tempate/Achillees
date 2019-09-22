#include "board.h"
#include "play.h"
#include "eval.h"
#include "search.h"
#include "hashtables.h"
#include "sort.h"
#include "draw.h"

static int seePromotion(Board *board, const Move *move);

static void insertionSort(Move *list, const int n);

static Move killerMoves[MAX_GAME_LENGTH][2];

/*
 * 1. TT move
 * 2. Good captures
 * 3. Promotions
 * 4. Equal captures
 * 5. Killer moves
 * 6. Quiet moves
 * 7. Bad captures
 */
void sort(Board *board, Move *moves, const int nMoves) {

	const int index = board->key % settings.tt_entries;
	Move pvMove = decompressMove(board, &tt[index].move);

	for (int i = 0; i < nMoves; ++i) {
		if (board->key == tt[index].key && compareMoves(&pvMove, &moves[i]))
			moves[i].score = INFINITY;

		else if (moves[i].type == CAPTURE)
			moves[i].score = 60 + seeCapture(board, &moves[i]);
		
		else if (moves[i].promotion)
			moves[i].score = 65;
		
		else {

			if (compareMoves(&moves[i], &killerMoves[board->ply][0]))
				moves[i].score = 50;
			else if (compareMoves(&moves[i], &killerMoves[board->ply][1]))
				moves[i].score = 45;
		}
	}

	insertionSort(moves, nMoves);
}

void sortAB(Board *board, Move *moves, const int nMoves, const int depth, const int alpha, const int beta, const int nullmove) {

	for (int i = 0; i < nMoves; ++i) {

		History history;

		makeMove(board, &moves[i], &history);

		if (isDraw(board))
			moves[i].score = 0;
		else
			moves[i].score = -pvSearch(board, depth, -beta, -alpha, nullmove);
		
		undoMove(board, &moves[i], &history);
	}

	insertionSort(moves, nMoves);
}

void initKillerMoves(void) {
	const Move nullMove = (Move){.from=0,.to=0};

	for (int i = 0; i < MAX_GAME_LENGTH; ++i) {
		killerMoves[i][0] = nullMove;
		killerMoves[i][1] = nullMove;
	}
}

void saveKillerMove(const Move *move, const int ply) {
	killerMoves[ply][1] = killerMoves[ply][0];
	killerMoves[ply][0] = *move;
}

int see(Board *board, const int sqr) {
	const int from = getSmallestAttacker(board, sqr, board->turn);

	// Return if the square isn't attacked by more pieces
	if (from == -1)
		return 0;

	int value = 0;

	const int attacker = findPiece(board, bitmask[from], board->turn);
	const int pieceCaptured = findPiece(board, bitmask[sqr], board->opponent);

	ASSERT(attacker >= 0 && pieceCaptured >= 0);

	const int promotion = (attacker == PAWN && (sqr < 8 || sqr >= 56)) ? QUEEN : 0;
	const Move move = (Move){.to=sqr, .from=from, .piece=attacker, .color=board->turn, .type=CAPTURE, .castle=-1, .promotion=promotion};

	History history;

	makeMove(board, &move, &history);
	const int score = pieceValues[pieceCaptured] - see(board, sqr);
	value = max(score, 0);
	undoMove(board, &move, &history);

	return value;
}

int seeCapture(Board *board, const Move *move) {
	ASSERT(move->type == CAPTURE);

	History history;

	const int pieceCaptured = findPiece(board, bitmask[move->to], board->opponent);

	makeMove(board, move, &history);
	const int value = pieceValues[pieceCaptured] - see(board, move->to);
	undoMove(board, move, &history);

	return value;
}

// Orders a list of moves by their score using insertion sort
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
