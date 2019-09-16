#ifndef SRC_HASHTABLES_H_
#define SRC_HASHTABLES_H_

#define DEF_TT_SIZE 128

#define CAST_OFFSET 768
#define ENPA_OFFSET 772
#define TURN_OFFSET 780

#include "search.h"

enum { bPawn, wPawn, bKnight, wKnight, bBishop, wBishop, bRook, wRook, bQueen, wQueen, bKing, wKing };

typedef struct {
	uint16_t from : 6;
	uint16_t to : 6;
	uint16_t promotion : 3;
}  /*__attribute__ ((packed)) */ MoveCompressed;


// 13 bytes
typedef struct {
	uint64_t key;

	MoveCompressed move;

	int16_t score;
	uint8_t depth : 6;
	uint8_t flag : 2;

	//TODO: age
} /*__attribute__ ((packed)) */ Entry;


// Transposition Table
extern Entry *tt;

void initTT(const int size);
void resizeTT(const int size);
void clearTT(void);

uint64_t zobristKey(const Board *board);

void updateBoardKey(Board *board, const Move *move, const History *history);
void updateNullMoveKey(Board *board);

Entry compressEntry(const uint64_t key, const Move *move, const int score, const int depth, const int flag);
Move decompressMove(const Board *board, const MoveCompressed *moveComp);

int probePV(Board board, Move *pv);


#endif /* SRC_HASHTABLES_H_ */
