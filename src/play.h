#ifndef SRC_PLAY_H_
#define SRC_PLAY_H_


long perft(Board board, const int depth);

void makeMove(Board *board, Move move, History *history);
void undoMove(Board *board, Move move, History history);

int findPiece(Board board, uint64_t toBB, int color);


#endif /* SRC_PLAY_H_ */
