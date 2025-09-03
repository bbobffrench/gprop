#include "config.h"
#include "map.h"
#include "search.h"

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
	OSMSearch *search;

	search = geosearch(argv[1]);
	if(search == NULL){
		fprintf(stderr, "No search results\n");
		return 1;
	}
	else printf("Positioning map at %s\n", search->name);

	width = 780;
	height = 789;

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
	r = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED);

	initmap(&map, width, height, r);
	setlocation(&map, search->lon, search->lat);

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

			else if(e.type == SDL_KEYDOWN){
				if(e.key.keysym.sym == SDLK_EQUALS && e.key.keysym.mod & KMOD_SHIFT)
					incmapzoom(&map);
				else if(e.key.keysym.sym == SDLK_MINUS)
					decmapzoom(&map);
			}
		}

		SDL_Delay(fmax(0, 1000 / (double)FPS - (time - SDL_GetTicks())));
	}

	mapcleanup(&map);
	SDL_DestroyRenderer(r);
	SDL_DestroyWindow(w);
	IMG_Quit();
	SDL_Quit();
	return 0;
}
