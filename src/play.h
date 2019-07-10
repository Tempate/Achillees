#ifndef SRC_PLAY_H_
#define SRC_PLAY_H_


extern const int castleLookup[4][3];

long perft(Board *board, const int depth);

void makeMove(Board *board, const Move *move, History *history);
void undoMove(Board *board, const Move *move, const History *history);

int findPiece(const Board *board, uint64_t toBB, int color);

void makeNullMove(Board *board, History *history);
void undoNullMove(Board *board, History *history);


#endif /* SRC_PLAY_H_ */
