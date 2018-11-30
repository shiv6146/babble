#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "babble_registration.h"

client_bundle_t *registration_table[MAX_CLIENT];

// lock to solve reader-writer problems on registration_table
pthread_rwlock_t rt = PTHREAD_RWLOCK_INITIALIZER;

int nb_registered_clients;

void registration_init(void)
{
    nb_registered_clients=0;

    memset(registration_table, 0, MAX_CLIENT * sizeof(client_bundle_t*));
    
    pthread_rwlock_init(&rt, NULL);

}

client_bundle_t* registration_lookup(unsigned long key)
{
    // Grab read lock before lookup
    
    pthread_rwlock_rdlock(&rt);


    int i=0;
    client_bundle_t *c = NULL;
    
    for(i=0; i< nb_registered_clients; i++){
        if(registration_table[i]->key == key){
            c = registration_table[i];
            break;
        }
    }

    pthread_rwlock_unlock(&rt);

    return c;
}

int registration_insert(client_bundle_t* cl)
{    
    if(nb_registered_clients == MAX_CLIENT){
        fprintf(stderr, "ERROR: MAX NUMBER OF CLIENTS REACHED\n");
        return -1;
    }


    // Grab read lock before lookup
    pthread_rwlock_rdlock(&rt);
    /* lookup to find if key already exists */
    int i=0;
    
    for(i=0; i< nb_registered_clients; i++){
        if(registration_table[i]->key == cl->key){
            break;
        }
    }


    if(i != nb_registered_clients){
        fprintf(stderr, "Error -- id % ld already in use\n", cl->key);
        return -1;
    }
    pthread_rwlock_unlock(&rt);



    // Grab write lock before insert
    pthread_rwlock_wrlock(&rt);

    /* insert cl */
    registration_table[nb_registered_clients]=cl;
    nb_registered_clients++;


    // Release write lock after insert
    pthread_rwlock_unlock(&rt);

    
    return 0;
}


client_bundle_t* registration_remove(unsigned long key)
{
    
    // Grab read lock before lookup
    pthread_rwlock_rdlock(&rt);

    int i=0;
    
    for(i=0; i<nb_registered_clients; i++){
        if(registration_table[i]->key == key){
            break;
        }
    }

    if(i == nb_registered_clients){
        fprintf(stderr, "Error -- no client found\n");
        return NULL;
    }

    client_bundle_t* cl= registration_table[i];


    pthread_rwlock_unlock(&rt);

    
    // Grab write lock before remove
    pthread_rwlock_wrlock(&rt);

    nb_registered_clients--;
    registration_table[i] = registration_table[nb_registered_clients];


    // Release write lock after remove
    pthread_rwlock_unlock(&rt);

    return cl;
}
