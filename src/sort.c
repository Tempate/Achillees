#include <assert.h>

#include "board.h"
#include "play.h"
#include "eval.h"
#include "search.h"
#include "hashtables.h"
#include "sort.h"

static void insertionSort(Move *list, const int n);

static Move killerMoves[MAX_GAME_LENGTH][2];

/*
 * 1. Best move from the previous PV
 * 2. SEE for captures
 * 3. Killer moves
 */
void sort(Board *board, Move *moves, const int nMoves) {
	const int index = board->key % settings.tt_entries;
	Move pvMove = decompressMove(board, &tt[index].move);

	for (int i = 0; i < nMoves; ++i) {
		if (board->key == tt[index].key && compareMoves(&pvMove, &moves[i])) {
			moves[i].score = INFINITY;

		} else if (moves[i].type == CAPTURE) {
			moves[i].score = 60 + seeCapture(board, &moves[i]);

		} else {

			if (compareMoves(&moves[i], &killerMoves[board->ply][0]))
				moves[i].score = 50;
			else if (compareMoves(&moves[i], &killerMoves[board->ply][1]))
				moves[i].score = 45;
		}
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

	int value = 0;

	/* Skip it if the square isn't attacked by more pieces. */
	if (from != -1) {
		const int attacker = findPiece(board, bitmask[from], board->turn);
		const int pieceCaptured = findPiece(board, bitmask[sqr], board->opponent);

		assert(attacker >= 0 && pieceCaptured >= 0);

		const int promotion = (attacker == PAWN && (sqr < 8 || sqr >= 56)) ? QUEEN : 0;
		const Move move = (Move){.to=sqr, .from=from, .piece=attacker, .color=board->turn, .type=CAPTURE, .castle=-1, .promotion=promotion};

		History history;

		makeMove(board, &move, &history);
		const int score = pieceValues[pieceCaptured] - see(board, sqr);
		value = max(score, 0);
		undoMove(board, &move, &history);
	}

	return value;
}

int seeCapture(Board *board, const Move *move) {
	History history;

	const int pieceCaptured = findPiece(board, bitmask[move->to], board->opponent);

	makeMove(board, move, &history);
	const int value = pieceValues[pieceCaptured] - see(board, move->to);
	undoMove(board, move, &history);

	return value;
}

void sortByEval(Board *board, Move *moves, const int nMoves) {
	for (int i = 0; i < nMoves; ++i) {
		History history;

		makeMove(board, &moves[i], &history);
		moves[i].score = eval(board);
		undoMove(board, &moves[i], &history);
	}

	insertionSort(moves, nMoves);
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
