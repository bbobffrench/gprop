#ifndef TILE_H
#define TILE_H

#include "chunk.h"

#include <curl/curl.h>

enum TileStatus {MISSING, PENDING, AVAILABLE};
typedef enum TileStatus TileStatus;

typedef struct TileTracker TileTracker;
struct TileTracker{
	TileStatus status;
	int x;
	int y;
	int zoom;
	CURL *curl;
	Chunk *chunk;
	TileTracker *next;
};

typedef struct Downloader Downloader;
struct Downloader{
	CURLM *multi;
	TileTracker *tracker;
};

void
initdl(Downloader *);

void
dlcleanup(Downloader *);

TileStatus
tilestatus(Downloader *, int, int, int);

void
requesttile(Downloader *, int, int, int);

void
dlsync(Downloader *);

void
tilecoords(double, double, int, int *, int *, int *, int *);

void
latlon(int, int, int, int, int, double *, double *);

#endif