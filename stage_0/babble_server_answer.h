#ifndef __BABBLE_SERVER_ANSWER_H__
#define __BABBLE_SERVER_ANSWER_H__


/* an answer msg */
typedef struct answer_msg{
    void *buf; /* the data to send */
    size_t size; /* the size of buf in bytes */
    struct answer_msg *next; /* Since a request can require multiple msgs
                          * in the answer (eg, answer to a timeline
                          * msg), we add the possibility to chain
                          * answers in a list */
} answer_msg_t;

/* a answer to one client command; it can include a list of answer_msg */
typedef struct answer{
    unsigned long key; /* key of the target client */
    unsigned int nb_items; /* nb of msgs in the answer */
    answer_msg_t *first; /* first msg in the answer */
} answer_t;

answer_t* alloc_answer(unsigned long key);
void free_answer(answer_t *answer);
void add_msg_to_answer(answer_t *answer, size_t buf_size, void *buf);

/* the answer is self-contained, it includes all information necessary
 * to send the data to the client */
int send_answer_to_client(answer_t * answer);

#endif /* __BABBLE_SERVER_ANSWER_H__ */
