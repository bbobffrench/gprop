#include "config.h"
#include "chunk.h"
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
storechunk(char *data, size_t size, size_t nmemb, void *ptr)
{
	addchunk((Chunk **)ptr, data, size * nmemb);
	return size * nmemb;
}

static void
parseresponse(OSMSearch **search, char *data)
{
	cJSON *json, *iter, *field;
	char *name;
	double lon, lat;

	json = cJSON_Parse(data);
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
}

OSMSearch *
geosearch(char *query)
{
	CURL *curl;
	char *escaped, *request, *data;
	int requestlen;
	OSMSearch *res;
	Chunk *chunk;

	curl = curl_easy_init();

	escaped = curl_easy_escape(curl, query, 0);
	requestlen = snprintf(NULL, 0, "%sq=%s&format=json", NOMINATIM_URL, escaped);
	request = malloc(requestlen + 1);
	sprintf(request, "%sq=%s&format=json", NOMINATIM_URL, escaped);

	/* Retry until the response is received successfully */
	chunk = NULL;
	res = NULL;
	while(1){
		curl_easy_setopt(curl, CURLOPT_URL, request);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, storechunk);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		if(curl_easy_perform(curl) == CURLE_OK){
			data = concatchunks(chunk);
			break;
		} else{
			freechunks(chunk);
			chunk = NULL;
		}
	}
	parseresponse(&res, data);

	free(data);
	free(request);
	curl_free(escaped);
	curl_easy_cleanup(curl);

	return res;
}
