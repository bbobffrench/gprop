#include "map.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdio.h>

int
main(int argc, char **argv)
{
	Map map;
	int width, height, i;
	SDL_Window *w;
	SDL_Renderer *r;
	SDL_Rect src, dst;

	width = 1024;
	height = 768;

	src.x = src.y = dst.x = dst.y = 0;
	src.w = dst.w = width;
	src.h = dst.h = height;

	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);

	w = SDL_CreateWindow(
		"gprop",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width, height,
		SDL_WINDOW_SHOWN
	);
	r = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED);

	initmap(&map, width, height, r);

	for(i = 0; i < 100; i++){
		updatemap(&map);
		SDL_RenderCopy(r, map.maptexture, &src, &dst);
		SDL_RenderPresent(r);
		SDL_Delay(50);
	}
	mapcleanup(&map);
	SDL_DestroyRenderer(r);
	SDL_DestroyWindow(w);
	SDL_Quit();
	IMG_Quit();
}
