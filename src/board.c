#include <string.h>
#include <math.h>
#include <assert.h>

#include "main.h"
#include "board.h"
#include "play.h"
#include "hashtables.h"


static const char *sqToCoord(int sq);
static int coordToSq(char *coord);

static int charToPiece(char val);


const char pieceChars[12] = {'P','N','B','R','Q','K','p','n','b','r','q','k'};


// BOARD

Board blankBoard() {
	Board board = (Board) {
		.empty = ~0,
		.occupied = 0
	};

	memset(board.players, 0, 2*sizeof(int));
	memset(board.pieces[WHITE], 0, PIECES*sizeof(uint64_t));
	memset(board.pieces[BLACK], 0, PIECES*sizeof(uint64_t));

	return board;
}

void printBoard(const Board *board) {
	uint64_t bb;
	int color;

	for (int y = RANKS-1; y >= 0; y--) {
		for (int x = 0; x < FILES; x++) {
			bb = get_sqr(x,y);

			if (board->empty & bb) {
				printf(". ");
			} else {
				color = (board->players[WHITE] & bb) ? WHITE : BLACK;
				for (int j = 0; j < PIECES; j++) {
					if (board->pieces[color][j] & bb) {
						printf("%c ", pieceChars[j + 6*color]);
						break;
					}
				}
			}
		}

		printf("\n");
	}

	printf("\n");
	printf("Turn: %s\n", (board->turn == WHITE) ? "White" : "Black");
	printf("Castling: ");

	if (board->castling) {
		if (board->castling & KCastle) printf("K");
		if (board->castling & QCastle) printf("Q");
		if (board->castling & kCastle) printf("k");
		if (board->castling & qCastle) printf("q");
	} else {
		printf("-");
	}

	printf("\n");
	printf("En passant: %s\n", (board->enPassant > 0) ? sqToCoord(board->enPassant) : "-");
	printf("Fifty Moves: %d\n", board->fiftyMoves);
	printf("Ply: %d\n\n", board->ply);
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

void setBits(Board *board, const int color, const int piece, const int index) {
	// Sets the bit on the general board for that player
	setBit(&board->players[color], index);

	// Sets the bit on the specific board for that piece
	setBit(&board->pieces[color][piece], index);
}

void unsetBits(Board *board, const int color, const int piece, const int index) {
	// Unsets the bit on the general board for that player
	unsetBit(&board->players[color], index);

	// Unsets the bit on the specific board for that piece
	unsetBit(&board->pieces[color][piece], index);
}


// MOVES

void printMoves(Move *moves, int n) {
	for (int i = 0; i < n; ++i)
		printMove(moves[i], 0);

	printf("\nTotal: %d\n", n);
}

void printMove(Move move, int nodes) {
	char *text = malloc(6);

	moveToText(move, text);

	if (nodes) {
		printf("%s \t %d\n", text, nodes);
	} else {
		printf("%s\n", text);
	}

	free(text);
}

void moveToText(Move move, char *text) {
	static const char pieceNames[6] = {'p', 'n', 'b', 'r', 'q', 'k'};
	static const uint64_t promotion = 0xff000000000000ff;

	if (move.piece == PAWN && (pow2[move.to] & promotion)) {
		snprintf(text, 6, "%s%s%c", sqToCoord(move.from), sqToCoord(move.to), pieceNames[move.promotion]);
	} else {
		snprintf(text, 5, "%s%s", sqToCoord(move.from), sqToCoord(move.to));
	}
}

Move textToMove(const Board *board, char *text) {
	Move move;
	uint64_t fromBB;

	int diff, castle;

	move.from = coordToSq(text);
	move.to = coordToSq(text + 2);

	fromBB = pow2[move.from];
	move.color = (fromBB & board->players[WHITE]) ? WHITE : BLACK;

	if (text[4] >= 'a' && text[4] <= 'z') {
		move.piece = PAWN;
		move.promotion = charToPiece(text[4]);
	} else {
		move.piece = findPiece(board, fromBB, move.color);
		move.promotion = 0;

		if (move.piece == KING) {
			diff = get_file(move.to) - get_file(move.from);

			// Castle
			if (abs(diff) == 2) {
				castle = (diff == 2) ? 1 : 2;
				move.castle = castle << 2*move.color;
			} else {
				move.castle = -1;
			}
		}
	}

	return move;
}


// FEN

// Returns the fens length
int parseFen(Board *board, char *fen) {
	int rank = RANKS-1, file = 0, piece;
	uint64_t sqr;
	int i = 0;

	*board = blankBoard();

	while (fen[i] != ' ') {
		if (fen[i] == '/') {
			--rank;
			file = 0;
		} else if (fen[i] >= '1' && fen[i] <= '8') {
			file += fen[i] - '0';
		} else {
			piece = (int) charToPiece(fen[i]);
			sqr = get_sqr(file, rank);

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

	i += 2;
	if (fen[i] == '-') {
		board->castling = 0;
		++i;
	} else {
		if (fen[i] == 'K') { board->castling += pow2[0]; ++i; }
		if (fen[i] == 'Q') { board->castling += pow2[1]; ++i; }
		if (fen[i] == 'k') { board->castling += pow2[2]; ++i; }
		if (fen[i] == 'q') { board->castling += pow2[3]; ++i; }
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
	initializeTT();

	return i;
}

void generateFen(const Board *board, char *fen) {
	uint64_t sqr;
	int k = -1, blanks;

	for (int y = RANKS-1; y >= 0; y--) {
		blanks = 0;

		for (int x = 0; x < FILES; x++) {
			sqr = get_sqr(x,y);

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
		if (board->castling & pow2[0]) fen[++k] = 'K';
		if (board->castling & pow2[1]) fen[++k] = 'Q';
		if (board->castling & pow2[2]) fen[++k] = 'k';
		if (board->castling & pow2[3]) fen[++k] = 'q';
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


// Bitboards

void printBB(const uint64_t bb) {
	uint64_t sqr;

	printf("----------------\n");

	for (int y = 7; y >= 0; y--) {
		for (int x = 0; x < 8; x++) {
			sqr = get_sqr(x,y);

			printf("%d ", (bb & sqr) ? 1 : 0);
		}

		printf("\n");
	}

	printf("----------------\n");
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



// AUX

static const char *sqToCoord(int sq) {
	static const char* s[] = {"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"};

	return s[sq];
}

static int coordToSq(char *coord) {
	return (coord[0] - 'a') + (coord[1] - '1') * 8;
}

static int charToPiece(char val) {
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
