#ifndef HEADERS_PAWNS_H_
#define HEADERS_PAWNS_H_

enum {NONE, VERTICAL, HORIZONTAL, DIAGRIGHT, DIAGLEFT};

extern const uint64_t pawnAttacksLookup[2][64];

uint64_t pawnAttacks(const Board *board, const int color);
void pawnMoves(Board *board, Move **moves, const uint64_t checkAttack, const uint64_t pinned);

#endif
