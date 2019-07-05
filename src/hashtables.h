#ifndef SRC_HASHTABLES_H_
#define SRC_HASHTABLES_H_


#define HASHTABLE_MAX_SIZE 0x400000

#define CAST_OFFSET 768
#define ENPA_OFFSET 772
#define TURN_OFFSET 780


enum { bPawn, wPawn, bKnight, wKnight, bBishop, wBishop, bRook, wRook, bQueen, wQueen, bKing, wKing };

typedef struct {
	uint8_t from : 6;
	uint8_t to : 6;
	uint8_t promotion : 3;
} MoveCompressed;


// 106 bits ~ 14 bytes
typedef struct {
	uint64_t key;

	MoveCompressed move;

	int16_t score;
	uint8_t depth;
	uint8_t type : 2;

	//TODO: age
} Position;


// Transposition/Refutation Table
extern Position tt[HASHTABLE_MAX_SIZE];


void initializeTT(void);

uint64_t zobristKey(Board board);
void updateBoardKey(Board *board, Move move, History history);

Position compressPosition(const uint64_t key, const Move move, const int score, const int depth, const int type);
Move decompressMove(Board board, MoveCompressed moveComp);


#endif /* SRC_HASHTABLES_H_ */
