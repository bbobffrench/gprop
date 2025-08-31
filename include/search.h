#ifndef SEARCH_H
#define SEARCH_H

typedef struct OSMSearch OSMSearch;
struct OSMSearch{
	char *name;
	double lon;
	double lat;
	OSMSearch *next;
};

void
freesearch(OSMSearch *);

OSMSearch *
geosearch(char *);

#endif