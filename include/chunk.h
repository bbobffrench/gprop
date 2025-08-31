#ifndef CHUNK_H
#define CHUNK_H

#include <stdio.h>

typedef struct Chunk Chunk;
struct Chunk{
	size_t size;
	char *data;
	Chunk *next;
};

void
addchunk(Chunk **, char *, size_t);

char *
concatchunks(Chunk *);

void
writechunks(Chunk *, FILE *);

void
freechunks(Chunk *);

#endif