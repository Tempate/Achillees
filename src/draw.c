#include "main.h"
#include "board.h"
#include "draw.h"


static int threeFoldRepetition(const Board *board);
static int insufficientMaterial(const Board *board);

Memory memory = (Memory) { .size = 0 };

int isDraw(const Board *board) {
	if (board->fiftyMoves >= 100)
		return 1;

	if (threeFoldRepetition(board))
		return 1;

	if (insufficientMaterial(board))
		return 1;

	return 0;
}

/*
 * A draw by insufficient material is declared on:
 *  - King vs King
 *  - King and Knight vs King
 *  - King and Bishop vs King
 *  - King and Bishop vs King and Bishop (of the same color)
 */
static int insufficientMaterial(const Board *board) {
	switch (popCount(board->players[WHITE])) {
	case 1:
		switch (popCount(board->players[BLACK])) {
		case 1:
			return 1;
		case 2:
			if (popCount(board->pieces[BLACK][KNIGHT]) || popCount(board->pieces[BLACK][BISHOP]))
				return 1;
			return 0;
		}
		return 0;
	case 2:
		switch (popCount(board->players[BLACK])) {
		case 1:
			if (popCount(board->pieces[WHITE][KNIGHT]) || popCount(board->pieces[WHITE][BISHOP]))
				return 1;
			break;
		case 2:
			if 	(popCount(board->pieces[WHITE][BISHOP]) == 1 &&
				 popCount(board->pieces[BLACK][BISHOP]) == 1 &&
				 sameParity(
						bitScanForward(board->pieces[WHITE][BISHOP]),
						bitScanForward(board->pieces[BLACK][BISHOP])))
			{
				return 1;
			}
			return 0;
		}
		return 0;
	}

	return 0;
}


static int threeFoldRepetition(const Board *board) {
	int counter = 1;

	for (int i = memory.size - board->fiftyMoves; i < memory.size; i += 2) {
		if (board->key == memory.keys[i])
			++counter;
	}

	return (counter >= 3) ? 1 : 0;
}

void saveKeyToMemory(const uint64_t key) {
	memory.keys[memory.size] = key;
	++(memory.size);
}

void freeKeyFromMemory(void) {
	--(memory.size);
}