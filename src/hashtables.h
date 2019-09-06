#ifndef SRC_HASHTABLES_H_
#define SRC_HASHTABLES_H_

#define DEF_TT_SIZE 128
#define TT_ENTRIES DEF_TT_SIZE * 1024 * 1024 / 14

#define CAST_OFFSET 768
#define ENPA_OFFSET 772
#define TURN_OFFSET 780

#include "search.h"

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
	uint8_t flag : 2;

	//TODO: age
} Entry;


// Transposition Table
extern Entry tt[TT_ENTRIES];


void initTT(void);
void clearTT(void);

uint64_t zobristKey(const Board *board);

void updateBoardKey(Board *board, const Move *move, const History *history);
void updateNullMoveKey(Board *board);

Entry compressEntry(const uint64_t key, const Move *move, const int score, const int depth, const int flag);
Move decompressMove(const Board *board, const MoveCompressed *moveComp);

PV probePV(Board board);


#endif /* SRC_HASHTABLES_H_ */
