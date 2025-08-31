#include "chunk.h"

#include <stdlib.h>
#include <string.h>

void
addchunk(Chunk **chunk, char *data, size_t size)
{
	Chunk *new, *curr;

	/* Allocate chunk and copy data */
	new = malloc(sizeof(Chunk));
	new->size = size;
	new->data = malloc(size);
	memcpy(new->data, data, size);
	new->next = NULL;

	/* Append the chunk to the list */
	if(*chunk == NULL)
		*chunk = new;
	else{
		for(curr = *chunk; curr->next != NULL; curr = curr->next);
		curr->next = new;
	}
}

/* Concatenate all chunks into a null-terminated string and free the chunk list */
char *
concatchunks(Chunk *chunk)
{
	Chunk *curr;
	char *buff;
	size_t size, pos;

	/* Allocate a buffer large enough to fit all data */
	size = 0;
	for(curr = chunk; curr != NULL; curr = curr->next)
		size += curr->size;
	buff = malloc(size + 1);

	/* Populate the buffer with all chunk data and free each chunk */
	pos = 0;
	for(; chunk != NULL; chunk = curr){
		memcpy(&buff[pos], chunk->data, chunk->size);
		pos += chunk->size;
		curr = chunk->next;
		free(chunk->data);
		free(chunk);
	}
	buff[size] = '\0';
	return buff;
}

/* Write all chunks to a file and free the chunk list */
void
writechunks(Chunk *chunk, FILE *fp)
{
	Chunk *curr;

	/* Write each chunk to the file and subsequently free it */
	for(; chunk != NULL; chunk = curr){
		fwrite(chunk->data, 1, chunk->size, fp);
		curr = chunk->next;
		free(chunk->data);
		free(chunk);
	}
}

void
freechunks(Chunk *chunk){
	Chunk *curr;

	for(; chunk != NULL; chunk = curr){
		curr = chunk->next;
		free(chunk->data);
		free(chunk);
	}
}