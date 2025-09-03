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

void
setlocation(Map *map, double lon, double lat)
{
    int tx, ty, px, py, xctr, yctr;

    tilecoords(lon, lat, map->zoom, &tx, &ty, &px, &py);
    xctr = map->width / 2;
    yctr = map->height / 2;

	/* Calculate bounds and pixel offsets on the Y axis */
	map->nbound = ty - (yctr - py + TILE_DIMENSION) / TILE_DIMENSION;
	map->sbound = ty + (map->height - (yctr - py)) / TILE_DIMENSION;
	map->yoffset = TILE_DIMENSION - (yctr - py + TILE_DIMENSION) % TILE_DIMENSION;

	/* Calculate bounds and pixel offsets on the X axis */
	map->wbound = tx - (xctr - px + TILE_DIMENSION) / TILE_DIMENSION;
	map->ebound = tx + (map->width - (xctr - px)) / TILE_DIMENSION;
	map->xoffset = TILE_DIMENSION - (xctr - px + TILE_DIMENSION) % TILE_DIMENSION;
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
				fclose(fp);
				if((surf = IMG_Load(filename)) != NULL){
					curr->texture = SDL_CreateTextureFromSurface(map->r, surf);
					SDL_FreeSurface(surf);
				}
				/* If the load fails, request the tile if it is not already pending */
				else if(tilestatus(&map->dl, curr->tx, curr->ty, map->zoom) != PENDING)
					requesttile(&map->dl, curr->tx, curr->ty, map->zoom);
			}
			/* If not, request it only if the tile isn't yet being tracked */
			else if(tilestatus(&map->dl, curr->tx, curr->ty, map->zoom) == MISSING)
				requesttile(&map->dl, curr->tx, curr->ty, map->zoom);
			free(filename);
		}
	}
}

void
updatemap(Map *map)
{
	int txcurr, tycurr, xcurr, ycurr;
	SDL_Rect src, dst;

	src.x = src.y = 0;
	src.w = src.h = dst.w = dst.h = TILE_DIMENSION;

	/* Add all bounded tiles to the cache */
	for(tycurr = map->nbound; tycurr <= map->sbound; tycurr++)
		for(txcurr = map->wbound; txcurr <= map->ebound; txcurr++)
			addtile(&map->tiles, txcurr, tycurr);

	/* Copy each tile texture onto the main map texture */
	SDL_SetRenderTarget(map->r, map->maptexture);
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

	updatetextures(map);
}

void
initmap(Map *map, int width, int height, SDL_Renderer *r)
{
	SDL_Rect outline;

	map->r = r;
	map->width = width;
	map->height = height;
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
mapcoords(Map *map, int x, int y, double *lon, double *lat)
{
	int tx, ty, px, py;

	/* Recover the tile and pixel indices of the requested point */
	tx = map->wbound + (x + map->xoffset) / TILE_DIMENSION;
	ty = map->nbound + (y + map->yoffset) / TILE_DIMENSION;
	px = (x + map->xoffset) % TILE_DIMENSION;
	py = (y + map->yoffset) % TILE_DIMENSION;

	latlon(tx, ty, px, py, map->zoom, lon, lat);
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

static void
setzoom(Map *map, int zoom)
{
	double lon, lat;

	/* Ensure that the requested zoom is legal */
	if(zoom < ZOOM_MIN || zoom > ZOOM_MAX)
		return;

	/* Get the geographical coordinates of the maps central point */
	mapcoords(map, map->width / 2, map->height / 2, &lon, &lat);

	/* Recenter the map on this coordinate with the new zoom applied */
	map->zoom = zoom;
	setlocation(map, lon, lat);

	/* Remove tiles of the previous zoom level */
	cleartiles(map->tiles);
	map->tiles = NULL;
}

void
incmapzoom(Map *map)
{
	setzoom(map, map->zoom + 1);
}

void
decmapzoom(Map *map)
{
	setzoom(map, map->zoom - 1);
}
