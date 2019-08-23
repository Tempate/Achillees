#ifndef SRC_SEARCH_H_
#define SRC_SEARCH_H_

#define MAX_DEPTH 50
#define DEF_DEPTH 5

#define INFINITY 2 * MAX_SCORE
#define R 2

enum {EXACT, UPPER_BOUND, LOWER_BOUND};

typedef struct {
	Move moves[MAX_DEPTH];
	int count;
} PV;

typedef struct {
	uint64_t nodes;

	int instCutoffs;
	int betaCutoffs;
} Stats;

Move search(Board *board);

const int alphabeta(Board *board, int depth, int alpha, int beta, const int nullmove);

#endif /* SRC_SEARCH_H_ */
