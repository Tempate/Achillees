/*
 * sort.h
 *
 *  Created on: Aug 21, 2019
 *      Author: alpha
 */

#ifndef SRC_SORT_H_
#define SRC_SORT_H_

void sort(Board *board, Move *moves, const int nMoves);
void sortAB(Board *board, Move *moves, const int nMoves, const int depth, const int alpha, const int beta, const int nullmove);

void initKillerMoves(void);
void saveKillerMove(const Move *move, const int ply);

int see(Board *board, const int sqr);
int seeCapture(Board *board, const Move *move);

#endif /* SRC_SORT_H_ */
