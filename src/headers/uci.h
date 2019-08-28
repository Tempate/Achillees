#ifndef SRC_UCI_H_
#define SRC_UCI_H_

#include "search.h"

void uci(void);

void infoString(const Board *board, PV *pv, const int score, const int depth, const long time, const uint64_t nodes);


#endif /* SRC_UCI_H_ */
