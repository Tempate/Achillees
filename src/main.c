#include <string.h>

#include "headers/board.h"
#include "headers/tests.h"
#include "headers/eval.h"
#include "headers/uci.h"
#include "headers/magic.h"

#define INITIAL "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

static void listen(void);
static void initInBetween(void);

uint64_t square[64] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000, 0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x200000, 0x400000, 0x800000, 0x1000000, 0x2000000, 0x4000000, 0x8000000, 0x10000000, 0x20000000, 0x40000000, 0x80000000, 0x100000000, 0x200000000, 0x400000000, 0x800000000, 0x1000000000, 0x2000000000, 0x4000000000, 0x8000000000, 0x10000000000, 0x20000000000, 0x40000000000, 0x80000000000, 0x100000000000, 0x200000000000, 0x400000000000, 0x800000000000, 0x1000000000000, 0x2000000000000, 0x4000000000000, 0x8000000000000, 0x10000000000000, 0x20000000000000, 0x40000000000000, 0x80000000000000, 0x100000000000000, 0x200000000000000, 0x400000000000000, 0x800000000000000, 0x1000000000000000, 0x2000000000000000, 0x4000000000000000, 0x8000000000000000};
uint64_t inBetweenLookup[64][64];

Settings settings;

int main(void) {

	initMagics();
	initInBetween();

	listen();

	return EXIT_SUCCESS;
}

static void listen(void) {

	Board *board = malloc(sizeof(Board));
	char *msg = malloc(4096);

	initialBoard(board);

	while (1) {
		fprintf(stdout, "Achillees: ");
		fflush(stdout);

		char *r;
		r = fgets(msg, 4096, stdin);

		if (r == NULL) {
			fprintf(stdout, "There has been an error with the input. Quitting.\n");
			fflush(stdout);
			break;
		}

		if (strncmp(msg, "uci", 3) == 0) {
			uci(); break;
		} else if (strncmp(msg, "newboard", 8) == 0)
			initialBoard(board);
		else if (strncmp(msg, "position", 8) == 0)
			parseFen(board, msg + 9);
		else if (strncmp(msg, "moves", 5) == 0)
			moves(board, msg + 6);
		else if (strncmp(msg, "print", 5) == 0)
			printBoard(board);
		else if (strncmp(msg, "fen", 3) == 0) {
			char fen[256];
			generateFen(board, fen);

			fprintf(stdout, "\n%s\n\n", fen);
		} else if (strncmp(msg, "eval", 4) == 0) {
			fprintf(stdout, "\n");

			evaluate(board);

			fprintf(stdout, "\n");
		} else if (strncmp(msg, "depth", 5) == 0) {
			fprintf(stdout, "\n");

			settings.depth = atoi(msg + 6);
			bestmove(board);

			fprintf(stdout, "\n");
		} else if (strncmp(msg, "perft", 5) == 0)
			testPerft(board, atoi(msg + 6));
		else if (strncmp(msg, "test perft", 10) == 0)
			testPerftFile(atoi(msg + 11));
		else if (strncmp(msg, "test keys", 9) == 0)
			testKeys();
		else if (strncmp(msg, "test draw", 9) == 0)
			testDraw();
		else if (strncmp(msg, "test see", 8) == 0)
			testSee();
		else if (strncmp(msg, "quit", 4) == 0)
			break;
		else if (strncmp(msg, "test", 4) == 0) {
			fprintf(stdout, "\n"
					"test perft <depth>     tests perft from the specified depth [4/5/6]\n"
					"test keys            tests if Zobrist keys and how they are updated is working\n"
					"test draw            tests if draw checking is working\n"
					"test see             tests if the SEE is working\n\n");
		} else {
			fprintf(stdout, "\n"
					"uci                  switches to uci mode\n"
					"newboard             sets the board to the starting position\n"
					"position <fen>       sets the board from a given fen\n"
					"moves <moves>        makes a set of moves to the current board\n"
					"fen                  prints the fen from the current board\n"
					"print                prints the current board\n"
					"eval                 evaluates the current board\n"
					"depth <depth>        searches the current board to the given depth\n"
					"perft <depth>        counts the number of moves for the current board\n"
					"test                 shows test menu\n"
					"test <id>            runs a test by id\n"
					"help                 shows this menu\n"
					"quit                 terminates the program\n\n");
		}

		fflush(stdout);
	}

	free(board);
}

void defaultSettings(Settings *settings) {
	settings->stop = 0;
	settings->depth = MAX_DEPTH;
	settings->nodes = 0;
	settings->mate = 0;
	settings->wtime = 0;
	settings->btime = 0;
	settings->winc = 0;
	settings->binc = 0;
	settings->movestogo = 20;
	settings->movetime = 0;
}

void evaluate(const Board *board) {
	int score = eval(board);

	if (board->turn == BLACK)
		score = -score;

	fprintf(stdout, "%d\n", score);
	fflush(stdout);
}

static void initInBetween(void) {
	for (int i = 0; i < 64; ++i) {
		for (int j = 0; j < 64; ++j) {
			if (i == j)
				continue;

			inBetweenLookup[i][j] = 0;

			if (i % 8 == j % 8 || i / 8 == j / 8) {
				inBetweenLookup[i][j] = rookAttacks(i, square[j]) & rookAttacks(j, square[i]);
				continue;
			}

			if (abs(j % 8 - i % 8) == abs(j / 8 - i / 8)) {
				inBetweenLookup[i][j] = bishopAttacks(i, square[j]) & bishopAttacks(j, square[i]);;
				continue;
			}


		}
	}
}
