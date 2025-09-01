#include "config.h"
#include "map.h"

#include <SDL2/SDL_image.h>

#include <stdlib.h>
#include <math.h>

struct Tile{
	SDL_Texture *texture;
	int tx;
	int ty;
	Tile *next;
};

static void
addtile(Tile **tile, int tx, int ty)
{
	Tile *curr, *prev;

	/* Get the list tail */
	prev = NULL;
	for(curr = *tile; curr != NULL; curr = curr->next){
		prev = curr;
		/* Exit early if the tile is already present */
		if(curr->tx == tx && curr->ty == ty)
			break;
	}
	/* If the tile is not already present, create a new one and append it to the list */
	if(curr == NULL){
		curr = malloc(sizeof(Tile));
		curr->texture = NULL;
		curr->tx = tx;
		curr->ty = ty;
		curr->next = NULL;
		if(prev == NULL) *tile = curr;
		else prev->next = curr;
	}
}

static void
cleartiles(Tile *tile)
{
	Tile *next;

	for(; tile != NULL; tile = next){
		if(tile->texture != NULL)
			SDL_DestroyTexture(tile->texture);
		next = tile->next;
		free(tile);
	}
}

static SDL_Texture *
tiletexture(Map *map, int tx, int ty)
{
	Tile *curr;

	for(curr = map->tiles; curr != NULL; curr = curr->next){
		if(curr->tx == tx && curr->ty == ty)
			return curr->texture != NULL ? curr->texture : map->missing;
	}
	return map->missing;
}

/* Set offsets and map bounds so that the supplied coordinate is centered */
void
setlocation(Map *map, double lon, double lat)
{
	int width, height, tx, ty, px, py, available;

	SDL_QueryTexture(map->maptexture, NULL, NULL, &width, &height);

	/* Set the pixel offset so that the coordinate is centered */
	tilecoords(lon, lat, map->zoom, &tx, &ty, &px, &py);

	map->xoffset = px;
	map->yoffset = py;

	/* Set northernmost tile index */
	available = height / 2 - py;
	if(available <= 0) map->nbound = ty;
	else
		map->nbound = ty - ceil(available / (double)TILE_DIMENSION);

	/* Set southernmost tile index */
	available = height - (height / 2 - py + TILE_DIMENSION);
	if(available >= height) map->sbound = ty;
	else
		map->sbound = ty + ceil(available / (double)TILE_DIMENSION);

	/* Set westernmost tile index */
	available = width / 2 - px;
	if(available <= 0) map->wbound = tx;
	else
		map->wbound = tx - ceil(available / (double)TILE_DIMENSION);

	/* Set easternmost tile index */
	available = width - (width / 2 - px + TILE_DIMENSION);
	if(available >= width) map->ebound = tx;
	else
		map->ebound = tx + ceil(available / (double)TILE_DIMENSION);
}

static void
updatetextures(Map *map)
{
	Tile *curr;
	int filenamelen;
	char *filename;
	FILE *fp;
	SDL_Surface *surf;

	dlsync(&map->dl);

	for(curr = map->tiles; curr != NULL; curr = curr->next){
		/* Check through each missing texture to see if a tile PNG is available */
		if(curr->texture == NULL){
			filenamelen = snprintf(NULL, 0, "%s%d-%d-%d.png", TMP_DIR, map->zoom, curr->tx, curr->ty);
			filename = malloc(filenamelen + 1);
			sprintf(filename, "%s%d-%d-%d.png", TMP_DIR, map->zoom, curr->tx, curr->ty);

			/* Check whether the tile is available locally */
			if((fp = fopen(filename, "r")) != NULL){
				surf = IMG_Load(filename);
				curr->texture = SDL_CreateTextureFromSurface(map->r, surf);
				SDL_FreeSurface(surf);
			}
			/* If not, request it only if the tile has not yet been requested */
			else if(tilestatus(&map->dl, curr->tx, curr->ty, map->zoom) == MISSING)
				requesttile(&map->dl, curr->tx, curr->ty, map->zoom);
			free(filename);
		}
	}
}

void
updatemap(Map *map)
{
	int width, height, txcurr, tycurr, xcurr, ycurr;
	SDL_Rect src, dst;

	src.x = src.y = 0;
	src.w = src.h = dst.w = dst.h = TILE_DIMENSION;

	updatetextures(map);

	/* Add all bounded tiles to the cache */
	for(tycurr = map->nbound; tycurr <= map->sbound; tycurr++)
		for(txcurr = map->wbound; txcurr <= map->ebound; txcurr++)
			addtile(&map->tiles, txcurr, tycurr);

	SDL_QueryTexture(map->maptexture, NULL, NULL, &width, &height);
	SDL_SetRenderTarget(map->r, map->maptexture);

	/* Copy each tile texture onto the main map texture */
	ycurr = -map->yoffset;
	for(tycurr = map->nbound; tycurr <= map->sbound; tycurr++){
		xcurr = -map->xoffset;
		for(txcurr = map->wbound; txcurr <= map->ebound; txcurr++){
			dst.x = xcurr;
			dst.y = ycurr;
			SDL_RenderCopy(map->r, tiletexture(map, txcurr, tycurr), &src, &dst);
			xcurr += TILE_DIMENSION;
		}
		ycurr += TILE_DIMENSION;
	}
	SDL_SetRenderTarget(map->r, NULL);
}

void
initmap(Map *map, int width, int height, SDL_Renderer *r)
{
	SDL_Rect outline;

	map->r = r;
	map->zoom = DEFAULT_ZOOM;
	map->tiles = NULL;

	/* Create a texture to hold the current map frame */
	map->maptexture = SDL_CreateTexture(
		r,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET,
		width, height
	);

	/* Create missing tile fallback texture */
	map->missing = SDL_CreateTexture(
		r,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET,
		TILE_DIMENSION, TILE_DIMENSION
	);
	SDL_SetRenderTarget(map->r, map->missing);
	SDL_SetRenderDrawColor(map->r, 0xff, 0xff, 0xff, 0xff);
	SDL_RenderClear(map->r);
	SDL_SetRenderDrawColor(map->r, 0x00, 0x00, 0x00, 0xff);
	outline.x = outline.y = 0;
	outline.w = outline.h = TILE_DIMENSION;
	SDL_RenderDrawRect(map->r, &outline);
	SDL_SetRenderTarget(map->r, NULL);

	initdl(&map->dl);
	setlocation(map, DEFAULT_LON, DEFAULT_LAT);
	updatemap(map);
}

void
mapcleanup(Map *map)
{
	SDL_DestroyTexture(map->maptexture);
	SDL_DestroyTexture(map->missing);
	dlcleanup(&map->dl);
	cleartiles(map->tiles);
}

void
panmap(Map *map, int xoffset, int yoffset)
{
	map->xoffset += -xoffset;
	map->yoffset += -yoffset;

	/* If there is Y overflow, rebound the Y axis and stabilize the offset */
	if(map->yoffset < 0){
		for(; map->yoffset < 0; map->yoffset += TILE_DIMENSION){
			map->nbound--;
			map->sbound--;
		}
	} else{
		for(; map->yoffset >= TILE_DIMENSION; map->yoffset -= TILE_DIMENSION){
			map->nbound++;
			map->sbound++;
		}
	}
	/* If there is X overflow, rebound the X axis and stabilize the offset */
	if(map->xoffset < 0){
		for(; map->xoffset < 0; map->xoffset += TILE_DIMENSION){
			map->wbound--;
			map->ebound--;
		}
	} else{
		for(; map->xoffset >= TILE_DIMENSION; map->xoffset -= TILE_DIMENSION){
			map->wbound++;
			map->ebound++;
		}
	}
}


