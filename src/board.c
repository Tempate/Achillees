#include <string.h>

#include "main.h"
#include "board.h"
#include "draw.h"
#include "hashtables.h"

const char pieceChars[12] = {'P','N','B','R','Q','K','p','n','b','r','q','k'};


void initialBoard(Board *board) {
	static char* initial = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	
	clearTT();
	clearKeys();
	
	fenToBoard(board, initial);
}

Board blankBoard(void) {
	Board board = (Board) {
		.empty = ~0,
		.occupied = 0
	};

	memset(board.players, 0, 2*sizeof(int));
	memset(board.pieces[WHITE], 0, PIECES*sizeof(uint64_t));
	memset(board.pieces[BLACK], 0, PIECES*sizeof(uint64_t));

	return board;
}

void updateBoard(Board *board) {
	board->players[WHITE] =
			board->pieces[WHITE][PAWN]   | board->pieces[WHITE][KNIGHT] |
			board->pieces[WHITE][BISHOP] | board->pieces[WHITE][ROOK] |
			board->pieces[WHITE][QUEEN]  | board->pieces[WHITE][KING];

	board->players[BLACK] =
			board->pieces[BLACK][PAWN]   | board->pieces[BLACK][KNIGHT] |
			board->pieces[BLACK][BISHOP] | board->pieces[BLACK][ROOK] |
			board->pieces[BLACK][QUEEN]  | board->pieces[BLACK][KING];

	updateOccupancy(board);
}

void updateOccupancy(Board *board) {
	board->occupied = board->players[WHITE] | board->players[BLACK];
	board->empty = ~board->occupied;
}

// Returns the fens length
int fenToBoard(Board *board, char *fen) {
	int rank = RANKS-1, file = 0, i = 0;

	*board = blankBoard();

	while (fen[i] != ' ') {
		if (fen[i] == '/') {
			--rank;
			file = 0;
		} else if (fen[i] >= '1' && fen[i] <= '8') {
			file += fen[i] - '0';
		} else {
			const int piece = (int) charToPiece(fen[i]);
			const uint64_t sqr = get_sqr(file, rank);

			if (fen[i] >= 'A' && fen[i] <= 'Z') {
				board->pieces[WHITE][piece] |= sqr;
			} else if (fen[i] >= 'a' && fen[i] <= 'z') {
				board->pieces[BLACK][piece] |= sqr;
			}

			++file;
		}

		++i;
	}

	board->turn = (fen[++i] == 'w') ? WHITE : BLACK;
	board->opponent = board->turn ^ 1;

	i += 2;
	if (fen[i] == '-') {
		board->castling = 0;
		++i;
	} else {
		if (fen[i] == 'K') { board->castling += bitmask[0]; ++i; }
		if (fen[i] == 'Q') { board->castling += bitmask[1]; ++i; }
		if (fen[i] == 'k') { board->castling += bitmask[2]; ++i; }
		if (fen[i] == 'q') { board->castling += bitmask[3]; ++i; }
	}

	if (fen[++i] == '-') {
		board->enPassant = 0;
	} else {
		board->enPassant = fen[i] - 'a';
		board->enPassant += (fen[++i] - '1') * 8;
	}

	i += 2;

	board->fiftyMoves = 0;

	if (fen[i] >= '0' && fen[i] <= '9') {
		board->ply = 0;

		while (fen[i] >= '0' && fen[i] <= '9') {
			board->fiftyMoves *= 10;
			board->fiftyMoves += fen[i] - '0';
			++i;
		}

		++i;

		while (fen[i] >= '0' && fen[i] <= '9') {
			board->ply *= 10;
			board->ply += fen[i] - '0';
			++i;
		}
	} else {
		board->ply = 1;

		--i;
	}

	board->key = zobristKey(board);

	updateBoard(board);

	return i;
}

void boardToFen(const Board *board, char *fen) {
	int k = -1;

	for (int y = RANKS-1; y >= 0; y--) {
		int blanks = 0;

		for (int x = 0; x < FILES; x++) {
			const uint64_t sqr = get_sqr(x,y);

			if (board->empty & sqr) {
				++blanks;
			} else {
				if (blanks > 0) {
					fen[++k] = blanks + '0';
					blanks = 0;
				}

				for (int i = 0; i < PIECES; ++i) {
					if (board->pieces[WHITE][i] & sqr) {
						fen[++k] = pieceChars[i];
						break;
					} else if (board->pieces[BLACK][i] & sqr) {
						fen[++k] = pieceChars[i + 6];
						break;
					}
				}
			}
		}

		if (blanks > 0) {
			fen[++k] = blanks + '0';
			blanks = 0;
		}

		if (y != 0)
			fen[++k] = '/';
	}

	fen[++k] = ' ';
	fen[++k] = (board->turn == WHITE) ? 'w' : 'b';
	fen[++k] = ' ';

	if (board->castling == -1) {
		fen[++k] = '-';
	} else {
		if (board->castling & bitmask[0]) fen[++k] = 'K';
		if (board->castling & bitmask[1]) fen[++k] = 'Q';
		if (board->castling & bitmask[2]) fen[++k] = 'k';
		if (board->castling & bitmask[3]) fen[++k] = 'q';
	}

	fen[++k] = ' ';
	if (board->enPassant == 0) {
		fen[++k] = '-';
	} else {
		fen[++k] = get_file(board->enPassant) + 'a';
		fen[++k] = get_rank(board->enPassant) + '1';
	}

	/*
	fen[++k] = ' ';
	fen[++k] = board->fiftyMoves + '0';
	fen[++k] = ' ';
	fen[++k] = board->ply + '0';
	*/

	fen[++k] = '\0';
}

void printBoard(const Board *board) {
	fprintf(stdout, "\n");

	for (int y = RANKS-1; y >= 0; y--) {
		for (int x = 0; x < FILES; x++) {
			const uint64_t bb = get_sqr(x,y);

			if (board->empty & bb) {
				fprintf(stdout, ". ");
			} else {
				const int color = (board->players[WHITE] & bb) ? WHITE : BLACK;

				for (int j = 0; j < PIECES; j++) {
					if (board->pieces[color][j] & bb) {
						fprintf(stdout, "%c ", pieceChars[j + 6*color]);
						break;
					}
				}
			}
		}

		fprintf(stdout, "\n");
	}

	fprintf(stdout, "\n");
	fprintf(stdout, "Turn: %s\n", (board->turn == WHITE) ? "White" : "Black");
	fprintf(stdout, "Castling: ");

	if (board->castling) {
		if (board->castling & KCastle) fprintf(stdout, "K");
		if (board->castling & QCastle) fprintf(stdout, "Q");
		if (board->castling & kCastle) fprintf(stdout, "k");
		if (board->castling & qCastle) fprintf(stdout, "q");
	} else {
		fprintf(stdout, "-");
	}

	fprintf(stdout, "\n");
	fprintf(stdout, "En passant: %s\n", (board->enPassant > 0) ? sqrToCoord(board->enPassant) : "-");
	fprintf(stdout, "Fifty Moves: %d\n", board->fiftyMoves);
	fprintf(stdout, "Ply: %d\n\n", board->ply);
	fflush(stdout);
}

void printBB(const uint64_t bb) {
	fprintf(stdout, "----------------\n");

	for (int y = 7; y >= 0; y--) {
		for (int x = 0; x < 8; x++) {
			fprintf(stdout, "%d ", (bb & get_sqr(x,y)) ? 1 : 0);
		}

		fprintf(stdout, "\n");
	}

	fprintf(stdout, "----------------\n");
	fflush(stdout);
}


// This lookup table is used to mirror the piece-square tables for black
int mirrorLSB(const uint64_t bb) {
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

	return mirror64[bitScanForward(bb)];
}

// Returns the line that connects two points, if there's one.
uint64_t line(const int a, const int b) {

	// Lookup table for all eight ray-directions: NORT, NOEA, EAST, SOEA, SOUT, SOWE, WEST, NOWE
	static const uint64_t rayLookup[8][64] = {
		{0x101010101010100, 0x202020202020200, 0x404040404040400, 0x808080808080800, 0x1010101010101000, 0x2020202020202000, 0x4040404040404000, 0x8080808080808000, 0x101010101010000, 0x202020202020000, 0x404040404040000, 0x808080808080000, 0x1010101010100000, 0x2020202020200000, 0x4040404040400000, 0x8080808080800000, 0x101010101000000, 0x202020202000000, 0x404040404000000, 0x808080808000000, 0x1010101010000000, 0x2020202020000000, 0x4040404040000000, 0x8080808080000000, 0x101010100000000, 0x202020200000000, 0x404040400000000, 0x808080800000000, 0x1010101000000000, 0x2020202000000000, 0x4040404000000000, 0x8080808000000000, 0x101010000000000, 0x202020000000000, 0x404040000000000, 0x808080000000000, 0x1010100000000000, 0x2020200000000000, 0x4040400000000000, 0x8080800000000000, 0x101000000000000, 0x202000000000000, 0x404000000000000, 0x808000000000000, 0x1010000000000000, 0x2020000000000000, 0x4040000000000000, 0x8080000000000000, 0x100000000000000, 0x200000000000000, 0x400000000000000, 0x800000000000000, 0x1000000000000000, 0x2000000000000000, 0x4000000000000000, 0x8000000000000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
		{0x8040201008040200, 0x80402010080400, 0x804020100800, 0x8040201000, 0x80402000, 0x804000, 0x8000, 0x0, 0x4020100804020000, 0x8040201008040000, 0x80402010080000, 0x804020100000, 0x8040200000, 0x80400000, 0x800000, 0x0, 0x2010080402000000, 0x4020100804000000, 0x8040201008000000, 0x80402010000000, 0x804020000000, 0x8040000000, 0x80000000, 0x0, 0x1008040200000000, 0x2010080400000000, 0x4020100800000000, 0x8040201000000000, 0x80402000000000, 0x804000000000, 0x8000000000, 0x0, 0x804020000000000, 0x1008040000000000, 0x2010080000000000, 0x4020100000000000, 0x8040200000000000, 0x80400000000000, 0x800000000000, 0x0, 0x402000000000000, 0x804000000000000, 0x1008000000000000, 0x2010000000000000, 0x4020000000000000, 0x8040000000000000, 0x80000000000000, 0x0, 0x200000000000000, 0x400000000000000, 0x800000000000000, 0x1000000000000000, 0x2000000000000000, 0x4000000000000000, 0x8000000000000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
		{0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x0, 0xfe00, 0xfc00, 0xf800, 0xf000, 0xe000, 0xc000, 0x8000, 0x0, 0xfe0000, 0xfc0000, 0xf80000, 0xf00000, 0xe00000, 0xc00000, 0x800000, 0x0, 0xfe000000, 0xfc000000, 0xf8000000, 0xf0000000, 0xe0000000, 0xc0000000, 0x80000000, 0x0, 0xfe00000000, 0xfc00000000, 0xf800000000, 0xf000000000, 0xe000000000, 0xc000000000, 0x8000000000, 0x0, 0xfe0000000000, 0xfc0000000000, 0xf80000000000, 0xf00000000000, 0xe00000000000, 0xc00000000000, 0x800000000000, 0x0, 0xfe000000000000, 0xfc000000000000, 0xf8000000000000, 0xf0000000000000, 0xe0000000000000, 0xc0000000000000, 0x80000000000000, 0x0, 0xfe00000000000000, 0xfc00000000000000, 0xf800000000000000, 0xf000000000000000, 0xe000000000000000, 0xc000000000000000, 0x8000000000000000, 0x0},
		{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x0, 0x204, 0x408, 0x810, 0x1020, 0x2040, 0x4080, 0x8000, 0x0, 0x20408, 0x40810, 0x81020, 0x102040, 0x204080, 0x408000, 0x800000, 0x0, 0x2040810, 0x4081020, 0x8102040, 0x10204080, 0x20408000, 0x40800000, 0x80000000, 0x0, 0x204081020, 0x408102040, 0x810204080, 0x1020408000, 0x2040800000, 0x4080000000, 0x8000000000, 0x0, 0x20408102040, 0x40810204080, 0x81020408000, 0x102040800000, 0x204080000000, 0x408000000000, 0x800000000000, 0x0, 0x2040810204080, 0x4081020408000, 0x8102040800000, 0x10204080000000, 0x20408000000000, 0x40800000000000, 0x80000000000000, 0x0},
		{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x101, 0x202, 0x404, 0x808, 0x1010, 0x2020, 0x4040, 0x8080, 0x10101, 0x20202, 0x40404, 0x80808, 0x101010, 0x202020, 0x404040, 0x808080, 0x1010101, 0x2020202, 0x4040404, 0x8080808, 0x10101010, 0x20202020, 0x40404040, 0x80808080, 0x101010101, 0x202020202, 0x404040404, 0x808080808, 0x1010101010, 0x2020202020, 0x4040404040, 0x8080808080, 0x10101010101, 0x20202020202, 0x40404040404, 0x80808080808, 0x101010101010, 0x202020202020, 0x404040404040, 0x808080808080, 0x1010101010101, 0x2020202020202, 0x4040404040404, 0x8080808080808, 0x10101010101010, 0x20202020202020, 0x40404040404040, 0x80808080808080},
		{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x0, 0x100, 0x201, 0x402, 0x804, 0x1008, 0x2010, 0x4020, 0x0, 0x10000, 0x20100, 0x40201, 0x80402, 0x100804, 0x201008, 0x402010, 0x0, 0x1000000, 0x2010000, 0x4020100, 0x8040201, 0x10080402, 0x20100804, 0x40201008, 0x0, 0x100000000, 0x201000000, 0x402010000, 0x804020100, 0x1008040201, 0x2010080402, 0x4020100804, 0x0, 0x10000000000, 0x20100000000, 0x40201000000, 0x80402010000, 0x100804020100, 0x201008040201, 0x402010080402, 0x0, 0x1000000000000, 0x2010000000000, 0x4020100000000, 0x8040201000000, 0x10080402010000, 0x20100804020100, 0x40201008040201},
		{0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0x0, 0x100, 0x300, 0x700, 0xf00, 0x1f00, 0x3f00, 0x7f00, 0x0, 0x10000, 0x30000, 0x70000, 0xf0000, 0x1f0000, 0x3f0000, 0x7f0000, 0x0, 0x1000000, 0x3000000, 0x7000000, 0xf000000, 0x1f000000, 0x3f000000, 0x7f000000, 0x0, 0x100000000, 0x300000000, 0x700000000, 0xf00000000, 0x1f00000000, 0x3f00000000, 0x7f00000000, 0x0, 0x10000000000, 0x30000000000, 0x70000000000, 0xf0000000000, 0x1f0000000000, 0x3f0000000000, 0x7f0000000000, 0x0, 0x1000000000000, 0x3000000000000, 0x7000000000000, 0xf000000000000, 0x1f000000000000, 0x3f000000000000, 0x7f000000000000, 0x0, 0x100000000000000, 0x300000000000000, 0x700000000000000, 0xf00000000000000, 0x1f00000000000000, 0x3f00000000000000, 0x7f00000000000000},
		{0x0, 0x100, 0x10200, 0x1020400, 0x102040800, 0x10204081000, 0x1020408102000, 0x102040810204000, 0x0, 0x10000, 0x1020000, 0x102040000, 0x10204080000, 0x1020408100000, 0x102040810200000, 0x204081020400000, 0x0, 0x1000000, 0x102000000, 0x10204000000, 0x1020408000000, 0x102040810000000, 0x204081020000000, 0x408102040000000, 0x0, 0x100000000, 0x10200000000, 0x1020400000000, 0x102040800000000, 0x204081000000000, 0x408102000000000, 0x810204000000000, 0x0, 0x10000000000, 0x1020000000000, 0x102040000000000, 0x204080000000000, 0x408100000000000, 0x810200000000000, 0x1020400000000000, 0x0, 0x1000000000000, 0x102000000000000, 0x204000000000000, 0x408000000000000, 0x810000000000000, 0x1020000000000000, 0x2040000000000000, 0x0, 0x100000000000000, 0x200000000000000, 0x400000000000000, 0x800000000000000, 0x1000000000000000, 0x2000000000000000, 0x4000000000000000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
	};

	const int aFile = get_file(a), bFile = get_file(b);

	if (aFile == bFile)
		return rayLookup[NORT][a] | rayLookup[SOUT][a] | bitmask[a];

	const int aRank = get_rank(a), bRank = get_rank(b);

	if (aRank == bRank)
		return rayLookup[EAST][a] | rayLookup[WEST][a] | bitmask[a];

	// Right diagonal
	if (aRank - aFile == bRank - bFile)
	   return rayLookup[NOEA][a] | rayLookup[SOWE][a] | bitmask[a];

	// Left diagonal
	if (aRank + aFile == bRank + bFile)
		return rayLookup[NOWE][a] | rayLookup[SOEA][a] | bitmask[a];

	return 0;
}



// AUX

const char *sqrToCoord(int sq) {
	static const char* s[64] = {
		"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
	};

	return s[sq];
}

int coordToSqr(char *coord) {
	return (coord[0] - 'a') + (coord[1] - '1') * 8;
}

int charToPiece(char val) {
	int piece = -1;

	switch (val) {
	case 'P': case 'p':
		piece = PAWN;
		break;
	case 'N': case 'n':
		piece = KNIGHT;
		break;
	case 'B': case 'b':
		piece = BISHOP;
		break;
	case 'R': case 'r':
		piece = ROOK;
		break;
	case 'Q': case 'q':
		piece = QUEEN;
		break;
	case 'K': case 'k':
		piece = KING;
		break;
	}

	return piece;
}
