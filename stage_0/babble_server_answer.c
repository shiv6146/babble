#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "babble_server_answer.h"
#include "babble_server.h"

answer_t* alloc_answer(unsigned long key)
{
    answer_t *a = (answer_t*) malloc(sizeof(answer_t));

    a->key = key;
    a->nb_items = 0;
    a->first = NULL;

    return a;
}

void free_answer(answer_t *answer)
{
    /* If the answer is empty, there is nothing to free */
    if(!answer){
        return ;
    }

    int count = 0;
    answer_msg_t *iter = answer->first, *next=NULL;

    
    while(iter != NULL){
        next = iter->next;
        free(iter->buf);
        free(iter);
        iter = next;
        count++;
    }

    assert(count == answer->nb_items);

    free(answer);
}

void add_msg_to_answer(answer_t *answer, size_t buf_size, void *buf)
{
    answer_msg_t *iter=NULL;
    
    /* the new msg */
    answer_msg_t *new_msg = malloc(sizeof(answer_msg_t));
    new_msg->buf = malloc(buf_size);
    new_msg->size = buf_size;
    new_msg->next = NULL;

    memcpy(new_msg->buf, buf, buf_size);

    /* case of first msg */
    if(answer->first == NULL){
        answer->first = new_msg;
    }
    else{
        /* get to the last in the list */
        iter = answer->first;
        
        while(iter != NULL){
            if(iter->next == NULL){
                break;
            }
            iter=iter->next;
        }

        /* add the new msg to the answer */
        iter->next = new_msg;
    }
    
    answer->nb_items++;    
}


int send_answer_to_client(answer_t * answer)
{
    /* If the answer is empty, there is nothing to send */
    if(!answer){
        return 0;
    }
    
    /* we send a first message with the size of the answer */
    if(write_to_client(answer->key, sizeof(unsigned int), &answer->nb_items)){
        fprintf(stderr,"Error -- could not send message size to client %lu\n", answer->key);
        return -1;
    }

    /* we iterate over the messages and send them on by one */
    answer_msg_t *iter = answer->first;
    
    while(iter != NULL ){
        if(write_to_client(answer->key, iter->size, iter->buf)){

            fprintf(stderr,"Error -- could not send a msg to client %lu\n", answer->key);
            return -1;
        }
        iter = iter->next;
    }

    return 0;
}
