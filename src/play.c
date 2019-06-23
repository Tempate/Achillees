#include "board.h"
#include "play.h"


void checkCapture(Board *board, History *history, int index, int color);
int findPiece(Board board, uint64_t toBB, int color);
void removeCastlingForRook(Board *board, int index, int color);

static const int castleLookup[4][3] = {{6, 7, 5}, {2, 0, 3}, {62, 63, 61}, {58, 56, 59}};


long perft(Board board, const int depth) {
	Move moves[218];
	History history;

	int k, nMoves, newDepth;
	long nodes = 0;

	if (depth == 1) {
		nodes = legalMoves(board, moves);
	} else {
		newDepth = depth - 1;
		k = legalMoves(board, moves);

		for (int i = 0; i < k; i++) {
			makeMove(&board, moves[i], &history);

			nMoves = perft(board, newDepth);
			nodes += nMoves;

			undoMove(&board, moves[i], history);
		}
	}

	return nodes;
}


void makeMove(Board *board, Move move, History *history) {
	const int color = move.color, opColor = 1 - color;
	int enPassant, castle;

	history->castling = board->castling;
	history->enPassant = board->enPassant;
	history->fiftyMoves = board->fiftyMoves;
	history->capture = -1;

	unsetBits(board, color, move.piece, move.from);

	switch (move.piece) {
	case PAWN:
		// Updates the enPassant
		enPassant = board->enPassant;
		board->enPassant = (abs(move.to - move.from) == 16) ? move.from + 8 - 16*color : 0;

		if (move.to == enPassant && enPassant) {
			// Removes a pawn captured en passant
			unsetBits(board, opColor, PAWN, move.to - 8 + 16*color);
			// Move the pawn to the en passant square
			setBits(board, color, PAWN, move.to);
		} else if (move.promotion) {
			// Sets the bit for the new piece
			setBits(board, color, move.promotion, move.to);
			checkCapture(board, history, move.to, opColor);
		} else {
			setBits(board, color, PAWN, move.to);
			checkCapture(board, history, move.to, opColor);
		}

		board->fiftyMoves = 0;

		break;
	case KING:
		// Removes castling for the kings color
		board->castling &= 12 >> 2*color;            // WHITE: 1100   BLACK: 0011
		board->enPassant = 0;

		if (move.castle != -1) {
			castle = bitScanForward(move.castle);
			setBits  (board, color, KING, castleLookup[castle][0]);
			unsetBits(board, color, ROOK, castleLookup[castle][1]);
			setBits  (board, color, ROOK, castleLookup[castle][2]);
			++(board->fiftyMoves);
		} else {
			setBits(board, color, KING, move.to);
			checkCapture(board, history, move.to, opColor);
		}

		break;
	case ROOK:
		removeCastlingForRook(board, move.from, color);
		/* no break */
	default:
		board->enPassant = 0;

		setBits(board, color, move.piece, move.to);
		checkCapture(board, history, move.to, opColor);
	}

	updateOccupancy(board);

	++(board->ply);
	board->turn = 1 - board->turn;
}

void undoMove(Board *board, Move move, History history) {
	const int color = move.color, opColor = 1 - color;
	int castle;

	board->castling = history.castling;
	board->enPassant = history.enPassant;
	board->fiftyMoves = history.fiftyMoves;

	setBits(board, color, move.piece, move.from);

	switch (move.piece) {
	case PAWN:
		if (move.to == board->enPassant && board->enPassant) {
			// Adds the pawn captured en passant
			setBits(board, opColor, PAWN, move.to - 8 + 16*color);
			unsetBits(board, color, PAWN, move.to);
			break;
		} else if (move.promotion) {
			// Removes the promoted piece
			unsetBits(board, color, move.promotion, move.to);
			goto CAPTURE;
		} else {
			unsetBits(board, color, PAWN, move.to);
			goto CAPTURE;
		}
	case KING:
		if (move.castle != -1) {
			castle = bitScanForward(move.castle);
			unsetBits(board, color, KING, castleLookup[castle][0]);
			setBits  (board, color, ROOK, castleLookup[castle][1]);
			unsetBits(board, color, ROOK, castleLookup[castle][2]);
			break;
		}
		/* no break */
	default:
		unsetBits(board, color, move.piece, move.to);

		CAPTURE:
		if (history.capture != -1)
			setBits(board, opColor, history.capture, move.to);
	}

	updateOccupancy(board);

	--(board->ply);
	board->turn = 1 - board->turn;
}

void checkCapture(Board *board, History *history, int index, int color) {
	uint64_t toBB = pow2[index];

	if (toBB & board->players[color]) {
		history->capture = findPiece(*board, toBB, color);
		unsetBits(board, color, history->capture, index);

		if (history->capture == ROOK)
			removeCastlingForRook(board, index, color);

		board->fiftyMoves = 0;
	} else {
		++(board->fiftyMoves);
	}
}

int findPiece(Board board, uint64_t toBB, int color) {
	for (int i = 0; i < PIECES; i++) {
		if (toBB & board.pieces[color][i])
			return i;
	}
	return -1;
}

void removeCastlingForRook(Board *board, int index, int color) {
	if (index == 56*color) {
		board->castling &= 13 - 6*color;  // WHITE: 1101   BLACK: 0111
	} else if (index == 56*color + 7) {
		board->castling &= 14 - 3*color;  // WHITE: 1110   BLACK: 1011
	}
}
