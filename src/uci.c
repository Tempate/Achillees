#include <string.h>
#include <assert.h>
#include <time.h>

#include <unistd.h>
#include <pthread.h>

#include "main.h"
#include "board.h"
#include "play.h"
#include "search.h"
#include "eval.h"
#include "hashtables.h"
#include "uci.h"


static void uci(void);
static void isready(void);
static void ucinewgame(Board *board);
static void position(Board *board, char *s);
static void go(Settings *settings, char *s);
static void *bestmove(void *args);
static void evalCmd(const Board *board);

static void perftUntilDepth(Board *board, const int depth);
static void defaultSettings(Settings *settings);

Settings settings;

void listen(void) {
	Board *board = malloc(sizeof(Board));

	char message[4096], *r, *part;
	int depth, quit = 0;

	pthread_t worker;
	int working = 0;

	while (!quit) {
		r = fgets(message, 4096, stdin);

		if (r == NULL)
			break;

		part = message;

		if (strncmp(part, "ucinewgame", 10) == 0) {
			ucinewgame(board);
		} else if (strncmp(part, "uci", 3) == 0) {
			uci();
		} else if (strncmp(part, "isready", 7) == 0) {
			isready();
		} else if (strncmp(part, "position", 8) == 0) {
			position(board, part + 9);
		} else if (strncmp(part, "go", 2) == 0) {
			go(&settings, part + 2);

			/*
			 * Creates a second thread to search so that the UCI thread
			 * can still reply to commands.
			 */
			pthread_create(&worker, NULL, bestmove, (void *) board);
			working = 1;
		} else if (strncmp(part, "stop", 4) == 0) {
			settings.stop = 1;

			// Waits until the worker thread has stopped.
			if (working == 1) {
				pthread_join(worker, NULL);
				working = 0;
			}
		} else if (strncmp(part, "print", 5) == 0) {
			printBoard(board);
		} else if (strncmp(part, "perft", 5) == 0) {
			depth = atoi(part + 6);
			perftUntilDepth(board, depth);
		} else if (strncmp(part, "eval", 4) == 0) {
			evalCmd(board);
		} else if (strncmp(part, "quit", 4) == 0) {
			quit = 1;
		}
	}

	if (working == 1) {
		settings.stop = 1;
		pthread_join(worker, NULL);
	}

	free(board);
}

// Initial command to set UCI as the control mode
static void uci(void) {
	fprintf(stdout, "id name %s\n", ENGINE_NAME);
	fprintf(stdout, "id author %s\n", ENGINE_AUTHOR);
	fprintf(stdout, "uciok\n");
	fflush(stdout);
}

// Lets the GUI know the engine is ready. Serves as a ping.
static void isready(void) {
	fprintf(stdout, "readyok\n");
	fflush(stdout);
}

// Creates a brand new game
static void ucinewgame(Board *board) {
	static char* initial = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	parseFen(board, initial);
}

/*
 * Sets the board to a certain position
 * position (startpos | fen) (moves e2e4 c7c5)?
 */
static void position(Board *board, char *s) {
	History history;
	Move move;

	char *moveText, *rest;

	if (strncmp(s, "startpos", 8) == 0) {
		s += 9;
		ucinewgame(board);
	} else {
		s += parseFen(board, s) + 1;
	}

	if (strncmp(s, "moves", 5) == 0) {
		s += 6;
		rest = s;

		while ((moveText = strtok_r(rest, " ", &rest))) {
			move = textToMove(board, moveText);
			makeMove(board, &move, &history);
			updateBoardKey(board, &move, &history);

			// assert(board->key == zobristKey(*board));
		}
	}
}

static void go(Settings *settings, char *s) {
	defaultSettings(settings);

	while (s[1] != '\0' && s[1] != '\n') {
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

static void *bestmove(void *args) {
	Board *board = (Board *) args;
	Move move;
	char pv[6];

	move = search(board);
	moveToText(move, pv);

	fprintf(stdout, "bestmove %s\n", pv);
	fflush(stdout);

	pthread_exit(NULL);

	return NULL;
}

static void evalCmd(const Board *board) {
	int score = eval(board);

	if (board->turn == BLACK)
		score = -score;

	fprintf(stdout, "%d\n", score);
	fflush(stdout);
}

/*
 * Runs perft from 1 to the required depth.
 */
static void perftUntilDepth(Board *board, const int depth) {
	long nodes = 0;

	clock_t start, duration;

	for (int d = 1; d <= depth; ++d) {
		start = clock();
		nodes = perft(board, d);
		duration = 1000 * (clock() - start) / CLOCKS_PER_SEC;
		fprintf(stdout, "info depth %d nodes %ld time %d\n", d, nodes, (int) duration);
	}

	fprintf(stdout, "nodes %ld\n", nodes);
	fflush(stdout);
}

static void defaultSettings(Settings *settings) {
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
