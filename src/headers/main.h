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

#define MAX_GAME_LENGTH 1024

extern uint64_t square[64];

enum {WHITE, BLACK};
enum {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};
enum {KCastle=1, QCastle=2, kCastle=4, qCastle=8};

static inline int max(const int a, const int b) { return (a > b) ? a : b; }
static inline int min(const int a, const int b) { return (a < b) ? a : b; }

#endif /* MAIN_H_ */
