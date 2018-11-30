#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "babble_registration.h"

client_bundle_t *registration_table[MAX_CLIENT];

// locks to solve reader-writer problems on registration_table
// pthread_mutex_t mutex, rwlock;
pthread_rwlock_t rt = PTHREAD_RWLOCK_INITIALIZER;
// sem_t rm, wm, rt, res;
int read_cnt, write_cnt = 0;

int nb_registered_clients;

void registration_init(void)
{
    nb_registered_clients=0;

    memset(registration_table, 0, MAX_CLIENT * sizeof(client_bundle_t*));
    // sem_init(&rm, 0, 1);
    // sem_init(&wm, 0, 1);
    // sem_init(&rt, 0, 1);
    // sem_init(&res, 0, 1);
    pthread_rwlock_init(&rt, NULL);

    // pthread_mutex_init(&mutex, NULL);
    // pthread_mutex_init(&rwlock, NULL);
}

client_bundle_t* registration_lookup(unsigned long key)
{
    // Grab read lock before lookup
    // pthread_mutex_lock(&mutex);

    // // Increment read_cnt
    // read_cnt++;

    // // Ensure no writers can enter CS when there is at least 1 reader
    // if (read_cnt == 1) pthread_mutex_lock(&rwlock);

    // // Readers can concurrently access when the reader is in CS
    // pthread_mutex_unlock(&mutex);
    pthread_rwlock_rdlock(&rt);

    // sem_wait(&rt);
    // sem_wait(&rm);
    // read_cnt++;
    // if (read_cnt == 1) sem_wait(&res);
    // sem_post(&rm);
    // sem_post(&rt);

    int i=0;
    client_bundle_t *c = NULL;
    
    for(i=0; i< nb_registered_clients; i++){
        if(registration_table[i]->key == key){
            c = registration_table[i];
            break;
        }
    }

    // sem_wait(&rm);
    // read_cnt--;
    // if (read_cnt == 0) sem_post(&res);
    // sem_post(&rm);
    pthread_rwlock_unlock(&rt);

    // Current reader has finished reading
    // pthread_mutex_lock(&mutex);

    // // Current reader has left CS
    // read_cnt--;

    // // If no reader is in CS then writer can enter
    // if (read_cnt == 0) pthread_mutex_unlock(&rwlock);

    // // Reader left CS
    // pthread_mutex_unlock(&mutex);

    return c;
}

int registration_insert(client_bundle_t* cl)
{    
    if(nb_registered_clients == MAX_CLIENT){
        fprintf(stderr, "ERROR: MAX NUMBER OF CLIENTS REACHED\n");
        return -1;
    }

    // sem_wait(&rt);
    // sem_wait(&rm);
    // read_cnt++;
    // if (read_cnt == 1) sem_wait(&res);
    // sem_post(&rm);
    // sem_post(&rt);

    // Grab read lock before lookup
    // pthread_mutex_lock(&mutex);

    // // Increment read_cnt
    // read_cnt++;

    // // Ensure no writers can enter CS when there is at least 1 reader
    // if (read_cnt == 1) pthread_mutex_lock(&rwlock);

    // // Readers can concurrently access when the reader is in CS
    // pthread_mutex_unlock(&mutex);
    pthread_rwlock_rdlock(&rt);
    /* lookup to find if key already exists */
    int i=0;
    
    for(i=0; i< nb_registered_clients; i++){
        if(registration_table[i]->key == cl->key){
            break;
        }
    }

    // Current reader has finished reading
    // pthread_mutex_lock(&mutex);

    // // Current reader has left CS
    // read_cnt--;

    // // If no reader is in CS then writer can enter
    // if (read_cnt == 0) pthread_mutex_unlock(&rwlock);

    // // Reader left CS
    // pthread_mutex_unlock(&mutex);

    if(i != nb_registered_clients){
        fprintf(stderr, "Error -- id % ld already in use\n", cl->key);
        return -1;
    }
    pthread_rwlock_unlock(&rt);

    // sem_wait(&rm);
    // read_cnt--;
    // if (read_cnt == 0) sem_post(&res);
    // sem_post(&rm);


    // Grab write lock before insert
    // pthread_mutex_lock(&rwlock);
    pthread_rwlock_wrlock(&rt);

    // sem_wait(&wm);
    // write_cnt++;
    // if (write_cnt == 1) sem_wait(&rt);
    // sem_post(&wm);

    // sem_wait(&res);
    /* insert cl */
    registration_table[nb_registered_clients]=cl;
    nb_registered_clients++;

    // sem_post(&res);

    // Release write lock after insert
    // pthread_mutex_unlock(&rwlock);
    pthread_rwlock_unlock(&rt);

    // sem_wait(&wm);
    // write_cnt--;
    // if (write_cnt == 0) sem_post(&rt);
    // sem_post(&wm);
    
    return 0;
}


client_bundle_t* registration_remove(unsigned long key)
{
    // sem_wait(&rt);
    // sem_wait(&rm);
    // read_cnt++;
    // if (read_cnt == 1) sem_wait(&res);
    // sem_post(&rm);
    // sem_post(&rt);
    // Grab read lock before lookup
    // pthread_mutex_lock(&mutex);

    // // Increment read_cnt
    // read_cnt++;

    // // Ensure no writers can enter CS when there is at least 1 reader
    // if (read_cnt == 1) pthread_mutex_lock(&rwlock);

    // // Readers can concurrently access when the reader is in CS
    // pthread_mutex_unlock(&mutex);
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

    // sem_wait(&rm);
    // read_cnt--;
    // if (read_cnt == 0) sem_post(&res);
    // sem_post(&rm);

    pthread_rwlock_unlock(&rt);

    // Current reader has finished reading
    // pthread_mutex_lock(&mutex);

    // // Current reader has left CS
    // read_cnt--;

    // // If no reader is in CS then writer can enter
    // if (read_cnt == 0) pthread_mutex_unlock(&rwlock);

    // // Reader left CS
    // pthread_mutex_unlock(&mutex);
    
    // Grab write lock before remove
    // pthread_mutex_lock(&rwlock);
    pthread_rwlock_wrlock(&rt);

    // sem_wait(&wm);
    // write_cnt++;
    // if (write_cnt == 1) sem_wait(&rt);
    // sem_post(&wm);

    // sem_wait(&res);

    nb_registered_clients--;
    registration_table[i] = registration_table[nb_registered_clients];

    // sem_post(&res);

    // Release write lock after remove
    // pthread_mutex_unlock(&rwlock);
    pthread_rwlock_unlock(&rt);
    // sem_wait(&wm);
    // write_cnt--;
    // if (write_cnt == 0) sem_post(&rt);
    // sem_post(&wm);

    return cl;
}
