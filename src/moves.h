#ifndef SRC_MOVES_H_
#define SRC_MOVES_H_


typedef struct {
	int from;
	int to;

	int piece;
	int color;

	int castle;
	int promotion;
} Move;

typedef struct {
	int castling;
	int enPassant;
	int capture;
	int fiftyMoves;
} History;

int legalMoves(Board board, Move *moves);
int kingInCheck(Board board, uint64_t kingBB, int color);


#endif /* SRC_MOVES_H_ */
