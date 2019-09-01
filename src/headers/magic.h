#ifndef HEADERS_MAGIC_H_
#define HEADERS_MAGIC_H_

#define MAX_BISH_BLOCKERS 9
#define MAX_ROOK_BLOCKERS 12
#define BISH_SHIFT (64 - MAX_BISH_BLOCKERS)
#define ROOK_SHIFT (64 - MAX_ROOK_BLOCKERS)

extern const uint64_t diagonalMasks[64];
extern const uint64_t straightMasks[64];

void initMagics(void);
void generateMagics(void);

uint64_t bishopAttacks(const int sqr, const uint64_t occupied);
uint64_t xrayBishopAttacks(const int sqr, const uint64_t occupied, const uint64_t myPieces);

uint64_t rookAttacks(const int sqr, const uint64_t occupied);
uint64_t xrayRookAttacks(const int sqr, const uint64_t occupied, const uint64_t myPieces);


#endif
