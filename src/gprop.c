#include "tile.h"

#include <stdio.h>

int
main(int argc, char **argv)
{
	double lon, lat;
	int tx, ty, px, py, zoom;
	Downloader dl;

	/* sweethome :^) */
	lon = -78.79972073198542;
	lat = 43.010120241553935;

	zoom = 17;
	printf("break\n");
	tilecoords(lon, lat, zoom, &tx, &ty, &px, &py);
	printf("Tile coordinates: %d, %d\n", tx, ty);
	printf("Pixel coordinates: %d, %d\n", px, py);
	lon = lat = 0;
	latlon(tx, ty, px, py, zoom, &lon, &lat);
	printf("Latitude: %lf, Longitude: %lf\n", lat, lon);

	initdl(&dl);
	
	requesttile(&dl, tx, ty, zoom);
	while(1){
		dlsync(&dl);
	}

	dlcleanup(&dl);

	return 0;
}
