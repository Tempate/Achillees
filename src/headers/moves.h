#ifndef SRC_MOVES_H_
#define SRC_MOVES_H_

#include "board.h"

#define MAX_MOVES 218

typedef struct {
	int from;
	int to;

	int piece;
	int color;

	int castle;
	int promotion;

	int score;
	int capture;
} Move;

typedef struct {
	int castling;
	int enPassant;
	int capture;
	int fiftyMoves;
} History;

int legalMoves(Board *board, Move *moves);
int pseudoLegalMoves(const Board *board, Move *moves);

long perft(Board *board, int depth);

int isLegalMove(Board *board, const Move *move);

int inCheck(const Board *board);
int kingInCheck(const Board *board, const uint64_t kingBB, const int color);

int givesCheck(Board *board, const Move *move);
int getSmallestAttacker(Board *board, const int sqr, const int color);

uint64_t bishopMoves(int index, uint64_t occupied, uint64_t myPieces);
uint64_t rookMoves  (int index, uint64_t occupied, uint64_t myPieces);
uint64_t queenMoves (int index, uint64_t occupied, uint64_t myPieces);

uint64_t kingMoves(int index);

int compareMoves(const Move *moveA, const Move *moveB);

#endif /* SRC_MOVES_H_ */
