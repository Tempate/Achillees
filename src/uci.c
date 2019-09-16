#include <string.h>
#include <time.h>

#include <unistd.h>
#include <pthread.h>

#include "board.h"
#include "play.h"
#include "search.h"
#include "eval.h"
#include "hashtables.h"
#include "uci.h"
#include "draw.h"


static void isready(void);
static void position(Board *board, char *s);
static void go(Board *board, Settings *settings, char *s);
static void setoption(Settings *settings, char *s);

pthread_t worker;
int working = 0;

void uci(void) {

	fprintf(stdout, "id name %s\n", ENGINE_NAME);
	fprintf(stdout, "id author %s\n", ENGINE_AUTHOR);
	fprintf(stdout, "option name hash type spin default 128 min 1 max 2048\n");
	fprintf(stdout, "uciok\n");
	fflush(stdout);

	Board board;
	initialBoard(&board);

	char msg[4096];

	while (fgets(msg, 4096, stdin) != NULL) {

		if (strncmp(msg, "isready", 7) == 0)
			isready();
		else if (strncmp(msg, "ucinewgame", 10) == 0)
			initialBoard(&board);
		else if (strncmp(msg, "position", 8) == 0)
			position(&board, msg + 9);
		else if (strncmp(msg, "eval", 4) == 0)
			evaluate(&board);
		else if (strncmp(msg, "go", 2) == 0)
			go(&board, &settings, msg + 2);
		else if (strncmp(msg, "setoption name", 14) == 0)
			setoption(&settings, msg + 15);
		else if (strncmp(msg, "stop", 4) == 0) {
			settings.stop = 1;

			if (working)
				stopSearchThread();
		} else if (strncmp(msg, "quit", 4) == 0)
			break;
	}

	if (working)
		stopSearchThread();
}

// Lets the GUI know the engine is ready. Serves as a ping.
static void isready(void) {
	fprintf(stdout, "readyok\n");
	fflush(stdout);
}

/*
 * Sets the board to a certain position
 * position (startpos | fen) (moves e2e4 c7c5)?
 */
static void position(Board *board, char *s) {
	clearKeys();

	if (strncmp(s, "startpos", 8) == 0) {
		s += 9;
		initialBoard(board);
	} else if (strncmp(s, "fen", 3) == 0) {
		s += 4;
		s += fenToBoard(board, s) + 1;
	} else {
		s += fenToBoard(board, s) + 1;
	}

	if (strncmp(s, "moves", 5) == 0)
		playMoves(board, s + 6);
}

static void go(Board *board, Settings *settings, char *s) {
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

	createSearchThread(board);
}

// Options are expected to be in the form:
// <option> value <value>
static void setoption(Settings *settings, char *s) {
	printf("%s\n", s);

	if (strncmp(s, "hash", 4) == 0)
		resizeTT(atoi(s + 11));
}

void playMoves(Board *board, char *moves) {
	char *rest, *moveText;
	rest = moves;

	while ((moveText = strtok_r(rest, " ", &rest))) {
		const Move move = textToMove(board, moveText);

		History history;

		makeMove(board, &move, &history);
		updateBoardKey(board, &move, &history);
		saveKeyToMemory(board->key);
	}
}

void *bestmove(void *args) {
	Board *board = (Board *) args;
	char pv[6];

	const Move move = search(board);
	moveToText(move, pv);

	fprintf(stdout, "bestmove %s\n", pv);
	fflush(stdout);

	return NULL;
}

void infoString(const Board *board, const int depth, const int score, const uint64_t nodes, const int duration, Move *pv, const int nPV) {

	fprintf(stdout, "info depth %d score cp %d nodes %ld time %d ", depth, score, nodes, duration);

	if (duration > 0) {
		const int nps = 1000 * nodes / duration;
		fprintf(stdout, "nps %d ", nps);
	}

	fprintf(stdout, "pv ");

	for (int i = 0; i < nPV; ++i) {
		char moveText[6];
		moveToText(pv[i], moveText);
		fprintf(stdout, "%s ", moveText);
	}

	fprintf(stdout, "\n");
	fflush(stdout);
}

void createSearchThread(Board *board) {
	if (working)
		pthread_join(worker, NULL);

	pthread_create(&worker, NULL, bestmove, (void *) board);
	working = 1;
}

void stopSearchThread(void) {
	settings.stop = 1;
	pthread_join(worker, NULL);
	working = 0;
}
