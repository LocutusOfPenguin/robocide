#include <assert.h>

#include "moves.h"
#include "perft.h"
#include "time.h"
#include "uci.h"

void perft(Pos *pos, Depth maxDepth) {
	assert(depthIsValid(maxDepth));

	uciWrite("Perft:\n");
	uciWrite("%6s %11s %9s %15s\n", "Depth", "Nodes", "Time", "NPS");
	Depth depth;
	for(depth=1;depth<=maxDepth;++depth) {
		TimeMs time=timeGet();
		unsigned long long int nodes=perftRaw(pos, depth);
		time=timeGet()-time;

		if (time>0) {
			unsigned long long int nps=(nodes*1000llu)/time;
			uciWrite("%6i %11llu %9llu %4llu,%03llu,%03llunps\n", (unsigned int)depth, nodes, time, nps/1000000, (nps/1000)%1000, nps%1000);
		} else
			uciWrite("%6i %11llu %9i %15s\n", (unsigned int)depth, nodes, 0, "-");
	}
}

void divide(Pos *pos, Depth depth) {
	if (depth<1)
		return;

	unsigned long long int total=0;
	Moves moves;
	movesInit(&moves, pos, 0, MoveTypeAny);
	Move move;
	while((move=movesNext(&moves))!=MoveInvalid) {
		char str[8];
		posMoveToStr(pos, move, str);
		if (!posMakeMove(pos, move))
			continue;
		unsigned long long int nodes=perftRaw(pos, depth-1);
		uciWrite("  %6s %12llu\n", str, nodes);
		total+=nodes;
		posUndoMove(pos);
	}
	uciWrite("Total: %llu\n", total);
}

unsigned long long int perftRaw(Pos *pos, Depth depth) {
	assert(depthIsValid(depth));

	if (depth<1)
		return 1;

	unsigned long long int total=0;
	Moves moves;
	movesInit(&moves, pos, 0, MoveTypeAny);
	Move move;

	if (depth==1) {
		while((move=movesNext(&moves))!=MoveInvalid)
			total+=posCanMakeMove(pos, move);
	} else {
		while((move=movesNext(&moves))!=MoveInvalid) {
			if (!posMakeMove(pos, move))
				continue;
			total+=perftRaw(pos, depth-1);
			posUndoMove(pos);
		}
	}

	return total;
}
