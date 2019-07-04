/*
 * tests.h
 *
 *  Created on: Jun 18, 2019
 *      Author: alpha
 */

#ifndef SRC_TESTS_H_
#define SRC_TESTS_H_


void testMakeMove(char *fen);
void testMoves(char *fen, const int depth);
void testPerft(const char* filename, const int depth);

void testKeys(void);

#endif /* SRC_TESTS_H_ */
