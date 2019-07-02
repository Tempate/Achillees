#ifndef BOARD_H_
#define BOARD_H_

#include "main.h"
#include "moves.h"

#define unsetLSB(bb) bb &= bb - 1


static const uint64_t notAFile = 0xfefefefefefefefe; // ~0x0101010101010101
static const uint64_t notHFile = 0x7f7f7f7f7f7f7f7f; // ~0x8080808080808080

static inline uint64_t get_sqr(int file, int rank) { return (uint64_t) 1ULL << (8*rank+file); }

static inline int get_rank(uint64_t sqr) { return sqr / 8; }
static inline int get_file(uint64_t sqr) { return sqr % 8; }

static inline uint64_t setBit  (uint64_t *bb, int i) { return *bb |= pow2[i]; }
static inline uint64_t unsetBit(uint64_t *bb, int i) { return *bb &= ~pow2[i]; }

static inline uint64_t soutOne (uint64_t b) { return b >> 8; }
static inline uint64_t nortOne (uint64_t b) { return b << 8; }
static inline uint64_t eastOne (uint64_t b) { return (b << 1) & notAFile; }
static inline uint64_t noEaOne (uint64_t b) { return (b << 9) & notAFile; }
static inline uint64_t soEaOne (uint64_t b) { return (b >> 7) & notAFile; }
static inline uint64_t westOne (uint64_t b) { return (b >> 1) & notHFile; }
static inline uint64_t soWeOne (uint64_t b) { return (b >> 9) & notHFile; }
static inline uint64_t noWeOne (uint64_t b) { return (b << 7) & notHFile; }

static inline int  popCount       (uint64_t bb)  { return __builtin_popcountll(bb); }
static inline int  bitScanForward (uint64_t bb)  { return __builtin_ctzll(bb); }
static inline int  bitScanReverse (uint64_t bb)  { return 63 - __builtin_clzll(bb); }


Board blankBoard();

void printBoard(Board board);
void updateBoard(Board *board);
void updateOccupancy(Board *board);

void setBits  (Board *board, int color, int piece, int index);
void unsetBits(Board *board, int color, int piece, int index);

void printMoves(Move *moves, int n);
void printMove (Move move, int nodes);
void moveToText(Move move, char *text);
Move textToMove(Board board, char *text);

int parseFen(Board *board, char *fen);
void generateFen(Board board, char *fen);

void printBB(const uint64_t bb);
int mirrorLSB(const uint64_t bb);


#endif /* BOARD_H_ */
