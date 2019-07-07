#ifndef SRC_SEARCH_H_
#define SRC_SEARCH_H_

#define MAX_DEPTH 50
#define DEF_DEPTH 5

enum {EXACT, HIGHER_BOUND, LOWER_BOUND};

Move search(Board *board);


#endif /* SRC_SEARCH_H_ */
