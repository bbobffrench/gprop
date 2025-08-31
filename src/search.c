#include "config.h"
#include "search.h"

#include <curl/curl.h>
#include <cjson/cJSON.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
addsearchres(OSMSearch **search, char *name, double lon, double lat)
{
	OSMSearch *curr, *new;

	new = malloc(sizeof(OSMSearch));
	new->name = name;
	new->lon = lon;
	new->lat = lat;
	new->next = NULL;

	if(*search == NULL) *search = new;
	else {
		for(curr = *search; curr->next != NULL; curr = curr->next);
		curr->next = new;
	}
}

void
freesearch(OSMSearch *search)
{
	OSMSearch *curr, *next;

	for(curr = search; curr != NULL; curr = next){
		next = curr->next;
		free(curr->name);
		free(curr);
	}
}

static size_t
writesearch(char *data, size_t size, size_t nmemb, void *ptr)
{
	OSMSearch **search;
	cJSON *json, *iter, *field;
	char *name;
	double lon, lat;

	json = cJSON_ParseWithLength(data, size * nmemb);
	search = (OSMSearch **)ptr;
	cJSON_ArrayForEach(iter, json){
		field = cJSON_GetObjectItemCaseSensitive(iter, "display_name");
		name = malloc(strlen(field->valuestring) + 1);
		strcpy(name, field->valuestring);

		field = cJSON_GetObjectItemCaseSensitive(iter, "lon");
		lon = atof(field->valuestring);

		field = cJSON_GetObjectItemCaseSensitive(iter, "lat");
		lat = atof(field->valuestring);

		addsearchres(search, name, lon, lat);
	}
	cJSON_Delete(json);
	return size * nmemb;
}

OSMSearch *
geosearch(char *query)
{
	CURL *curl;
	char *escaped, *request;
	int requestlen;
	OSMSearch *res;

	curl = curl_easy_init();

	escaped = curl_easy_escape(curl, query, 0);
	requestlen = snprintf(NULL, 0, "%sq=%s&format=json", NOMINATIM_URL, escaped);
	request = malloc(requestlen + 1);
	sprintf(request, "%sq=%s&format=json", NOMINATIM_URL, escaped);

	curl_easy_setopt(curl, CURLOPT_URL, request);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writesearch);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&res);

	res = NULL;
	curl_easy_perform(curl);

	free(request);
	curl_free(escaped);
	curl_easy_cleanup(curl);

	return res;
}
