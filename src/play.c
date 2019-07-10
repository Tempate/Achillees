#include "board.h"
#include "play.h"
#include "hashtables.h"


void checkCapture(Board *board, History *history, int index, int color);
void removeCastlingForRook(Board *board, int index, int color);

const int castleLookup[4][3] = {{6, 7, 5}, {2, 0, 3}, {62, 63, 61}, {58, 56, 59}};


long perft(Board *board, const int depth) {
	Move moves[218];
	History history;

	int k, newDepth;
	long nodes = 0;

	if (depth == 1) {
		nodes = legalMoves(board, moves);
	} else {
		newDepth = depth - 1;
		k = legalMoves(board, moves);

		for (int i = 0; i < k; ++i) {
			makeMove(board, &moves[i], &history);
			nodes += perft(board, newDepth);
			undoMove(board, &moves[i], &history);
		}
	}

	return nodes;
}


void makeMove(Board *board, const Move *move, History *history) {
	static const int removeCastling[2] = {12, 3};
	const int color = move->color, opColor = 1 ^ color;

	int castle;

	history->castling = board->castling;
	history->enPassant = board->enPassant;
	history->fiftyMoves = board->fiftyMoves;
	history->capture = -1;

	unsetBits(board, color, move->piece, move->from);

	switch (move->piece) {
	case PAWN:
		if (board->enPassant && move->to == board->enPassant) {
			unsetBits(board, opColor, PAWN, move->to - 8 + 16*color);
			setBits(board, color, PAWN, move->to);
		} else if (move->promotion) {
			setBits(board, color, move->promotion, move->to);
			checkCapture(board, history, move->to, opColor);
		} else {
			setBits(board, color, PAWN, move->to);
			checkCapture(board, history, move->to, opColor);
		}

		if (abs(move->to - move->from) == 16) {
			board->enPassant = move->from + 8 - 16 * color;
		} else {
			board->enPassant = 0;
		}

		board->fiftyMoves = 0;

		break;
	case KING:
		board->castling &= removeCastling[color];	// WHITE: 1100   BLACK: 0011
		board->enPassant = 0;

		if (move->castle != -1) {
			++(board->fiftyMoves);
			castle = bitScanForward(move->castle);
			setBits  (board, color, KING, castleLookup[castle][0]);
			unsetBits(board, color, ROOK, castleLookup[castle][1]);
			setBits  (board, color, ROOK, castleLookup[castle][2]);
		} else {
			setBits(board, color, KING, move->to);
			checkCapture(board, history, move->to, opColor);
		}

		break;
	case ROOK:
		removeCastlingForRook(board, move->from, color);
		/* no break */
	default:
		board->enPassant = 0;
		setBits(board, color, move->piece, move->to);
		checkCapture(board, history, move->to, opColor);
	}

	updateOccupancy(board);

	++(board->ply);
	board->turn = 1 - board->turn;
}

void undoMove(Board *board, const Move *move, const History *history) {
	const int color = move->color, opColor = 1 ^ color;
	int castle;

	board->castling = history->castling;
	board->enPassant = history->enPassant;
	board->fiftyMoves = history->fiftyMoves;

	setBits(board, color, move->piece, move->from);

	switch (move->piece) {
	case PAWN:
		if (board->enPassant && move->to == board->enPassant) {
			// Adds the pawn captured en passant
			setBits(board, opColor, PAWN, move->to - 8 + 16*color);
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
		if (move->castle != -1) {
			castle = bitScanForward(move->castle);
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
			setBits(board, opColor, history->capture, move->to);
	}

	updateOccupancy(board);

	--(board->ply);
	board->turn ^= 1;
}

void makeNullMove(Board *board, History *history) {
	history->fiftyMoves = board->fiftyMoves;
	history->enPassant  = board->enPassant;

	board->turn ^= 1;

	updateNullMoveKey(board);
}

void undoNullMove(Board *board, History *history) {
	board->fiftyMoves = history->fiftyMoves;
	board->enPassant  = history->enPassant;

	updateNullMoveKey(board);

	board->turn ^= 1;
}

void checkCapture(Board *board, History *history, int index, int color) {
	uint64_t toBB = pow2[index];

	if (toBB & board->players[color]) {
		history->capture = findPiece(board, toBB, color);
		unsetBits(board, color, history->capture, index);

		if (history->capture == ROOK)
			removeCastlingForRook(board, index, color);

		board->fiftyMoves = 0;
	} else {
		++(board->fiftyMoves);
	}
}

int findPiece(const Board *board, uint64_t toBB, int color) {
	for (int i = 0; i < PIECES; ++i) {
		if (toBB & board->pieces[color][i])
			return i;
	}

	return -1;
}

void removeCastlingForRook(Board *board, int index, int color) {
	static const int removeCastling[2][2] = {{14, 11}, {13, 7}};

	if (index == 56*color) {
		board->castling &= removeCastling[1][color];  // WHITE: 1101   BLACK: 0111
	} else if (index == 56*color + 7) {
		board->castling &= removeCastling[0][color];  // WHITE: 1110   BLACK: 1011
	}
}
