#ifndef BOARD_H_
#define BOARD_H_

#include <inttypes.h>

#define unsetLSB(bb) bb &= bb - 1
#define lsbBB(bb) bb & -bb

#define NO_CHECK 0xffffffffffffffff

typedef struct {
	uint64_t pieces[2][6];
	uint64_t players[2];

	uint64_t empty;
	uint64_t occupied;

	uint64_t key;

	int turn;
	int opponent;
	int castling;
	int enPassant;

	int fiftyMoves;
	int ply;
} Board;

#include "main.h"

static const uint64_t notAFile = 0xfefefefefefefefe; // ~0x0101010101010101
static const uint64_t notHFile = 0x7f7f7f7f7f7f7f7f; // ~0x8080808080808080

static inline uint64_t get_sqr(int file, int rank) { return (uint64_t) 1ULL << (8*rank+file); }

static inline int get_rank(const int sqr) { return sqr / 8; }
static inline int get_file(const int sqr) { return sqr % 8; }

static inline uint64_t setBit  (uint64_t *bb, int i) { return *bb |= bitmask[i]; }
static inline uint64_t unsetBit(uint64_t *bb, int i) { return *bb &= ~bitmask[i]; }

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

static inline int  sameParity(const int a, const int b) { return ((a ^ b) & 1) == 0; }


#include "moves.h"

extern const char pieceChars[12];

void initialBoard(Board *board);
Board blankBoard(void);

void updateBoard(Board *board);
void updateOccupancy(Board *board);

int fenToBoard(Board *board, char *fen);
void boardToFen(const Board *board, char *fen);

void printBoard(const Board *board);

void printBB(const uint64_t bb);
int mirrorLSB(const uint64_t bb);

uint64_t line(const int a, const int b);

const char *sqrToCoord(int sq);
int coordToSqr(char *coord);

int charToPiece(char val);

#endif /* BOARD_H_ */
