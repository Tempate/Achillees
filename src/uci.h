#ifndef SRC_UCI_H_
#define SRC_UCI_H_


typedef struct {
	int stop;

	int depth;
	int nodes;
	int mate;

	int wtime;
	int btime;
	int winc;
	int binc;

	int movestogo;
	int movetime;
} Settings;

extern Settings settings;

void listen(void);


#endif /* SRC_UCI_H_ */
