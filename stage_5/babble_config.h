#ifndef __BABBLE_CONFIG_H__
#define __BABBLE_CONFIG_H__

#define BABBLE_BACKLOG 100

#define BABBLE_PORT 5656
#define MAX_CLIENT 1000
#define MAX_FOLLOW MAX_CLIENT

#define BABBLE_BUFFER_SIZE 256
#define BABBLE_SIZE 64
#define BABBLE_ID_SIZE 16

#define BABBLE_DELIMITER " "

#define BABBLE_TIMELINE_MAX 4

#define BABBLE_EXECUTOR_THREADS 4
#define BABBLE_ANSWER_THREADS 4

#define BABBLE_PRODCONS_SIZE 10

#define BABBLE_CELEBRITY_THRESHOLD 100

#endif
