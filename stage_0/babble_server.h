#ifndef __BABBLE_SERVER_H__
#define __BABBLE_SERVER_H__

#include <stdio.h>

#include "babble_types.h"
#include "babble_server_answer.h"

/* server starting date */
extern time_t server_start;

/* pool of client threads */
extern pthread_t *clients;

/* init functions */
void server_data_init(void);
int server_connection_init(int port);
int server_connection_accept(int sock);

/* new object */
command_t* new_command(unsigned long key);

/* operations */
int run_login_command(command_t *cmd, answer_t **answer);
int run_publish_command(command_t *cmd, answer_t **answer);
int run_follow_command(command_t *cmd, answer_t **answer);
int run_timeline_command(command_t *cmd, answer_t **answer);
int run_fcount_command(command_t *cmd, answer_t **answer);
int run_rdv_command(command_t *cmd, answer_t **answer);

int unregisted_client(command_t *cmd);

/* display functions */
void display_command(command_t *cmd, FILE* stream);

/* error management */
int notify_parse_error(command_t *cmd, char *input, answer_t **answer);

/* high level comm function */
int write_to_client(unsigned long key, int size, void* buf);

/* get client name from client key */
char* get_name_from_key(unsigned long key);

#endif
