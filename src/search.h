#ifndef SRC_SEARCH_H_
#define SRC_SEARCH_H_

#define MAX_DEPTH 63
#define DEF_DEPTH 5

#define INFINITY 2 * MAX_SCORE
#define R 2

enum {EXACT, UPPER_BOUND, LOWER_BOUND};

typedef struct {
	uint64_t nodes;

	int instCutoffs;
	int betaCutoffs;

	int ttHits;
} Stats;

Move search(Board *board);

int pvSearch(Board *board, int depth, int alpha, int beta, const int nullmove);

#endif /* SRC_SEARCH_H_ */
