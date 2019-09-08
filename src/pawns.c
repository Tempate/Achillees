#include "board.h"
#include "pawns.h"


static void wPawnPushMoves(Move **moves, const uint64_t bb, const uint64_t empty, const uint64_t checkAttack);
static void bPawnPushMoves(Move **moves, const uint64_t bb, const uint64_t empty, const uint64_t checkAttack);

static void wPinnedPawnsMoves(const Board *board, Move **moves, uint64_t pinnedPawns, const uint64_t opPieces);
static void bPinnedPawnsMoves(const Board *board, Move **moves, uint64_t pinnedPawns, const uint64_t opPieces);

static void addPawnMoves(Move **moves, uint64_t bb, const int color, const int shift, const int type);

static int typeOfPin(const int a, const int b);


// Static inlines for pawn moves

static inline uint64_t wSinglePushPawn(const uint64_t bb, const uint64_t empty) { return nortOne(bb) & empty; }
static inline uint64_t bSinglePushPawn(const uint64_t bb, const uint64_t empty) { return soutOne(bb) & empty; }

static inline uint64_t wDoublePushPawn(const uint64_t bb, const uint64_t empty) {
	static const uint64_t rank4 = 0x00000000FF000000;
	return wSinglePushPawn(bb, empty) & rank4;
}

static inline uint64_t bDoublePushPawn(const uint64_t bb, const uint64_t empty) {
	static const uint64_t rank5 = 0x000000FF00000000;
	return bSinglePushPawn(bb, empty) & rank5;
}

static inline uint64_t wCaptRightPawn (const uint64_t bb, const uint64_t opPieces) { return noEaOne(bb) & opPieces; }
static inline uint64_t bCaptRightPawn (const uint64_t bb, const uint64_t opPieces) { return soEaOne(bb) & opPieces; }
static inline uint64_t wCaptLeftPawn  (const uint64_t bb, const uint64_t opPieces) { return noWeOne(bb) & opPieces; }
static inline uint64_t bCaptLeftPawn  (const uint64_t bb, const uint64_t opPieces) { return soWeOne(bb) & opPieces; }


// Lookup tables

const uint64_t pawnAttacksLookup[2][64] = {
		{0x200, 0x500, 0xa00, 0x1400, 0x2800, 0x5000, 0xa000, 0x4000, 0x20000, 0x50000, 0xa0000, 0x140000, 0x280000, 0x500000, 0xa00000, 0x400000, 0x2000000, 0x5000000, 0xa000000, 0x14000000, 0x28000000, 0x50000000, 0xa0000000, 0x40000000, 0x200000000, 0x500000000, 0xa00000000, 0x1400000000, 0x2800000000, 0x5000000000, 0xa000000000, 0x4000000000, 0x20000000000, 0x50000000000, 0xa0000000000, 0x140000000000, 0x280000000000, 0x500000000000, 0xa00000000000, 0x400000000000, 0x2000000000000, 0x5000000000000, 0xa000000000000, 0x14000000000000, 0x28000000000000, 0x50000000000000, 0xa0000000000000, 0x40000000000000, 0x200000000000000, 0x500000000000000, 0xa00000000000000, 0x1400000000000000, 0x2800000000000000, 0x5000000000000000, 0xa000000000000000, 0x4000000000000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0x2, 0x5, 0xa, 0x14, 0x28, 0x50, 0xa0, 0x40, 0x200, 0x500, 0xa00, 0x1400, 0x2800, 0x5000, 0xa000, 0x4000, 0x20000, 0x50000, 0xa0000, 0x140000, 0x280000, 0x500000, 0xa00000, 0x400000, 0x2000000, 0x5000000, 0xa000000, 0x14000000, 0x28000000, 0x50000000, 0xa0000000, 0x40000000, 0x200000000, 0x500000000, 0xa00000000, 0x1400000000, 0x2800000000, 0x5000000000, 0xa000000000, 0x4000000000, 0x20000000000, 0x50000000000, 0xa0000000000, 0x140000000000, 0x280000000000, 0x500000000000, 0xa00000000000, 0x400000000000, 0x2000000000000, 0x5000000000000, 0xa000000000000, 0x14000000000000, 0x28000000000000, 0x50000000000000, 0xa0000000000000, 0x40000000000000}
};


uint64_t pawnAttacks(const Board *board, const int color) {

	if (color == WHITE)
		return noEaOne(board->pieces[color][PAWN]) | noWeOne(board->pieces[color][PAWN]);
	else
		return soEaOne(board->pieces[color][PAWN]) | soWeOne(board->pieces[color][PAWN]);
}

// TODO: Pinned en Passant
void pawnMoves(const Board *board, Move **moves, uint64_t checkAttack, const uint64_t pinned) {
	uint64_t opPieces = board->players[board->opponent];

	uint64_t pinnedPawns = board->pieces[board->turn][PAWN] & pinned;
	const uint64_t bb = board->pieces[board->turn][PAWN] ^ pinnedPawns;

	const int kingIndex = bitScanForward(board->pieces[board->turn][KING]);

	if (board->enPassant) {
		const uint64_t enPassantPawn = bitmask[board->enPassant - 8 + 16 * board->turn];

		// If the en-passant pawn is pinned, it cannot be captured.
		if ((enPassantPawn & pinned) == 0) {
			opPieces |= bitmask[board->enPassant];

			// The en-passant pawn is delivering check, so capturing en-passant
			// is a way to avoid it.
			if (enPassantPawn & pawnAttacksLookup[board->opponent][kingIndex])
				checkAttack |= bitmask[board->enPassant];
		}
	}

	if (board->turn == WHITE) {
		addPawnMoves(moves, wCaptRightPawn(bb, opPieces) & checkAttack, WHITE, 9, CAPTURE);
		addPawnMoves(moves, wCaptLeftPawn (bb, opPieces) & checkAttack, WHITE, 7, CAPTURE);

		wPawnPushMoves(moves, bb, board->empty, checkAttack);

		if (checkAttack == NO_CHECK)
			wPinnedPawnsMoves(board, moves, pinnedPawns, opPieces);
	} else {
		addPawnMoves(moves, bCaptRightPawn(bb, opPieces) & checkAttack, BLACK, -7, CAPTURE);
		addPawnMoves(moves, bCaptLeftPawn (bb, opPieces) & checkAttack, BLACK, -9, CAPTURE);

		bPawnPushMoves(moves, bb, board->empty, checkAttack);

		if (checkAttack == NO_CHECK)
			bPinnedPawnsMoves(board, moves, pinnedPawns, opPieces);
	}
}

static void wPawnPushMoves(Move **moves, const uint64_t bb, const uint64_t empty, const uint64_t checkAttack) {
	const uint64_t singlePush = wSinglePushPawn(bb, empty);
	uint64_t doublePush = wDoublePushPawn(singlePush, empty) & checkAttack;

	addPawnMoves(moves, singlePush & checkAttack, WHITE, 8, QUIET);

	if (doublePush) do {
		const int to = bitScanForward(doublePush);
		**moves = (Move){.from=to-16, .to=to, .enPassant=to-8, .piece=PAWN, .color=WHITE, .type=QUIET};
		(*moves)++;
	} while (unsetLSB(doublePush));
}

static void bPawnPushMoves(Move **moves, const uint64_t bb, const uint64_t empty, const uint64_t checkAttack) {
	const uint64_t singlePush = bSinglePushPawn(bb, empty);
	uint64_t doublePush = bDoublePushPawn(singlePush, empty) & checkAttack;

	addPawnMoves(moves, singlePush & checkAttack, BLACK, -8, QUIET);

	if (doublePush) do {
		const int to = bitScanForward(doublePush);
		**moves = (Move){.from=to+16, .to=to, .enPassant=to+8, .piece=PAWN, .color=BLACK, .type=QUIET};
		(*moves)++;
	} while (unsetLSB(doublePush));
}

static void wPinnedPawnsMoves(const Board *board, Move **moves, uint64_t pinnedPawns, const uint64_t opPieces) {
	const int kingIndex = bitScanForward(board->pieces[WHITE][KING]);

	if (pinnedPawns) do {
		const int pawn = bitScanForward(pinnedPawns);

		switch (typeOfPin(kingIndex, pawn)) {
		case VERTICAL:
			wPawnPushMoves(moves, bitmask[pawn], board->empty, NO_CHECK);
			break;
		case DIAGRIGHT:
			addPawnMoves(moves, wCaptRightPawn(bitmask[pawn], opPieces), WHITE, 9, CAPTURE);
			break;
		case DIAGLEFT:
			addPawnMoves(moves, wCaptLeftPawn(bitmask[pawn], opPieces), WHITE, 7, CAPTURE);
			break;
		}
	} while (unsetLSB(pinnedPawns));
}

static void bPinnedPawnsMoves(const Board *board, Move **moves, uint64_t pinnedPawns, const uint64_t opPieces) {
	const int kingIndex = bitScanForward(board->pieces[BLACK][KING]);

	if (pinnedPawns) do {
		const int pawn = bitScanForward(pinnedPawns);

		switch (typeOfPin(kingIndex, pawn)) {
		case VERTICAL:
			bPawnPushMoves(moves, bitmask[pawn], board->empty, NO_CHECK);
			break;
		case DIAGRIGHT:
			addPawnMoves(moves, bCaptLeftPawn(bitmask[pawn], opPieces), BLACK, -9, CAPTURE);
			break;
		case DIAGLEFT:
			addPawnMoves(moves, bCaptRightPawn(bitmask[pawn], opPieces), BLACK, -7, CAPTURE);
			break;
		}
	} while (unsetLSB(pinnedPawns));
}


// Adds pawn moves to the array considering promotions separately.
static void addPawnMoves(Move **moves, uint64_t bb, const int color, const int shift, const int type) {
	static const uint64_t rank1AndRank8 = 0xff000000000000ff;

	// Splits the bitboard into promoting pawns and non-promoting pawns
	uint64_t promoting = bb & rank1AndRank8;
	bb ^= promoting;

	// Adds the moves for all non-promoting pawns
	if (bb) do {
		const int to = bitScanForward(bb);
		**moves = (Move){.from=to-shift, .to=to, .piece=PAWN, .color=color, .type=type};
		(*moves)++;
	} while (unsetLSB(bb));

	// Adds the moves for all promoting pawns
	if (promoting) do {
		const int to = bitScanForward(promoting);
		const int from = to - shift;

		// A different move is considered for every possible promotion
		**moves = (Move){.from=from, .to=to, .piece=PAWN, .color=color, .type=type, .promotion=QUEEN};
		(*moves)++;
		
		**moves = (Move){.from=from, .to=to, .piece=PAWN, .color=color, .type=type, .promotion=KNIGHT};
		(*moves)++;
		
		**moves = (Move){.from=from, .to=to, .piece=PAWN, .color=color, .type=type, .promotion=BISHOP};
		(*moves)++;
		
		**moves = (Move){.from=from, .to=to, .piece=PAWN, .color=color, .type=type, .promotion=ROOK};
		(*moves)++;

	} while (unsetLSB(promoting));
}

static int typeOfPin(const int a, const int b) {
	assert(a >= 0 && a < 64 && b >= 0 && b < 64);

	const uint64_t inBetween = inBetweenLookup[a][b] | bitmask[a] | bitmask[b];
	const int c = min(a, b);

	if ((bitmask[c] << 1) & inBetween)
		return HORIZONTAL;

	if ((bitmask[c] << 8) & inBetween)
		return VERTICAL;

	if ((bitmask[c] << 9) & inBetween)
		return DIAGRIGHT;

	if ((bitmask[c] << 7) & inBetween)
		return DIAGLEFT;

	return NONE;
}