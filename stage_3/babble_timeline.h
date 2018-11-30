#ifndef __BABBLE_TIMELINE_H__
#define __BABBLE_TIMELINE_H__

#include <time.h>
#include <inttypes.h>

#include "babble_config.h"
#include "babble_server_answer.h"
#include "babble_types.h"

/* a publication */
typedef struct publication{
    char msg[BABBLE_BUFFER_SIZE];
    time_t date;
} publication_t;


/* the timeline */
/* it is implemented as a circular buffer of fixed size */
typedef struct timeline{
    publication_t circular_buffer[BABBLE_TIMELINE_MAX];
    unsigned int youngest; /* index of the most recent message */
    unsigned int count_recent_adds; /* count the numbers of inserts
                                     * since the last summary */
    unsigned long key; /* key of associated client */
}timeline_t;

/* instanciate a new timeline */
timeline_t* timeline_create(unsigned long client_key);
void timeline_free(timeline_t *timeline);

/* inserts msg in the timeline tm */
time_t timeline_insert(timeline_t *tm, client_bundle_t *publisher, char *msg);

/* generates a timeline answer */
void timeline_generate_summary(timeline_t *tm, answer_t** answer);

#endif
