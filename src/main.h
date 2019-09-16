#ifndef MAIN_H_
#define MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

#define ENGINE_NAME "Achillees"
#define ENGINE_AUTHOR "tempate"

#define FILES 8
#define RANKS 8
#define PIECES 6
#define SQRS 64

#define MAX_GAME_LENGTH 1024

// #define DEBUG

#ifdef DEBUG
    #define ASSERT(condition)\
    {\
        if (!(condition)) {\
            printf("info string file: %d\n", __FILE__);\
            printf("info string line: %d\n", __LINE__);\
            printf("info string function: %s\n", __PRETTY_FUNCTION__);\
            printf("info string condition: %s\n", #condition);\
            abort();\
        }\
    }
#else
    #define ASSERT(condition)
#endif

extern uint64_t bitmask[64];
extern uint64_t inBetweenLookup[64][64];

typedef struct {
	int stop;

	int depth;
	int nodes;
	int mate;

	int wtime;
	int btime;
	int winc;
	int binc;

	int movestogo;
	int movetime;

	long tt_entries;
	long tt_size;
} Settings;

extern Settings settings;

enum {WHITE, BLACK};
enum {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};
enum {KCastle=1, QCastle=2, kCastle=4, qCastle=8};

static inline int max(const int a, const int b) { return (a > b) ? a : b; }
static inline int min(const int a, const int b) { return (a < b) ? a : b; }

#include "board.h"

void defaultSettings(Settings *settings);
void evaluate(const Board *board);

#endif /* MAIN_H_ */
