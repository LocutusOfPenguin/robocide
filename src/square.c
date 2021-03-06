#include <assert.h>

#include "square.h"

bool sqIsValid(Sq sq) {
	return (sq>=SqA1 && sq<=SqH8);
}

Sq sqMake(File file, Rank rank) {
	assert(fileIsValid(file));
	assert(rankIsValid(rank));
	return (rank<<3)+file;
}

File sqFile(Sq sq) {
	assert(sqIsValid(sq));
	return (sq&7);
}

Rank sqRank(Sq sq) {
	assert(sqIsValid(sq));
	return (sq>>3);
}

Sq sqMirror(Sq sq) {
	assert(sqIsValid(sq));
	return (sq^7);
}

Sq sqFlip(Sq sq) {
	assert(sqIsValid(sq));
	return (sq^56);
}

Sq sqNorth(Sq sq, unsigned int n) {
	assert(sqIsValid(sq));
	assert(sqRank(sq)<8-n);
	return sq+(8*n);
}

Sq sqSouth(Sq sq, unsigned int n) {
	assert(sqIsValid(sq));
	assert(sqRank(sq)>=n);
	return sq-(8*n);
}

Sq sqWestOne(Sq sq) {
	assert(sqIsValid(sq));
	assert(sqFile(sq)!=FileA);
	return sq-1;
}

Sq sqEastOne(Sq sq) {
	assert(sqIsValid(sq));
	assert(sqFile(sq)!=FileH);
	return sq+1;
}

Sq sqForwardOne(Sq sq, Colour colour) {
	assert(sqIsValid(sq));
	assert(colourIsValid(colour));
	return (colour==ColourWhite ? sqNorth(sq,1) : sqSouth(sq,1));
}

Sq sqBackwardOne(Sq sq, Colour colour) {
	assert(sqIsValid(sq));
	assert(colourIsValid(colour));
	return (colour==ColourWhite ? sqSouth(sq,1) : sqNorth(sq,1));
}

bool sqIsLight(Sq sq) {
	assert(sqIsValid(sq));
	return ((sqFile(sq)^sqRank(sq))&1);
}
