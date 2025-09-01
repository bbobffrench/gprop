#include "config.h"
#include "tile.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

struct TileTracker{
	TileStatus status;
	int x;
	int y;
	int zoom;
	CURL *curl;
	Chunk *chunk;
	TileTracker *next;
};

void
initdl(Downloader *dl)
{
	dl->multi = curl_multi_init();
	dl->tracker = NULL;
}

void
dlcleanup(Downloader *dl){
	TileTracker *curr, *next;

	for(curr = dl->tracker; curr != NULL; curr = next){
		if(curr->curl != NULL){
			curl_multi_remove_handle(dl->multi, curr->curl);
			curl_easy_cleanup(curr->curl);
		}
		if(curr->chunk != NULL)
			freechunks(curr->chunk);
		next = curr->next;
		free(curr);
	}
	curl_multi_cleanup(dl->multi);
}

static TileTracker *
updatetracker(TileTracker **tracker, TileStatus status, int x, int y, int zoom)
{
	TileTracker *curr, *prev;

	/* Check if the tile is already there and position to the last tile if not */
	prev = NULL;
	for(curr = *tracker; curr != NULL; curr = curr->next){
		if(curr->x == x && curr->y == y && curr->zoom == zoom){
			curr->status = status;
			curr->curl = NULL;
			curr->chunk = NULL;
			return curr;
		}
		prev = curr;
	}
	/* Add a new tile to the end of the tracker list */
	if(prev == NULL){
		*tracker = malloc(sizeof(TileTracker));
		curr = *tracker;
	}
	else{
		prev->next = malloc(sizeof(TileTracker));
		curr = prev->next;
	}
	curr->status = status;
	curr->x = x;
	curr->y = y;
	curr->zoom = zoom;
	curr->curl = NULL;
	curr->chunk = NULL;
	curr->next = NULL;
	return curr;
}

TileStatus
tilestatus(Downloader *dl, int x, int y, int zoom)
{
	TileTracker *curr;

	for(curr = dl->tracker; curr != NULL; curr = curr->next)
		if(curr->x == x && curr->y == y && curr->zoom == zoom)
			return curr->status == AVAILABLE ? AVAILABLE : PENDING;
	return MISSING;
}

void
requesttile(Downloader *dl, int x, int y, int zoom)
{
	updatetracker(&dl->tracker, MISSING, x, y, zoom);
}

static size_t
storechunk(char *data, size_t size, size_t nmemb, void *ptr)
{
	addchunk(&((TileTracker *)ptr)->chunk, data, size * nmemb);
	return size * nmemb;
}

static void
handlemessages(Downloader *dl)
{
	CURLMsg *msg;
	TileTracker *curr;
	int filenamelen, queuelen;
	char *filename;
	FILE *fp;

	while((msg = curl_multi_info_read(dl->multi, &queuelen)) != NULL){
		/* Fetch the appropriate tracker */
		for(curr = dl->tracker; curr->curl != msg->easy_handle; curr = curr->next);

		/* Remove and cleanup the easy handle */
		curl_multi_remove_handle(dl->multi, curr->curl);
		curl_easy_cleanup(curr->curl);
		curr->curl = NULL;

		/* Allocate and format the filename string */
		filenamelen = snprintf(NULL, 0, "%s%d-%d-%d.png", TMP_DIR, curr->zoom, curr->x, curr->y);
		filename = malloc(filenamelen + 1);
		sprintf(filename, "%s%d-%d-%d.png", TMP_DIR, curr->zoom, curr->x, curr->y);

		/* Write the downloaded data to a file and free all chunks */
		fp = fopen(filename, "wb");
		writechunks(curr->chunk, fp);
		fclose(fp);
		free(filename);
		curr->chunk = NULL;

		/* Update the status */
		curr->status = AVAILABLE;
	}
}

static void
progress(Downloader *dl)
{
	TileTracker *curr;
	char *request;
	int requestlen, numrunning;

	/* Check the tracker for any downloads that need to be started */
	for(curr = dl->tracker; curr != NULL; curr = curr->next){
		if(curr->status == MISSING){
			/* Create the request string based on the tiles parameters */
			requestlen = snprintf(NULL, 0, "%s%d/%d/%d.png", TILE_URL, curr->zoom, curr->x, curr->y);
			request = malloc(requestlen + 1);
			sprintf(request, "%s%d/%d/%d.png", TILE_URL, curr->zoom, curr->x, curr->y);

			/* Create the easy handle and add it to the multi handle */
			curr->curl = curl_easy_init();
			curl_easy_setopt(curr->curl, CURLOPT_URL, request);
			curl_easy_setopt(curr->curl, CURLOPT_USERAGENT, USER_AGENT);
			curl_easy_setopt(curr->curl, CURLOPT_WRITEFUNCTION, storechunk);
			curl_easy_setopt(curr->curl, CURLOPT_WRITEDATA, curr);
			curl_multi_add_handle(dl->multi, curr->curl);

			/* Free the request string and update the tile status */
			free(request);
			curr->status = PENDING;
		}
	}
	/* Progress the pending downloads */
	curl_multi_perform(dl->multi, &numrunning);
}

void
dlsync(Downloader *dl)
{
	handlemessages(dl);
	progress(dl);
}

void
tilecoords(double lon, double lat, int zoom, int *tx, int *ty, int *px, int *py)
{
	double x, y, i;

	/* Convert lat and lon from degrees to radians */
	lat = (lat * M_PI) / 180;
	lon = (lon * M_PI) / 180;

	/* Reproject to the Spherical Mercator Projection */
	x = lon;
	y = log(tan(lat) + (1 / cos(lat)));

	/* Transform (x,y) onto the unit square */
	x = 0.5 + (x / (2 * M_PI));
	y = 0.5 - (y / (2 * M_PI));

	/* Calculate tile and pixel indices */
	*px = modf(x * pow(2, zoom), &i) * TILE_DIMENSION;
	*tx = i;
	*py = modf(y * pow(2, zoom), &i) * TILE_DIMENSION;
	*ty = i;
}

void
latlon(int tx, int ty, int px, int py, int zoom, double *lon, double *lat)
{
	double x, y;

	/* Reconstruct the Spherical Mercator Projection from the tile and pixel indices */
	x = (tx + ((double)px / TILE_DIMENSION)) / pow(2, zoom);
	y = (ty + ((double)py / TILE_DIMENSION)) / pow(2, zoom);

	/* Transform (x,y) onto unit sphere */
	x = (2 * M_PI) * (x - 0.5);
	y = (-2 * M_PI) * (y - 0.5);

	/* Reproject to EPSG:4326 in radians */
	*lon = x;
	*lat = asin(-pow(1.0 + exp(2.0 * y), -0.5)) + atan(exp(y));

	/* Convert lat and lon from radians to degrees */
	*lon = (180 * *lon) / M_PI;
	*lat = (180 * *lat) / M_PI;
}