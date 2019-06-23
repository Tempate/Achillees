
#include "board.h"
#include "play.h"

#include <time.h>
#include <string.h>


void testMakeMove(char *fen) {
	Board board;
	Move moves[218];
	History history;

	parseFen(&board, fen);

	int n = legalMoves(board, moves);

	printBoard(board);

	for (int i = 0; i < n; i++) {
		makeMove(&board, moves[i], &history);
		printBoard(board);
		undoMove(&board, moves[i], history);
	}
}

void testMoves(char *fen, const int depth) {
	Move *moves = malloc(218 * sizeof(Move));
	Board board;
	History history;

	const int newDepth = depth - 1;
	int k, r, s = 0;

	time_t start = clock();

	parseFen(&board, fen);
	printBoard(board);

	k = legalMoves(board, moves);

	for (int i = 0; i < k; i++) {
		makeMove(&board, moves[i], &history);

		if (newDepth > 0) {
			r = perft(board, newDepth);
			printMove(moves[i], r);
			s += r;
		} else {
			printMove(moves[i], 0);
		}

		undoMove(&board, moves[i], history);
	}

	printf("\n");
	printf("Time: %.4f\n", (double)(clock() - start)/CLOCKS_PER_SEC);
	printf("Moves: %d\n", k);
	printf("Nodes: %d\n", s);

	free(moves);
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
		k = perft(board, depth);

		printf("%ld \t %d", nodes, k);

		for (int i = 0; i < len2; i++)
			printf(" ");

		printf("%s\n", (nodes == k) ? "PASS" : "ERROR");
	}

	free(rest);
	free(fen);
	free(c);
}
