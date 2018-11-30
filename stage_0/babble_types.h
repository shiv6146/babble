#ifndef __BABBLE_TYPES_H__
#define __BABBLE_TYPES_H__

#include <time.h>

#include "babble_config.h"

/* forward declaration, defined in babble_timeline.h */
struct timeline;

typedef enum{
    LOGIN =0,
    PUBLISH,
    FOLLOW,
    TIMELINE,
    FOLLOW_COUNT,
    RDV,
    UNREGISTER
} command_id;

typedef struct command{
    command_id cid;
    int sock;    /* only needed by the LOGIN command, other commands
                  * will use the key */
    unsigned long key;
    char msg[BABBLE_SIZE];
    int answer_expected;   /* answer sent only if set */
} command_t;

typedef struct client_bundle{
    unsigned long key;     /* hash of the name */
    char client_name[BABBLE_ID_SIZE];    /* name as provided by the
                                          * client */
    int sock;              /* socket to communicate with this client */
    struct timeline *timeline;   /* timeline of the client */
    struct client_bundle *followers[MAX_CLIENT];  /* key of the followers */
    unsigned int nb_followers;
    unsigned int disconnected; /* set to 1 when client has disconnected */
} client_bundle_t;


#endif
