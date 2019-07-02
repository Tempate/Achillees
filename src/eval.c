#include "board.h"
#include "eval.h"


enum {PAWN_VALUE=100, KNIGHT_VALUE=325, BISHOP_VALUE=325, ROOK_VALUE=525, QUEEN_VALUE=900};

#define BISHOP_PAIR 		+25
#define KNIGHT_PAIR			+15

#define DOUBLED_PAWN 		-10
#define ISOLATED_PAWN 		-3
#define PASSED_PAWN 		+20

#define ROOK_SEVENTH_RANK 	+15
#define DOUBLED_ROOKS 		+20
#define ROOK_OPEN_FILE 		+15
#define ROOK_SEMIOPEN_FILE 	+10



static int getPhase(Count whiteCount, Count blackCount);
static int materialCount(Count count);

static int pieceSquareTables(Board board, const int phase, const int color);

static int pawnStructure(Board board, const int color);
static int rookScore    (Board board, const int color);


/*
 * Evaluates a position which has no possible moves.
 * Returns 1 on checkmate and 0 on stalemate.
 */
int finalEval(Board board, const int depth) {
	const int color = board.turn;
	int result;

	if (kingInCheck(board, board.pieces[color][KING], color)) {
		result = -MAX_SCORE - depth;
	} else {
		result = 0;
	}

	return result;
}

/*
 * This is the main evaluation function.
 * Scores are positive for the side about to play.
 */
int eval(Board board) {
	const Count whiteCount = (Count) {
		.nPawns   = popCount(board.pieces[WHITE][PAWN]),
		.nKnights = popCount(board.pieces[WHITE][KNIGHT]),
		.nBishops = popCount(board.pieces[WHITE][BISHOP]),
		.nRooks   = popCount(board.pieces[WHITE][ROOK]),
		.nQueens  = popCount(board.pieces[WHITE][QUEEN]),

	};

	const Count blackCount = (Count) {
		.nPawns   = popCount(board.pieces[BLACK][PAWN]),
		.nKnights = popCount(board.pieces[BLACK][KNIGHT]),
		.nBishops = popCount(board.pieces[BLACK][BISHOP]),
		.nRooks   = popCount(board.pieces[BLACK][ROOK]),
		.nQueens  = popCount(board.pieces[BLACK][QUEEN]),
	};

	const int phase = getPhase(whiteCount, blackCount);

	int score = 0;
	score += materialCount(whiteCount) - materialCount(blackCount);
	score += pieceSquareTables(board, phase, WHITE) - pieceSquareTables(board, phase, BLACK);

	score += pawnStructure(board, WHITE) - pawnStructure(board, BLACK);
	score += rookScore(board, WHITE) - rookScore(board, BLACK);

	if (board.turn == BLACK)
		score = -score;

	return score;
}


static int getPhase(Count whiteCount, Count blackCount) {
    const int knightPhase = 1;
    const int bishopPhase = 1;
    const int rookPhase   = 2;
    const int queenPhase  = 4;

    const int totalPhase = 4 * knightPhase + 4 * bishopPhase + 4 * rookPhase + 2 * queenPhase;

    int phase = totalPhase -
    		knightPhase * (whiteCount.nKnights + blackCount.nKnights) -
    		bishopPhase * (whiteCount.nBishops + blackCount.nBishops) -
			rookPhase   * (whiteCount.nRooks   + blackCount.nRooks)   -
			queenPhase  * (whiteCount.nQueens  + blackCount.nQueens);

    return (phase * 256 + (totalPhase / 2)) / totalPhase;
}

static inline int materialCount(Count count) {
	int score = PAWN_VALUE * count.nPawns + KNIGHT_VALUE * count.nKnights + BISHOP_VALUE * count.nBishops + ROOK_VALUE * count.nRooks + QUEEN_VALUE * count.nQueens;

	if (count.nBishops >= 2)
		score += BISHOP_PAIR;

	if (count.nKnights >= 2)
		score += KNIGHT_PAIR;

	return score;
}

/*
 * Gives points to every piece depending on its position on the board.
 * It can be either a bonus or a penalty.
 * It also depends on the stage of the game: opening/middle-game or endgame.
 */
static int pieceSquareTables(Board board, const int phase, const int color) {
	static const int pst[2][6][64] = {
	{
		{ 0, 0, 0, 0, 0, 0, 0, 0, -1, -7, -11, -35, -13, 5, 3, -5, 1, 1, -6, -19, -6, -7, -4, 10, 1, 14, 8, 4, 5, 4, 10, 7, 9, 30, 23, 31, 31, 23, 17, 11, 21, 54, 72, 56, 77, 95, 71, 11, 118, 121, 173, 168, 107, 82, -16, 22, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ -99, -30, -66, -64, -29, -19, -61, -81, -56, -31, -28, -1, -7, -20, -42, -11, -38, -16, 0, 14, 8, 3, 3, -42, -14, 0, 2, 3, 19, 12, 33, -7, -14, -4, 25, 33, 10, 33, 14, 43, -22, 18, 60, 64, 124, 143, 55, 6, -34, 24, 54, 74, 60, 122, 2, 29, -60, 0, 0, 0, 0, 0, 0, 0 },
		{ -7, 12, -8, -37, -31, -8, -45, -67, 15, 5, 13, -10, 1, 2, 0, 15, 5, 12, 14, 13, 10, -1, 3, 4, 1, 5, 23, 32, 21, 8, 17, 4, -1, 16, 29, 27, 37, 27, 17, 4, 7, 27, 20, 56, 91, 108, 53, 44, -24, -23, 30, 58, 65, 61, 69, 11, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ -2, -1, 3, 1, 2, 1, 4, -8, -26, -6, 2, -2, 2, -10, -1, -29, -16, 0, 3, -3, 8, -1, 12, 3, -9, -5, 8, 14, 18, -17, 13, -13, 19, 33, 46, 57, 53, 39, 53, 16, 24, 83, 54, 75, 134, 144, 85, 75, 46, 33, 64, 62, 91, 89, 70, 104, 84, 0, 0, 37, 124, 0, 0, 153 },
		{ 1, -10, -11, 3, -15, -51, -83, -13, -7, 3, 2, 5, -1, -10, -7, -2, -11, 0, 12, 2, 8, 11, 7, -6, -9, 5, 7, 9, 18, 17, 26, 4, -6, 0, 15, 25, 32, 9, 26, 12, -16, 10, 13, 25, 37, 30, 15, 26, 1, 11, 35, 0, 16, 55, 39, 57, -13, 6, -42, 0, 29, 0, 0, 102 },
		{ 0, 0, 0, -9, 0, -9, 25, 0, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9 }
	},
	{
		{ 0, 0, 0, 0, 0, 0, 0, 0, -17, -17, -17, -17, -17, -17, -17, -17, -11, -11, -11, -11, -11, -11, -11, -11, -7, -7, -7, -7, -7, -7, -7, -7, 16, 16, 16, 16, 16, 16, 16, 16, 55, 55, 55, 55, 55, 55, 55, 55, 82, 82, 82, 82, 82, 82, 82, 82, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ -99, -99, -94, -88, -88, -94, -99, -99, -81, -62, -49, -43, -43, -49, -62, -81, -46, -27, -15, -9, -9, -15, -27, -46, -22, -3, 10, 16, 16, 10, -3, -22, -7, 12, 25, 31, 31, 25, 12, -7, -2, 17, 30, 36, 36, 30, 17, -2, -7, 12, 25, 31, 31, 25, 12, -7, -21, -3, 10, 16, 16, 10, -3, -21 },
		{ -27, -21, -17, -15, -15, -17, -21, -27, -10, -4, 0, 2, 2, 0, -4, -10, 2, 8, 12, 14, 14, 12, 8, 2, 11, 17, 21, 23, 23, 21, 17, 11, 14, 20, 24, 26, 26, 24, 20, 14, 13, 19, 23, 25, 25, 23, 19, 13, 8, 14, 18, 20, 20, 18, 14, 8, -2, 4, 8, 10, 10, 8, 4, -2 },
		{ -32, -31, -30, -29, -29, -30, -31, -32, -27, -25, -24, -24, -24, -24, -25, -27, -15, -13, -12, -12, -12, -12, -13, -15, 1, 2, 3, 4, 4, 3, 2, 1, 15, 17, 18, 18, 18, 18, 17, 15, 25, 27, 28, 28, 28, 28, 27, 25, 27, 28, 29, 30, 30, 29, 28, 27, 16, 17, 18, 19, 19, 18, 17, 16 },
		{ -61, -55, -52, -50, -50, -52, -55, -61, -31, -26, -22, -21, -21, -22, -26, -31, -8, -3, 1, 3, 3, 1, -3, -8, 9, 14, 17, 19, 19, 17, 14, 9, 19, 24, 28, 30, 30, 28, 24, 19, 23, 28, 32, 34, 34, 32, 28, 23, 21, 26, 30, 31, 31, 30, 26, 21, 12, 17, 21, 23, 23, 21, 17, 12 },
		{ -34, -30, -28, -27, -27, -28, -30, -34, -17, -13, -11, -10, -10, -11, -13, -17, -2, 2, 4, 5, 5, 4, 2, -2, 11, 15, 17, 18, 18, 17, 15, 11, 22, 26, 28, 29, 29, 28, 26, 22, 31, 34, 37, 38, 38, 37, 34, 31, 38, 41, 44, 45, 45, 44, 41, 38, 42, 46, 48, 50, 50, 48, 46, 42 }
	}};

	// This lookup table is used to mirror the piece-square tables for black
	static const int mirror64[64] = {
		56,	57,	58,	59,	60,	61,	62,	63,
		48,	49,	50,	51,	52,	53,	54,	55,
		40,	41,	42,	43,	44,	45,	46,	47,
		32,	33,	34,	35,	36,	37,	38,	39,
		24,	25,	26,	27,	28,	29,	30,	31,
		16,	17,	18,	19,	20,	21,	22,	23,
		8,	9,	10,	11,	12,	13,	14,	15,
		0,	1,	2,	3,	4,	5,	6,	7
	};

	uint64_t bb;

	int index, score, opening = 0, endgame = 0;

	void *indexFunc = (color == WHITE) ? bitScanForward : mirrorLSB;

	for (int piece = PAWN; piece <= KING; piece++) {
		bb = board.pieces[color][piece];

		if (bb) do {
			index = indexFunc(bb);
			opening += pst[OPENING][piece][index];
			endgame += pst[ENDGAME][piece][index];
		} while (unsetLSB(bb));
	}

	score = ((opening * (256 - phase)) + (endgame * phase)) / 256;

	return score;
}

/*
 * Evaluates the pawn structure. Takes into account:
 *   - Doubled pawns
 *   - Isolated pawns
 *   - Passed pawns
 */
static int pawnStructure(Board board, const int color) {
	static const uint64_t neighborPawns[8] = {0x2020202020200, 0x5050505050500, 0xa0a0a0a0a0a00, 0x14141414141400, 0x28282828282800, 0x50505050505000, 0xa0a0a0a0a0a000, 0x40404040404000};

	uint64_t bb = board.pieces[color][PAWN], opBB = board.pieces[1 ^ color][PAWN];
	int score = 0, pawn, neighbors;

	if (bb) do {
		pawn = bitScanForward(bb);
		neighbors = neighborPawns[get_file(pawn)];

		if ((pow2[pawn << 8] & board.pieces[color][PAWN]))
			score -= DOUBLED_PAWN;

		if ((neighbors & board.pieces[color][PAWN]) == 0)
			score -= ISOLATED_PAWN;

		if ((neighbors & board.pieces[1 ^ color][PAWN]) == 0)
			score += PASSED_PAWN;
	} while (unsetLSB(bb));

	return score;
}


static int rookScore(Board board, const int color) {
	static const uint64_t files[8] = {0x101010101010101, 0x202020202020202, 0x404040404040404, 0x808080808080808, 0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080};
	static const uint64_t rank7 = 0xff000000000000, rank2 = 0xff00;

	uint64_t bb = board.pieces[color][ROOK];
	const uint64_t seventhRank = bb & ((color == WHITE) ? rank7 : rank2);

	int score = 0;
	int rook, file;

	// +20 points for rooks on the seventh rank
	if (seventhRank)
		score += 20 * popCount(seventhRank);

	if (bb) do {
		rook = bitScanForward(bb);
		file = files[get_file(rook)];

		// +15 points for rooks on the same file
		// +10 points for rooks on open files
		// +3 points for rooks on semi-open files
		if ((file & board.pieces[color][PAWN]) == 0) {
			if (file & board.pieces[1 ^ color][PAWN]) {
				score += ROOK_SEMIOPEN_FILE;
			} else {
				score += ROOK_OPEN_FILE;
			}

			if (file & bb)
				score += DOUBLED_ROOKS;
		}
	} while (unsetLSB(bb));

	return score;
}
