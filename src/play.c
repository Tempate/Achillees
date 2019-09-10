#include "board.h"
#include "play.h"
#include "hashtables.h"


static void setBits  (Board *board, const int color, const int piece, const int index);
static void unsetBits(Board *board, const int color, const int piece, const int index);

static void checkCapture(Board *board, History *history, const int index, const int color);
static void removeCastlingForRook(Board *board, const int index, const int color);

const int castleLookup[4][3] = {{6, 7, 5}, {2, 0, 3}, {62, 63, 61}, {58, 56, 59}};


void makeMove(Board *board, const Move *move, History *history) {
	static const int removeCastling[2] = {12, 3};
	const int color = move->color, opcolor = 1 ^ color;

	history->castling = board->castling;
	history->enPassant = board->enPassant;
	history->fiftyMoves = board->fiftyMoves;
	history->capture = -1;

	unsetBits(board, color, move->piece, move->from);

	switch (move->piece) {
	case PAWN:
		if (board->enPassant && move->to == board->enPassant) {
			unsetBits(board, opcolor, PAWN, move->to - 8 + 16*color);
			setBits(board, color, PAWN, move->to);
		} else if (move->promotion) {
			setBits(board, color, move->promotion, move->to);
			checkCapture(board, history, move->to, opcolor);
		} else {
			setBits(board, color, PAWN, move->to);
			checkCapture(board, history, move->to, opcolor);
		}

		board->fiftyMoves = 0;

		break;
	case KING:
		
		board->kingIndex[color] = move->to;
		board->castling &= removeCastling[color];	// WHITE: 1100   BLACK: 0011

		/*
		 * The fifty-move counter is increased automatically on castles because
		 * there could have been no capture.
		 */

		if (move->castle != -1) {
			const int castle = bitScanForward(move->castle);
			setBits  (board, color, KING, castleLookup[castle][0]);
			unsetBits(board, color, ROOK, castleLookup[castle][1]);
			setBits  (board, color, ROOK, castleLookup[castle][2]);

			++(board->fiftyMoves);
		} else {
			setBits(board, color, KING, move->to);
			checkCapture(board, history, move->to, opcolor);
		}

		break;
	case ROOK:
		removeCastlingForRook(board, move->from, color);
		/* no break */
	default:
		setBits(board, color, move->piece, move->to);
		checkCapture(board, history, move->to, opcolor);
	}

	updateOccupancy(board);

	board->enPassant = move->enPassant;

	++(board->ply);
	board->turn ^= 1;
	board->opponent ^= 1;
}

void undoMove(Board *board, const Move *move, const History *history) {
	const int color = move->color, opcolor = 1 ^ color;

	board->castling = history->castling;
	board->enPassant = history->enPassant;
	board->fiftyMoves = history->fiftyMoves;

	setBits(board, color, move->piece, move->from);

	switch (move->piece) {
	case PAWN:
		if (board->enPassant && move->to == board->enPassant) {
			// Adds the pawn captured en passant
			setBits(board, opcolor, PAWN, move->to - 8 + 16*color);
			unsetBits(board, color, PAWN, move->to);
			break;
		} else if (move->promotion) {
			// Removes the promoted piece
			unsetBits(board, color, move->promotion, move->to);
			goto CAPTURE;
		} else {
			unsetBits(board, color, PAWN, move->to);
			goto CAPTURE;
		}
	case KING:
		
		board->kingIndex[color] = move->from;
		
		if (move->castle != -1) {
			const int castle = bitScanForward(move->castle);
			unsetBits(board, color, KING, castleLookup[castle][0]);
			setBits  (board, color, ROOK, castleLookup[castle][1]);
			unsetBits(board, color, ROOK, castleLookup[castle][2]);
			break;
		}

		/* no break */
	default:
		unsetBits(board, color, move->piece, move->to);

		CAPTURE:
		if (history->capture != -1)
			setBits(board, opcolor, history->capture, move->to);
	}

	updateOccupancy(board);

	--(board->ply);
	board->turn ^= 1;
	board->opponent ^= 1;
}

void makeNullMove(Board *board, History *history) {
	history->fiftyMoves = board->fiftyMoves;
	history->enPassant  = board->enPassant;

	board->fiftyMoves = 0;
	board->enPassant = 0;

	board->turn ^= 1;
	board->opponent ^= 1;

	updateNullMoveKey(board);
}

void undoNullMove(Board *board, const History *history) {
	board->fiftyMoves = history->fiftyMoves;
	board->enPassant  = history->enPassant;

	board->turn ^= 1;
	board->opponent ^= 1;

	updateNullMoveKey(board);
}

int findPiece(const Board *board, const uint64_t sqrBB, const int color) {
	for (int piece = PAWN; piece <= KING; ++piece) {
		if (sqrBB & board->pieces[color][piece])
			return piece;
	}

	return -1;
}


// AUX

static void setBits(Board *board, const int color, const int piece, const int index) {
	// Sets the bit on the general board for that player
	setBit(&board->players[color], index);

	// Sets the bit on the specific board for that piece
	setBit(&board->pieces[color][piece], index);
}

static void unsetBits(Board *board, const int color, const int piece, const int index) {
	// Unsets the bit on the general board for that player
	unsetBit(&board->players[color], index);

	// Unsets the bit on the specific board for that piece
	unsetBit(&board->pieces[color][piece], index);
}

static void checkCapture(Board *board, History *history, const int index, const int color) {
	const uint64_t toBB = bitmask[index];

	if (toBB & board->players[color]) {
		history->capture = findPiece(board, toBB, color);
		ASSERT(history->capture != -1);

		unsetBits(board, color, history->capture, index);

		if (history->capture == ROOK)
			removeCastlingForRook(board, index, color);

		board->fiftyMoves = 0;
	} else {
		++(board->fiftyMoves);
	}
}

static void removeCastlingForRook(Board *board, const int index, const int color) {
	static const int removeCastling[2][2] = {{14, 11}, {13, 7}};

	if (index == 56*color) {
		board->castling &= removeCastling[1][color];  // WHITE: 1101   BLACK: 0111
	} else if (index == 56*color + 7) {
		board->castling &= removeCastling[0][color];  // WHITE: 1110   BLACK: 1011
	}
}
