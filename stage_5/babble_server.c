#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>

#include "babble_server.h"
#include "babble_types.h"
#include "babble_utils.h"
#include "babble_communication.h"
#include "babble_server_answer.h"
#include "babble_registration.h"

command_t *cmd_buf[BABBLE_PRODCONS_SIZE];
answer_t *ans_buf[BABBLE_PRODCONS_SIZE];
sem_t empty_count, full_count, mutex, ans_emp_cnt, ans_full_cnt, ans_mutex;
int in, out, ans_in, ans_out = 0;

static void display_help(char *exec)
{
    printf("Usage: %s -p port_number\n", exec);
}


static int parse_command(char* str, command_t *cmd)
{
    char *name = NULL;
    
    /* start by cleaning the input */
    str_clean(str);
    
    /* get command id */
    cmd->cid=str_to_command(str, &cmd->answer_expected);
    
    switch(cmd->cid){
    case LOGIN:
        if(str_to_payload(str, cmd->msg, BABBLE_ID_SIZE)){
            name = get_name_from_key(cmd->key);
            fprintf(stderr,"Error from [%s]-- invalid LOGIN -> %s\n", name, str);
            free(name);
            return -1;
        }
        break;
    case PUBLISH:
        if(str_to_payload(str, cmd->msg, BABBLE_SIZE)){
            name = get_name_from_key(cmd->key);
            fprintf(stderr,"Warning from [%s]-- invalid PUBLISH -> %s\n", name, str);
            free(name);
            return -1;
        }
        break;
    case FOLLOW:
        if(str_to_payload(str, cmd->msg, BABBLE_ID_SIZE)){
            name = get_name_from_key(cmd->key);
            fprintf(stderr,"Warning from [%s]-- invalid FOLLOW -> %s\n", name, str);
            free(name);
            return -1;
        }
        break;
    case TIMELINE:
        cmd->msg[0]='\0';
        break;
    case FOLLOW_COUNT:
        cmd->msg[0]='\0';
        break;
    case RDV:
        cmd->msg[0]='\0';
        break;    
    default:
        name = get_name_from_key(cmd->key);
        fprintf(stderr,"Error from [%s]-- invalid client command -> %s\n", name, str);
        free(name);
        return -1;
    }

    return 0;
}


/* processes the command and eventually generates an answer */
static int process_command(command_t *cmd, answer_t **answer)
{
    int res=0;

    switch(cmd->cid){
    case LOGIN:
        res = run_login_command(cmd, answer);
        break;
    case PUBLISH:
        res = run_publish_command(cmd, answer);
        break;
    case FOLLOW:
        res = run_follow_command(cmd, answer);
        break;
    case TIMELINE:
        res = run_timeline_command(cmd, answer);
        break;
    case FOLLOW_COUNT:
        res = run_fcount_command(cmd, answer);
        break;
    case RDV:
        res = run_rdv_command(cmd, answer);
        break;
    default:
        fprintf(stderr,"Error -- Unknown command id\n");
        return -1;
    }

    if(res){
        fprintf(stderr,"Error -- Failed to run command ");
        display_command(cmd, stderr);
    }

    return res;
}

// Command executor thread method
void *execute_command(void *ignored) {

    answer_t *answer = NULL;

    while(1) {
        
        sem_wait(&full_count);
        sem_wait(&mutex);

        command_t* next_cmd = cmd_buf[out];
        out = (out + 1) % BABBLE_PRODCONS_SIZE;

        if (next_cmd != NULL && next_cmd->cid >= PUBLISH && next_cmd->cid < UNREGISTER) {
            client_bundle_t *tmp = registration_lookup(next_cmd->key);
            if (tmp != NULL && tmp->disconnected == 0) {
                if(process_command(next_cmd, &answer) == -1){
                    fprintf(stderr, "Warning: unable to process command from client %lu\n", next_cmd->key);
                }
                next_cmd->processed = 1;
            }
        }

        sem_post(&mutex);
        sem_post(&empty_count);

        // Using ans_expected attribute in cmd struct to exclude cmds that does not expect an ans
        // Handling streaming scenario
        if (next_cmd != NULL && next_cmd->answer_expected) {
            // Produce ans for cmd and put into ans_buf
            sem_wait(&ans_emp_cnt);
            sem_wait(&ans_mutex);

            ans_buf[ans_in] = answer;
            ans_in = (ans_in + 1) % BABBLE_PRODCONS_SIZE;

            sem_post(&ans_mutex);
            sem_post(&ans_full_cnt);

        }
    }
}

// Answer handler thread method
void *handle_answer(void *ignored) {
    
    while(1) {

        sem_wait(&ans_full_cnt);
        sem_wait(&ans_mutex);

        // Read ans from ans_buf
        answer_t *ans = ans_buf[ans_out];
        ans_out = (ans_out + 1) % BABBLE_PRODCONS_SIZE;

        // Send the ans to client on respective client_sock_fd
        if(send_answer_to_client(ans) == -1){
            fprintf(stderr, "Warning: unable to answer command from client %lu\n", ans->key);
        }

        sem_post(&ans_mutex);
        sem_post(&ans_emp_cnt);

        

    }
}

/* Client thread method */
void *process_client(void *client_sock) {

    int newsockfd = (int)client_sock;
    char* recv_buff=NULL;
    int recv_size=0;
    
    unsigned long client_key=0;
    char client_name[BABBLE_ID_SIZE+1];

    command_t *cmd;
    answer_t *answer=NULL;

    memset(client_name, 0, BABBLE_ID_SIZE+1);

    
    if((recv_size = network_recv(newsockfd, (void**)&recv_buff)) < 0){
        
        fprintf(stderr, "Error -- recv from client\n");
        close(newsockfd);
        return NULL;
    }
    
    cmd = new_command(0);

    if(parse_command(recv_buff, cmd) == -1 || cmd->cid != LOGIN){
        fprintf(stderr, "Error -- in LOGIN message\n");
        close(newsockfd);
        free(cmd);
        return NULL;
    }

    /* before processing the command, we should register the
        * socket associated with the new client; this is to be done only
        * for the LOGIN command */
    cmd->sock = newsockfd;

    if(process_command(cmd, &answer) == -1){
        fprintf(stderr, "Error -- in LOGIN\n");
        close(newsockfd);
        free(cmd);
        return NULL;
    }

    /* notify client of registration */
    if(send_answer_to_client(answer) == -1){
        fprintf(stderr, "Error -- in LOGIN ack\n");
        close(newsockfd);
        free(cmd);
        free_answer(answer);
        return NULL;
    }
    else{
        free_answer(answer);
    }

    /* let's store the key locally */
    client_key = cmd->key;
    
    strncpy(client_name, cmd->msg, BABBLE_ID_SIZE);
    free(recv_buff);
    free(cmd);

    /* looping on client commands */
    while((recv_size=network_recv(newsockfd, (void**) &recv_buff)) > 0){
        cmd = new_command(client_key);
        

        if(parse_command(recv_buff, cmd) == -1){
            fprintf(stderr, "Warning: unable to parse message from client %s\n", client_name);
            notify_parse_error(cmd, recv_buff, &answer);
            send_answer_to_client(answer);
            free_answer(answer);
            free(cmd);
        }
        else{

            sem_wait(&empty_count);
            sem_wait(&mutex);

            if (cmd->cid >= PUBLISH && cmd->cid < UNREGISTER) {
                cmd_buf[in] = cmd;
                in = (in + 1) % BABBLE_PRODCONS_SIZE;
            }

            sem_post(&mutex);
            sem_post(&full_count);
        }
        free(recv_buff);
    }

    if(client_name[0] != 0){
        cmd = new_command(client_key);
        cmd->cid= UNREGISTER;
        
        if(unregisted_client(cmd)){
            fprintf(stderr,"Warning -- failed to unregister client %s\n",client_name);
        }
        free(cmd);
    }

    return NULL;

}

int main(int argc, char *argv[])
{
    int sockfd;
    int portno=BABBLE_PORT;
    
    int opt;
    int nb_args=1;
    int nb_client_threads=0;
    pthread_t *clients, *executors, *answer_handlers;

    // Init semaphores
    sem_init(&empty_count, 0, BABBLE_PRODCONS_SIZE);
    sem_init(&full_count, 0, 0);
    sem_init(&mutex, 0, 1);

    sem_init(&ans_emp_cnt, 0, BABBLE_PRODCONS_SIZE);
    sem_init(&ans_full_cnt, 0, 0);
    sem_init(&ans_mutex, 0, 1);

    // Initialize the thread pool
    clients = malloc(MAX_CLIENT * sizeof(pthread_t));
    executors = malloc(BABBLE_EXECUTOR_THREADS * sizeof(pthread_t));
    answer_handlers = malloc(BABBLE_ANSWER_THREADS * sizeof(pthread_t));
    
    while ((opt = getopt (argc, argv, "+p:")) != -1){
        switch (opt){
        case 'p':
            portno = atoi(optarg);
            nb_args+=2;
            break;
        case 'h':
        case '?':
        default:
            display_help(argv[0]);
            return -1;
        }
    }
    
    if(nb_args != argc){
        display_help(argv[0]);
        return -1;
    }

    server_data_init();

    if((sockfd = server_connection_init(portno)) == -1){
        return -1;
    }

    printf("Babble server bound to port %d\n", portno);

    // Create new thread for command executor
    int i;
    for (i = 0; i < BABBLE_EXECUTOR_THREADS; i++) {
        if (pthread_create(&executors[i], NULL, execute_command, NULL)) {
            printf("Server unable to create new thread for command executor!\n");
            return -1;
        }
    }

    // Create threads for answer_handlers
    for (i = 0; i < BABBLE_ANSWER_THREADS; i++) {
        if (pthread_create(&answer_handlers[i], NULL, handle_answer, NULL)) {
            printf("Server unable to create new thread for answer handler!\n");
            return -1;
        }
    }

    
    /* main server loop */
    while(1){
        
        int newsockfd;

        if((newsockfd = server_connection_accept(sockfd)) == -1){
            printf("Server unable to accept client connections!\n");
            return -1;
        }

        // Create new thread for client
        if (pthread_create(&clients[nb_client_threads], NULL, process_client, (void *)newsockfd)) {
            printf("Server unable to create new threads for clients!\n");
            return -1;
        }
        nb_client_threads++;
    }

    close(sockfd);
    return 0;
}
