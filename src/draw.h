/*
 * draw.h
 *
 *  Created on: Jul 7, 2019
 *      Author: alpha
 */

#ifndef SRC_DRAW_H_
#define SRC_DRAW_H_

#include "main.h"
#include "eval.h"


typedef struct {
	uint64_t keys[MAX_GAME_LENGTH];
	int size;
} Memory;


int isDraw(const Board *board);

void saveKeyToMemory(const uint64_t key);
void freeKeyFromMemory(void);

#endif /* SRC_DRAW_H_ */
