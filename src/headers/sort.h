/*
 * sort.h
 *
 *  Created on: Aug 21, 2019
 *      Author: alpha
 */

#ifndef SRC_SORT_H_
#define SRC_SORT_H_

void sort(Board *board, Move *moves, const int nMoves);

void initKillerMoves(void);
void saveKillerMove(const Move *move, const int ply);

const int see(Board *board, const int sqr);
const int seeCapture(Board *board, const Move *move);

void sortByEval(Board *board, Move *moves, const int nMoves);

#endif /* SRC_SORT_H_ */
