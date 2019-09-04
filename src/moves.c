#include <assert.h>

#include "headers/board.h"
#include "headers/play.h"
#include "headers/magic.h"
#include "headers/pawns.h"

#include "headers/hashtables.h"


static uint64_t attackedSquares(Board *board);
static uint64_t pinnedPieces   (const Board *board);

static int numberOfChecks(const Board *board);
static uint64_t checkingAttack(const Board *board);

static uint64_t knightAttacks(const Board *board, const int color);
static void knightMoves(const Board *board, Move *moves, int *n, const uint64_t checkAttacks, const uint64_t pinned);

static uint64_t kingAttacks(const Board *board, const int color);
static void kingMoves(const Board *board, Move *moves, int *n, const uint64_t attacked, const uint64_t check);

static uint64_t slidingAttacks(const Board *board, uint64_t (*movesFunc)(int, uint64_t), uint64_t bb);
static void slidingMoves(const Board *board, Move *moves, int *n, const int piece, const int color, uint64_t (*movesFunc)(int, uint64_t), const uint64_t checkAttacks, const uint64_t pinned);

static inline uint64_t queenAttacks (const int index, const uint64_t occupied);

static inline void saveMoves(Move *moves, int *n, const int piece, const uint64_t movesBB, const int from, const int color, const int type, const uint64_t toBB);


// Lookup Tables

const uint64_t knightLookup[64] = {
		0x20400, 0x50800, 0xa1100, 0x142200, 0x284400, 0x508800, 0xa01000, 0x402000, 0x2040004, 0x5080008, 0xa110011, 0x14220022, 0x28440044, 0x50880088, 0xa0100010, 0x40200020, 0x204000402, 0x508000805, 0xa1100110a, 0x1422002214, 0x2844004428, 0x5088008850, 0xa0100010a0, 0x4020002040, 0x20400040200, 0x50800080500, 0xa1100110a00, 0x142200221400, 0x284400442800, 0x508800885000, 0xa0100010a000, 0x402000204000,0x2040004020000, 0x5080008050000, 0xa1100110a0000, 0x14220022140000, 0x28440044280000, 0x50880088500000, 0xa0100010a00000, 0x40200020400000, 0x204000402000000, 0x508000805000000, 0xa1100110a000000, 0x1422002214000000, 0x2844004428000000, 0x5088008850000000, 0xa0100010a0000000, 0x4020002040000000, 0x400040200000000, 0x800080500000000, 0x1100110a00000000, 0x2200221400000000, 0x4400442800000000,0x8800885000000000, 0x100010a000000000, 0x2000204000000000, 0x4020000000000, 0x8050000000000, 0x110a0000000000, 0x22140000000000, 0x44280000000000, 0x88500000000000, 0x10a00000000000, 0x20400000000000
};

const uint64_t kingLookup[64] = {
		0x302, 0x705, 0xe0a, 0x1c14, 0x3828, 0x7050, 0xe0a0, 0xc040, 0x30203, 0x70507, 0xe0a0e, 0x1c141c, 0x382838, 0x705070, 0xe0a0e0, 0xc040c0, 0x3020300, 0x7050700, 0xe0a0e00, 0x1c141c00, 0x38283800, 0x70507000, 0xe0a0e000, 0xc040c000, 0x302030000, 0x705070000, 0xe0a0e0000, 0x1c141c0000, 0x3828380000, 0x7050700000, 0xe0a0e00000, 0xc040c00000, 0x30203000000, 0x70507000000, 0xe0a0e000000, 0x1c141c000000,0x382838000000, 0x705070000000, 0xe0a0e0000000, 0xc040c0000000, 0x3020300000000, 0x7050700000000, 0xe0a0e00000000, 0x1c141c00000000, 0x38283800000000, 0x70507000000000, 0xe0a0e000000000, 0xc040c000000000, 0x302030000000000, 0x705070000000000, 0xe0a0e0000000000, 0x1c141c0000000000, 0x3828380000000000, 0x7050700000000000, 0xe0a0e00000000000, 0xc040c00000000000, 0x203000000000000, 0x507000000000000,0xa0e000000000000, 0x141c000000000000, 0x2838000000000000, 0x5070000000000000, 0xa0e0000000000000, 0x40c0000000000000
};



uint64_t perft(Board *board, int depth) {
	Move moves[MAX_MOVES];
	const int nMoves = legalMoves(board, moves);

	if (depth == 1)
		return nMoves;

	--depth;
	uint64_t nodes = 0;

	for (int i = 0; i < nMoves; ++i) {
		History history;

		Board oldBoard = *board;

		makeMove(board, &moves[i], &history);
		nodes += perft(board, depth);
		undoMove(board, &moves[i], &history);
	}

	return nodes;
}

int legalMoves(Board *board, Move *moves) {
	int n = 0;

	const uint64_t attacked = attackedSquares(board);
	const uint64_t pinned = pinnedPieces(board);

	uint64_t checkAttack = NO_CHECK;

	const uint64_t check = attacked & board->pieces[board->turn][KING];

	if (check) {
		assert(inCheck(board));

		if (numberOfChecks(board) == 1) {
			checkAttack = checkingAttack(board);
		} else {
			kingMoves(board, moves, &n, attacked, check);
			return n;
		}
	}

	pawnMoves  (board, moves, &n, checkAttack, pinned);
	knightMoves(board, moves, &n, checkAttack, pinned);

	slidingMoves(board, moves, &n, BISHOP, board->turn, bishopAttacks, checkAttack, pinned);
	slidingMoves(board, moves, &n, ROOK,   board->turn, rookAttacks,   checkAttack, pinned);
	slidingMoves(board, moves, &n, QUEEN,  board->turn, queenAttacks,  checkAttack, pinned);

	kingMoves  (board, moves, &n, attacked, check);

	return n;
}

int kingAttacked(const Board *board, const uint64_t kingBB, const int color) {
	const int opcolor = 1 ^ color, kingIndex = bitScanForward(kingBB);

	if (pawnAttacksLookup[color][kingIndex] & board->pieces[opcolor][PAWN]) return 1;

	if (bishopAttacks(kingIndex, board->occupied) & (board->pieces[opcolor][QUEEN] | board->pieces[opcolor][BISHOP])) return 1;
	if (rookAttacks  (kingIndex, board->occupied) & (board->pieces[opcolor][QUEEN] | board->pieces[opcolor][ROOK])) return 1;

	if (knightLookup[kingIndex] & board->pieces[opcolor][KNIGHT]) return 1;
	if (kingLookup  [kingIndex] & board->pieces[opcolor][KING]) return 1;

	return 0;
}

static int numberOfChecks(const Board *board) {
	const int kingIndex = bitScanForward(board->pieces[board->turn][KING]);

	uint64_t checks = 0;

	checks |= pawnAttacksLookup[board->turn][kingIndex] &  board->pieces[board->opponent][PAWN];
	checks |= bishopAttacks(kingIndex, board->occupied) & (board->pieces[board->opponent][QUEEN] | board->pieces[board->opponent][BISHOP]);
	checks |= rookAttacks  (kingIndex, board->occupied) & (board->pieces[board->opponent][QUEEN] | board->pieces[board->opponent][ROOK]);
	checks |= knightLookup[kingIndex] & board->pieces[board->opponent][KNIGHT];
	checks |= kingLookup  [kingIndex] & board->pieces[board->opponent][KING];

	return popCount(checks);
}

static uint64_t checkingAttack(const Board *board) {
	const int kingIndex = bitScanForward(board->pieces[board->turn][KING]);

	uint64_t attacks = 0;

	attacks |= pawnAttacksLookup[board->turn][kingIndex] & board->pieces[board->opponent][PAWN];
	attacks |= knightLookup[kingIndex] & board->pieces[board->opponent][KNIGHT];

	uint64_t checkingSliders =
			(bishopAttacks(kingIndex, board->occupied) & (board->pieces[board->opponent][QUEEN] | board->pieces[board->opponent][BISHOP])) |
			(rookAttacks  (kingIndex, board->occupied) & (board->pieces[board->opponent][QUEEN] | board->pieces[board->opponent][ROOK]));

	attacks |= checkingSliders;

	if (checkingSliders) do {
		attacks |= inBetweenLookup[kingIndex][bitScanForward(checkingSliders)];
	} while (unsetLSB(checkingSliders));

	return attacks;
}

static uint64_t attackedSquares(Board *board) {
	board->occupied ^= board->pieces[board->turn][KING];

	const uint64_t attacked = pawnAttacks  (board, board->opponent) |
							  knightAttacks(board, board->opponent) |
							  kingAttacks  (board, board->opponent) |
							  slidingAttacks(board, bishopAttacks, board->pieces[board->opponent][BISHOP]) |
							  slidingAttacks(board, rookAttacks,   board->pieces[board->opponent][ROOK])   |
							  slidingAttacks(board, queenAttacks,  board->pieces[board->opponent][QUEEN]);

	board->occupied ^= board->pieces[board->turn][KING];

	return attacked;
}

static uint64_t pinnedPieces(const Board *board) {
	uint64_t pinned = 0;

	const uint64_t kingIndex = bitScanForward(board->pieces[board->turn][KING]);

	uint64_t pinners = (xrayBishopAttacks(kingIndex, board->occupied, board->players[board->turn]) &
			  	  	   (board->pieces[board->opponent][BISHOP] | board->pieces[board->opponent][QUEEN])) |

			  	  	   (xrayRookAttacks(kingIndex, board->occupied, board->players[board->turn]) &
			  	  	   (board->pieces[board->opponent][ROOK] | board->pieces[board->opponent][QUEEN]));

	if (pinners) do {
		const int sqr = bitScanForward(pinners);
		pinned |= inBetweenLookup[sqr][kingIndex] & board->players[board->turn];
	} while (unsetLSB(pinners));

	return pinned;
}

// KNIGHT

static uint64_t knightAttacks(const Board *board, const int color) {
	uint64_t attacks = 0;

	uint64_t bb = board->pieces[color][KNIGHT];

	if (bb) do {
		const int from = bitScanForward(bb);
		attacks |= knightLookup[from];
	} while (unsetLSB(bb));

	return attacks;
}

static void knightMoves(const Board *board, Move *moves, int *n, const uint64_t checkAttacks, const uint64_t pinned) {
	// Knights are always absolutely pinned, so their moves don't have to be considered.
	uint64_t bb = board->pieces[board->turn][KNIGHT] & ~pinned;

	if (bb) do {
		const int from = bitScanForward(bb);
		const uint64_t movesBB = knightLookup[from] & checkAttacks;

		saveMoves(moves, n, KNIGHT, movesBB, from, board->turn, CAPTURE, board->players[board->opponent]);
		saveMoves(moves, n, KNIGHT, movesBB, from, board->turn, QUIET, board->empty);
	} while (unsetLSB(bb));
}

// KING

static uint64_t kingAttacks(const Board *board, const int color) {
	uint64_t attacks = 0;

	uint64_t bb = board->pieces[color][KING];

	if (bb) do {
		const int from = bitScanForward(bb);
		attacks |= kingLookup[from];
	} while (unsetLSB(bb));

	return attacks;
}

static void kingMoves(const Board *board, Move *moves, int *n, const uint64_t attacked, const uint64_t check) {
	static const uint64_t castlingSqrs[4] = {0x60, 0xe, 0x6000000000000000, 0xe00000000000000};
	static const uint64_t inBetweenSqr[4] = {0x60, 0xc, 0x6000000000000000, 0xc00000000000000};

	const int from = bitScanForward(board->pieces[board->turn][KING]);
	const uint64_t movesBB = kingLookup[from] & ~attacked;

	saveMoves(moves, n, KING, movesBB, from, board->turn, CAPTURE, board->players[board->opponent]);
	saveMoves(moves, n, KING, movesBB, from, board->turn, QUIET, board->empty);


	/* Castling. Ensures that:
	 * 		- The king is not in check.
	 * 		- The castle to perform is enabled.
	 * 		- The castling squares are empty.
	 * 		- The squares the king passes through are free.
	 */

	 if (!check) {
 		int index = 2 * board->turn;
 		int castle = board->castling & bitmask[index];

 		if (castle && (castlingSqrs[index] & board->occupied) == 0 && (inBetweenSqr[index] & attacked) == 0)
 			moves[(*n)++] = (Move){.from=from, .to=from + 2, .piece=KING, .color=board->turn, .castle=castle};

 		++index;
 		castle = board->castling & bitmask[index];

 		if (castle && (castlingSqrs[index] & board->occupied) == 0 && (inBetweenSqr[index] & attacked) == 0)
 			moves[(*n)++] = (Move){.from=from, .to=from - 2, .piece=KING, .color=board->turn, .castle=castle};
 	}
}

// SLIDING PIECES

static inline uint64_t queenAttacks(const int index, const uint64_t occupied) {
	return bishopAttacks(index, occupied) | rookAttacks(index, occupied);
}

static uint64_t slidingAttacks(const Board *board, uint64_t (*movesFunc)(int, uint64_t), uint64_t bb) {
	uint64_t attacks = 0;

	if (bb) do
		attacks |= movesFunc(bitScanForward(bb), board->occupied);
	while (unsetLSB(bb));

	return attacks;
}

static void slidingMoves(const Board *board, Move *moves, int *n, const int piece, const int color, uint64_t (*movesFunc)(int, uint64_t), const uint64_t checkAttacks, const uint64_t pinned) {
	const int kingIndex = bitScanForward(board->pieces[color][KING]);
	const int opcolor = 1 ^ color;

	uint64_t pinnedSliders = board->pieces[color][piece] & pinned;
	uint64_t bb = board->pieces[color][piece] ^ pinnedSliders;

	if (bb) do {
		const int from = bitScanForward(bb);
		const uint64_t movesBB = movesFunc(from, board->occupied) & checkAttacks;

		saveMoves(moves, n, piece, movesBB, from, color, CAPTURE, board->players[opcolor]);
		saveMoves(moves, n, piece, movesBB, from, color, QUIET, board->empty);
	} while (unsetLSB(bb));

	// A piece cannot move when the king is in check and it's pinned
	if (checkAttacks == NO_CHECK && pinnedSliders) do {
		const int from = bitScanForward(pinnedSliders);
		const uint64_t movesBB = movesFunc(from, board->occupied) & line(from, kingIndex);

		// If the piece is pinned it can only possibly capture the pinning piece.
		uint64_t attacker = movesBB & board->players[opcolor];

		if (attacker)
			moves[(*n)++] = (Move){.from=from, .to=bitScanForward(attacker), .piece=piece, .color=color, .type=CAPTURE};

		saveMoves(moves, n, piece, movesBB, from, color, QUIET, board->empty);
	} while (unsetLSB(pinnedSliders));
}



// TODO: This function needs to be improved.
int isLegalMove(Board *board, const Move *move) {
	Move moves[MAX_MOVES];
	const int nMoves = legalMoves(board, moves);

	for (int i = 0; i < nMoves; ++i) {
		if (compareMoves(&moves[i], move))
			return 1;
	}

	return 0;
}


int givesCheck(const Board *board, const Move *move) {
	const int opKingIndex = bitScanForward(board->pieces[board->opponent][KING]);

	static const uint64_t castledRook[4] = {0x20, 8, 0x2000000000000000, 0x800000000000000};

	uint64_t bishsAndQueens = board->pieces[board->turn][BISHOP] | board->pieces[board->turn][QUEEN];
	uint64_t rooksAndQueens = board->pieces[board->turn][ROOK]   | board->pieces[board->turn][QUEEN];

	uint64_t occupied = (board->occupied | bitmask[move->to]) ^ bitmask[move->from];

	switch (move->piece) {
	case PAWN:

		switch (move->promotion) {
		case KNIGHT:

			if (knightLookup[move->to] & board->pieces[board->opponent][KING])
				return 1;

			break;
		case BISHOP:
			bishsAndQueens ^= bitmask[move->to];
			break;
		case ROOK:
			rooksAndQueens ^= bitmask[move->to];
			break;
		case QUEEN:
			bishsAndQueens ^= bitmask[move->to];
			rooksAndQueens ^= bitmask[move->to];
			break;
		default:

			if (pawnAttacksLookup[move->color][move->to] & board->pieces[board->opponent][KING])
				return 1;

			if (move->to == board->enPassant)
				occupied ^= bitmask[move->to - 8 + 16 * move->color];
		}

		break;
	case KNIGHT:

		if (knightLookup[move->to] & board->pieces[board->opponent][KING])
			return 1;

		break;
	case BISHOP:
		bishsAndQueens ^= bitmask[move->from] | bitmask[move->to];
		break;
	case ROOK:
		rooksAndQueens ^= bitmask[move->from] | bitmask[move->to];
		break;
	case QUEEN:
		bishsAndQueens ^= bitmask[move->from] | bitmask[move->to];
		rooksAndQueens ^= bitmask[move->from] | bitmask[move->to];
		break;
	case KING:

		if (move->castle != -1)
			rooksAndQueens ^= castledRook[bitScanForward(move->castle)];

		break;
	}

	// Discovered checks

	if (bishsAndQueens & bishopAttacks(opKingIndex, occupied))
		return 1;

	if (rooksAndQueens & rookAttacks(opKingIndex, occupied))
		return 1;

	return 0;
}

/*
 * Returns the square the smallest attacker is in.
 * If no one is attacking that square, returns -1.
 */
int getSmallestAttacker(Board *board, const int sqr, const int color) {
	uint64_t attacker;

	attacker = pawnAttacksLookup[1 ^ color][sqr] & board->pieces[color][PAWN];
	if (attacker) return bitScanForward(attacker);

	attacker = knightLookup[sqr] & board->pieces[color][KNIGHT];
	if (attacker) return bitScanForward(attacker);


	const uint64_t bishop_moves = bishopAttacks(sqr, board->occupied);

	attacker = bishop_moves & board->pieces[color][BISHOP];
	if (attacker) return bitScanForward(attacker);


	const uint64_t rook_moves = rookAttacks(sqr, board->occupied);

	attacker = rook_moves & board->pieces[color][ROOK];
	if (attacker) return bitScanForward(attacker);


	attacker = (bishop_moves | rook_moves) & board->pieces[color][QUEEN];
	if (attacker) return bitScanForward(attacker);


	attacker = kingLookup[sqr] & board->pieces[color][KING];
	if (attacker) return bitScanForward(attacker);

	return -1;
}


// AUX

static inline void saveMoves(Move *moves, int *n, const int piece, const uint64_t movesBB, const int from, const int color, const int type, const uint64_t toBB) {
	uint64_t bb = movesBB & toBB;

	if (bb) do {
		const int to = bitScanForward(bb);
		moves[(*n)++] = (Move){.from=from, .to=to, .piece=piece, .color=color, .type=type, .castle=-1, .promotion=0};
	} while (unsetLSB(bb));
}

