#include "config.h"
#include "map.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdio.h>

int
main(int argc, char **argv)
{
	Map map;
	int width, height;
	SDL_Window *w;
	SDL_Renderer *r;
	SDL_Rect rect;
	SDL_Event e;
	char running;
	unsigned time;

	width = 1024;
	height = 768;

	rect.x = rect.y = 0;
	rect.w = width;
	rect.h = height;

	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);

	w = SDL_CreateWindow(
		"gprop",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width, height,
		SDL_WINDOW_SHOWN
	);
	r = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	initmap(&map, width, height, r);

	running = 1;
	while(running){
		time = SDL_GetTicks();

		updatemap(&map);
		SDL_RenderCopy(r, map.maptexture, &rect, &rect);
		SDL_RenderPresent(r);

		while(SDL_PollEvent(&e) != 0){
			if(e.type == SDL_QUIT) running = 0;
			if(e.type == SDL_MOUSEMOTION && e.motion.state & SDL_BUTTON(1))
				panmap(&map, e.motion.xrel, e.motion.yrel);
		}

		SDL_Delay(fmax(0, 1000 / (double)FPS - SDL_GetTicks()));
	}

	mapcleanup(&map);
	SDL_DestroyRenderer(r);
	SDL_DestroyWindow(w);
	SDL_Quit();
	IMG_Quit();
}
