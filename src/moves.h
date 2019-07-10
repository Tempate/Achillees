#ifndef SRC_MOVES_H_
#define SRC_MOVES_H_

#include "board.h"

typedef struct {
	int from;
	int to;

	int piece;
	int color;

	int castle;
	int promotion;

	int score;
} Move;

typedef struct {
	int castling;
	int enPassant;
	int capture;
	int fiftyMoves;
} History;

int legalMoves(Board *board, Move *moves);
int isLegalMove(Board *board, const Move *move);

int inCheck(const Board *board);
int kingInCheck(const Board *board, uint64_t kingBB, int color);

const uint64_t kingMoves(int index);

int compareMoves(const Move *moveA, const Move *moveB);

#endif /* SRC_MOVES_H_ */
