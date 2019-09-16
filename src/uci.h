#ifndef SRC_UCI_H_
#define SRC_UCI_H_

#include "search.h"

void uci(void);

void infoString(const Board *board, const int depth, const int score, const uint64_t nodes, const int duration, Move *pv, const int nPV);

void playMoves(Board *board, char *moves);

void *bestmove(void *args);

void createSearchThread(Board *board);
void stopSearchThread(void);

#endif /* SRC_UCI_H_ */
