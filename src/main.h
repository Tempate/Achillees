#ifndef MAIN_H_
#define MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#define ENGINE_NAME "Achillees"
#define ENGINE_AUTHOR "tempate"

#define FILES 8
#define RANKS 8
#define PIECES 6

extern uint64_t pow2[64];

enum {WHITE, BLACK};
enum {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};
enum {KCastle=1, QCastle=2, kCastle=4, qCastle=8};

typedef struct {
	uint64_t pieces[2][6];
	uint64_t players[2];

	uint64_t empty;
	uint64_t occupied;

	uint64_t key;

	int turn;
	int castling;
	int enPassant;

	int fiftyMoves;
	int ply;
} Board;

#endif /* MAIN_H_ */
