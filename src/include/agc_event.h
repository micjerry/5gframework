#ifndef AGC_EVENT_H
#define AGC_EVENT_H

#include <agc.h>


struct agc_event_header {
	/*! the header name */
	char *name;
	/*! the header value */
	char *value;
	/*! array space */
	char **array;
	/*! array index */
	int idx;
	/*! hash of the header name */
	unsigned long hash;
	struct agc_event_header *next;
};

struct agc_event {
	/*! the event id (descriptor) */
	agc_event_types_t event_id;
	/*! the owner of the event */
	char *owner;
	/*! the event headers */
	agc_event_header *headers; 
	/*! the body of the event */
	char *body;
	/*! unique key */
	unsigned long key;
	struct agc_event *next;
};


#endif
