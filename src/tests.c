
#include "headers/board.h"
#include "headers/play.h"
#include "headers/draw.h"
#include "headers/search.h"
#include "headers/uci.h"
#include "headers/sort.h"
#include "headers/hashtables.h"

#include <time.h>
#include <string.h>


void testMakeMove(char *fen) {
	Board *board = malloc(sizeof(Board));
	Move moves[MAX_MOVES];
	History history;

	parseFen(board, fen);

	int n = legalMoves(board, moves);

	printBoard(board);

	for (int i = 0; i < n; i++) {
		makeMove(board, &moves[i], &history);
		printBoard(board);
		undoMove(board, &moves[i], &history);
	}

	free(board);
}

void testMoves(char *fen, const int depth) {
	Board *board = malloc(sizeof(Board));
	Move moves[MAX_MOVES];
	History history;

	const int newDepth = depth - 1;
	int k, r, s = 0;

	time_t start = clock();

	parseFen(board, fen);
	printBoard(board);

	k = legalMoves(board, moves);

	for (int i = 0; i < k; i++) {
		makeMove(board, &moves[i], &history);

		if (newDepth > 0) {
			r = perft(board, newDepth);
			printMove(moves[i], r);
			s += r;
		} else {
			printMove(moves[i], 0);
		}

		undoMove(board, &moves[i], &history);
	}

	printf("\n");
	printf("Time: %.4f\n", (double)(clock() - start)/CLOCKS_PER_SEC);
	printf("Moves: %d\n", k);
	printf("Nodes: %d\n", s);

	free(board);
}

void testPerft(const char* filename, const int depth) {
	Board board;
	FILE *ifp;

	char *c = malloc(85);
	char *fen = malloc(70);
	char *rest = malloc(15);

	int k, len1, len2;
	long nodes;

	ifp = fopen(filename, "r");

	if (ifp == NULL) {
		printf("Can't open input file %s\n", filename);
		exit(1);
	}

	while (!feof(ifp)) {
		fgets(c, 120, ifp);
		fen = strtok(c, ";");
		rest = strtok(NULL, c);
		nodes = atoi(rest + 3);

		len1 = (int) 70 - strlen(fen);
		len2 = (int) 10 - strlen(rest);

		printf("%s", fen);

		for (int i = 0; i < len1; i++)
			printf(" ");

		parseFen(&board, fen);
		k = perft(&board, depth);

		printf("%ld \t %d", nodes, k);

		for (int i = 0; i < len2; i++)
			printf(" ");

		printf("%s\n", (nodes == k) ? "PASS" : "ERROR");
	}

	free(rest);
	free(fen);
	free(c);
}

void testKeys(void) {
	Board *board1 = malloc(sizeof(Board));
	Board *board2 = malloc(sizeof(Board));

	Move move;
	History history;


	parseFen(board1, "rnbqk2r/pppp1ppp/4pn2/2b5/2B5/4PN2/PPPP1PPP/RNBQK2R w KQkq -");
	parseFen(board2, "rnbqk2r/pppp1ppp/4pn2/2b5/2B5/4PN2/PPPP1PPP/RNBQK1R1 b Qkq -");

	move = textToMove(board1, "h1g1");

	printf("%" PRIu64 "\n", board1->key);
	printBoard(board1);

	printf("%" PRIu64 "\n", board2->key);
	printBoard(board2);

	makeMove      (board1, &move, &history);
	updateBoardKey(board1, &move, &history);

	printf("%" PRIu64 "\n", board1->key);
	printBoard(board1);

	updateBoardKey(board1, &move, &history);
	undoMove      (board1, &move, &history);

	printf("%" PRIu64 "\n", board1->key);
	printBoard(board1);

	free(board1);
	free(board2);
}

void testSearch(Board *board, const int depth) {
	Move moves[MAX_MOVES];
	const int nMoves = legalMoves(board, moves);

	for (int i = 0; i < nMoves; ++i) {
		History history;

		makeMove(board, &moves[i], &history);
		updateBoardKey(board, &moves[i], &history);
		saveKeyToMemory(board->key);

		int score;

		if (isDraw(board)) {
			score = 0;
		} else {
			score = -alphabeta(board, depth, -2 * MAX_SCORE, 2 * MAX_SCORE, 0);
		}

		freeKeyFromMemory();
		updateBoardKey(board, &moves[i], &history);
		undoMove(board, &moves[i], &history);

		printMove(moves[i], score);
	}
}

void testPosition(char* fen, const int depth) {
	Board *board = malloc(sizeof(Board));

	parseFen(board, fen);
	printBoard(board);

	settings.depth = depth;

	const int index = board->key % HASHTABLE_MAX_SIZE;
	printMove(search(board), tt[index].score);

	free(board);
}

void testDraw(void) {
	printf("\n3fold Qc1\n");
	testPosition("7k/3QQ3/8/8/8/PPP5/2q5/K7 b - -", 10);

	printf("\n3fold Qc8\n");
	testPosition("k7/2Q5/ppp5/8/8/8/3qq3/7K w - -", 10);

	printf("\n50moves h3\n");
	testPosition("7k/8/R7/1R6/7K/8/7P/8 w - - 99 100", 7);

	printf("\n50moves h6\n");
	testPosition("8/7p/8/7k/1r6/r7/8/7K b - - 99 100", 7);

	printf("\n50moves a6\n");
	testPosition("8/8/8/P7/8/6n1/3R4/R3K2k w Q - 99 100", 7);

	printf("\n50moves a3\n");
	testPosition("r3k2K/3r4/6N1/8/p7/8/8/8 b q - 99 100", 7);

	printf("\nstalemate Qa7\n");
	testPosition("k5q1/p7/8/6q1/6q1/6q1/8/Q6K w - -", 7);

	printf("\nstalemate Qa2\n");
	testPosition("q6k/8/6Q1/6Q1/6Q1/8/P7/K5Q1 b - -", 7);

	printf("\nfastmate Rd8\n");
	testPosition("k7/pp6/8/8/8/8/3R4/K7 w - -", 7);

	printf("\nfastmate Rc8\n");
	testPosition("k7/8/pp6/8/8/2R5/2R5/K7 w - -", 7);

	printf("\nfastmate Rf8\n");
	testPosition("k7/6R1/5R1P/8/8/8/8/K7 w - -", 7);

	printf("\nmate Rc8\n");
	testPosition("k7/pp6/8/8/8/2Rq4/8/K7 w - -", 7);

	printf("\nmate 0-0-0\n");
	testPosition("8/8/8/8/8/8/R7/R3K2k w Q -", 7);

	printf("\nunderpromotion f8N\n");
	testPosition("8/5P1k/8/4B1K1/8/1B6/2N5/8 w - -", 7);

	printf("\nunderpromotion f1N\n");
	testPosition("8/2n5/1b6/8/4b1k1/8/5p1K/8 b - -", 7);
}

void testSee(void) {
	Board *board = malloc(sizeof(Board));

	parseFen(board, "k3r3/4r3/2b2n2/3q1pN1/4p2R/2n5/5NB1/K3R2Q w - -");
	printBoard(board);

	const int color = WHITE;
	const int sqr = 28;
	const int from = getSmallestAttacker(board, sqr, color);
	const int piece = findPiece(board, square[from], color);

	const Move move = (Move){.to=sqr,.from=from,.piece=piece,.color=color, .promotion=0};

	printf("Smallest attacker: %d\n", from);

	const int score = seeCapture(board, &move);

	printf("SEE score is: %d\n", score);

	free(board);
}
