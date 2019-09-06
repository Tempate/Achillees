#ifndef SRC_PLAY_H_
#define SRC_PLAY_H_


extern const int castleLookup[4][3];

void makeMove(Board *board, const Move *move, History *history);
void undoMove(Board *board, const Move *move, const History *history);

void makeShallowMove(Board *board, const Move *move, const int capturedPiece, const int captureSqr);

void makeNullMove(Board *board, History *history);
void undoNullMove(Board *board, const History *history);

int findPiece(const Board *board, const uint64_t sqrBB, const int color);


#endif /* SRC_PLAY_H_ */
