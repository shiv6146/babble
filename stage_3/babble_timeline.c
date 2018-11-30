#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "babble_timeline.h"
#include "babble_server.h"
#include "babble_communication.h"

timeline_t* timeline_create(unsigned long client_key)
{
    timeline_t* tm= malloc(sizeof(timeline_t));
    tm->youngest = 0;
    tm->count_recent_adds = 0;
    tm->key = client_key;
    
    return tm;
}

void timeline_free(timeline_t *timeline)
{
    free(timeline);
}


time_t timeline_insert(timeline_t *tm, client_bundle_t *publisher, char *msg)
{
    struct timespec tt;
    
    publication_t *pub= &tm->circular_buffer[tm->youngest];
    
    clock_gettime(CLOCK_REALTIME, &tt);
    
    memset(pub->msg, 0, BABBLE_SIZE);    
    strncpy(pub->msg, msg, BABBLE_SIZE);

    pub->date = tt.tv_sec - server_start;
    snprintf(pub->msg, BABBLE_BUFFER_SIZE,"    %s[%ld]: %s\n", publisher->client_name, pub->date, msg);
    
    /* shifting the index */
    tm->youngest = (tm->youngest + 1) % BABBLE_TIMELINE_MAX;

    tm->count_recent_adds++;
    
    return pub->date;
}

void timeline_generate_summary(timeline_t *tm, answer_t **answer)
{
    answer_t *the_answer=NULL;
    unsigned int index_first=0;

    the_answer = alloc_answer(tm->key);

    /* the first msg of the answer is the number of publications since
     * the last call to timeline */    
    add_msg_to_answer(the_answer, sizeof(unsigned int), &tm->count_recent_adds);

    /* compute the index of the first msg to add to the timeline */
    if(tm->count_recent_adds >= BABBLE_TIMELINE_MAX){
        index_first = tm->youngest;
        
        /* deal with the corner case where the buffer is full */
        add_msg_to_answer(the_answer, BABBLE_BUFFER_SIZE, &tm->circular_buffer[index_first]);
        index_first = (index_first + 1) % BABBLE_TIMELINE_MAX;
    }
    else{
        index_first = (BABBLE_TIMELINE_MAX + tm->youngest - tm->count_recent_adds) % BABBLE_TIMELINE_MAX;
    }
    
    /* add all new msgs in the timeline */
    while(index_first != tm->youngest ){
        add_msg_to_answer(the_answer, BABBLE_BUFFER_SIZE, &tm->circular_buffer[index_first]);

        index_first = (index_first + 1) % BABBLE_TIMELINE_MAX;
    }

    tm->count_recent_adds = 0;
    
    *answer = the_answer;
}
