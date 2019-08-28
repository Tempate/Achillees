/*
 * tests.h
 *
 *  Created on: Jun 18, 2019
 *      Author: alpha
 */

#ifndef SRC_TESTS_H_
#define SRC_TESTS_H_


void testMakeMove(char *fen);

void testPerft(Board *board, const int depth);
void testPerftFile(const char* filename, const int depth);

void testKeys(void);

void testDraw(void);

void testSearch(Board *board, const int depth);
void testPosition(char* fen, const int depth);

void testSee(void);

#endif /* SRC_TESTS_H_ */
