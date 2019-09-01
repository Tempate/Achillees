#ifndef HEADERS_PAWNS_H_
#define HEADERS_PAWNS_H_

extern const uint64_t wPawnAttack[64];
extern const uint64_t bPawnAttack[64];

uint64_t pawnAttacks(const Board *board, const int color);
void pawnMoves(const Board *board, Move *moves, int *n, const uint64_t checkAttack, const uint64_t pinned);

#endif
