#ifndef MAP_H
#define MAP_H

#include "tile.h"

#include <SDL2/SDL.h>

typedef struct Tile Tile;
struct Tile{
	SDL_Texture *texture;
	int tx;
	int ty;
	Tile *next;
};

typedef struct Map Map;
struct Map{
	SDL_Renderer *r;
	SDL_Texture *maptexture;
	SDL_Texture *missing;
	int zoom;
	int xoffset;
	int yoffset;
	int nbound;
	int sbound;
	int wbound;
	int ebound;
	Downloader dl;
	Tile *tiles;
};

void
initmap(Map *, int, int, SDL_Renderer *);

void
mapcleanup(Map *);

void
setlocation(Map *, double, double);

void
updatemap(Map *);

#endif