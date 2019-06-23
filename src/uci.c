#include <string.h>
#include <time.h>

#include "main.h"
#include "board.h"
#include "play.h"

#define MAX_DEPTH 6


static void isready(void);
static void ucinewgame(Board *board);
static void position(Board *board, char *s);
static void go(Settings *settings, char *s);
static void perftUntilDepth(Board board, const int depth);


void listen(void) {
	Board *board = malloc(sizeof(Board));
	Settings settings;

	char message[4096], *r, *part;
	int depth, quit = 0;

	/*
	printf("id name %s\n", ENGINE_NAME);
	printf("id author %s\n", ENGINE_AUTHOR);
	printf("uciok\n");
	*/

	/*
	while (1) {
		r = fgets(message, 4096, stdin);
		if (r == NULL) break;
		part = message;

		if (strncmp(part, "quit", 4) == 0) {
			return;
		} else if (strncmp(part, "isready", 7) == 0) {
			isready(); break;
		} else if (strncmp(part, "ucinewgame", 10) == 0) {
			ucinewgame(&board); break;
		}
	}
	*/

	while (!quit) {
		r = fgets(message, 4096, stdin);

		if (r == NULL)
			break;

		part = message;

		if (strncmp(part, "isready", 7) == 0) {
			isready();
		} else if (strncmp(part, "ucinewgame", 10) == 0) {
			ucinewgame(board);
		} else if (strncmp(part, "position", 8) == 0) {
			position(board, part + 9);
		} else if (strncmp(part, "go", 2) == 0) {
			go(&settings, part);
		} else if (strncmp(part, "stop", 4) == 0) {
			settings.stop = 1;
		} else if (strncmp(part, "print", 5) == 0) {
			printBoard(*board);
		} else if (strncmp(part, "perft", 5) == 0) {
			depth = atoi(part + 6);
			perftUntilDepth(*board, depth);
		} else if (strncmp(part, "quit", 4) == 0) {
			quit = 1;
		}
	}

	free(board);
}

// Lets the GUI know the engine is ready. Serves as a ping.
static void isready(void) {
	printf("readyok\n");
}

// Creates a brand new game
static void ucinewgame(Board *board) {
	static char* initial = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	parseFen(board, initial);
}

/*
 * Sets the board to a certain position
 * position (startpos | fen) <fen?> (moves e2e4 c7c5)?
 */
static void position(Board *board, char *s) {
	History *history = malloc(sizeof(History));
	Move move;

	char *moveText, *rest;

	if (strncmp(s, "startpos", 8) == 0) {
		s += 9;
		ucinewgame(board);
	} else if (strncmp(s, "fen", 3) == 0) {
		s += 4;
		s += parseFen(board, s) + 1;
	}

	if (strncmp(s, "moves", 5) == 0) {
		s += 6;
		rest = s;

		while ((moveText = strtok_r(rest, " ", &rest))) {
			move = textToMove(*board, moveText);
			makeMove(board, move, history);
		}
	}

	printBoard(*board);

	free(history);
}

static void go(Settings *settings, char *s) {
	// Default values for 1+0
	settings->stop = 0;
	settings->depth = 0;
	settings->nodes = 0;
	settings->mate = 0;
	settings->wtime = 60000;
	settings->btime = 60000;
	settings->winc = 0;
	settings->binc = 0;
	settings->movestogo = 20;
	settings->movetime = 0;

	while (s[1] != '\0') {
		if (strncmp(s, "infinite", 8) == 0) {
			settings->depth = MAX_DEPTH;
			break;
		} else if (strncmp(s, "wtime", 5) == 0) {
			s += 6;
			settings->wtime = atoi(s);
		} else if (strncmp(s, "btime", 5) == 0) {
			s += 6;
			settings->btime = atoi(s);
		} else if (strncmp(s, "winc", 4) == 0) {
			s += 5;
			settings->winc = atoi(s);
		} else if (strncmp(s, "binc", 4) == 0) {
			s += 5;
			settings->binc = atoi(s);
		} else if (strncmp(s, "movestogo", 9) == 0) {
			s += 10;
			settings->movestogo = atoi(s);
		} else if (strncmp(s, "depth", 5) == 0) {
			s += 6;
			settings->depth = atoi(s);
		} else if (strncmp(s, "movetime", 8) == 0) {
			s += 9;
			settings->movetime = atoi(s);
		}

		++s;
	}

	// Maintain a small period of buffer time for the search to end
	if (settings->movestogo == 1) {
		settings->wtime -= 50;
		settings->btime -= 50;
	}
}

/*
 * Runs perft from 1 to the required depth.
 */
static void perftUntilDepth(Board board, const int depth) {
	long nodes = 0;

	clock_t start, duration;

	for (int d = 1; d <= depth; ++d) {
		start = clock();
		nodes = perft(board, d);
		duration = 1000 * (clock() - start) / CLOCKS_PER_SEC;
		printf("info depth %d nodes %ld time %d\n", d, nodes, (int) duration);
	}

	printf("nodes %ld\n", nodes);
}
