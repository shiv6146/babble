#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "babble_client.h"
#include "babble_types.h"
#include "babble_communication.h"
#include "babble_utils.h"


void* recv_one_msg(int sock)
{
    unsigned int *nb_items;
    int recv_bytes=0;
    
    if((recv_bytes = network_recv(sock, (void**) &nb_items)) != sizeof(unsigned int)){
        fprintf(stderr, "ERROR in msg reception -- expected msg size -- received %d bytes\n", recv_bytes);
        return NULL;
    }

    if(*nb_items != 1){
        fprintf(stderr, "ERROR in msg reception -- a single msg expected -- %d annouced\n", *nb_items);
        free(nb_items);
        return NULL;
    }
    free(nb_items);
    
    char* msg=NULL;
    
    if(network_recv(sock, (void**) &msg) == -1){
        perror("ERROR reading from socket");
        return NULL;
    }

    return msg;
}

int recv_timeline_msg_and_print(int sock, int silent)
{
    unsigned int *buf1;
    unsigned int nb_items=0;
    unsigned int timeline_size=0;
    int recv_bytes=0;
    
    if((recv_bytes = network_recv(sock, (void**) &buf1)) != sizeof(unsigned int)){
        fprintf(stderr, "ERROR in msg reception -- expected msg size -- received %d bytes\n", recv_bytes);
        return -1;
    }
    nb_items = *buf1;
    free(buf1);

    /* first data is the number of msgs in the most recent timeline */
    if((recv_bytes = network_recv(sock, (void**) &buf1)) != sizeof(unsigned int)){
        fprintf(stderr, "ERROR in msg reception -- expected timeline size -- received %d bytes\n", recv_bytes);
        return -1;
    }
    timeline_size = *buf1;
    free(buf1);

    nb_items--;

    /* receive each publication in the timeline */
    while(nb_items){
        char* publi=NULL;
        
        if(network_recv(sock, (void**) &publi) == -1){
            return -1;
        }
        
        if(!silent){
            printf("%s", publi);
        }
        
        free(publi);
        
        nb_items--;
    }
    
   
    return timeline_size;
}



int connect_to_server(char* host, int port)
{
    /* creating the socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("ERROR opening socket");
        return -1;
    }

    /* connecting to the server */
    /*printf("Babble client connects to %s:%d\n", host, port);*/
    
    struct addrinfo hints, *results, *raddr;
    int res = 0;
    memset(&hints, 0, sizeof(struct addrinfo));
    char s_port[64];

    sprintf(s_port, "%d", port);
    
    hints.ai_family = AF_INET;

    res = getaddrinfo(host, s_port, &hints, &results);
    
    if (res) {
        perror("getaddrinfo");
        close(sockfd);
        return -1;
    }

    for (raddr = results; raddr != NULL; raddr = raddr->ai_next) {
        if (connect(sockfd, raddr->ai_addr, raddr->ai_addrlen) != -1){
            break;
        }
    }

   if (raddr == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        return -1;
    }

   freeaddrinfo(results);           /* No longer needed */

   return sockfd;
}


unsigned long client_login(int sock, char* id)
{
    char buffer[BABBLE_BUFFER_SIZE];
    memset(buffer, 0, BABBLE_BUFFER_SIZE);

    if(strlen(id) > BABBLE_ID_SIZE){
        fprintf(stderr,"Error -- invalid client id (too long): %s\n", id);
        fprintf(stderr,"Max id size is %d\n", BABBLE_ID_SIZE);
        return 0;
    }
    
    snprintf(buffer, BABBLE_BUFFER_SIZE, "%d %s\n", LOGIN, id);

    
    if (network_send(sock, strlen(buffer)+1, buffer) != strlen(buffer)+1){
        close(sock);
        return 0;
    }
    
    
    char* login_ack= recv_one_msg(sock);

    if(login_ack == NULL){
        close(sock);
        return 0;
    }
    
    /* parsing the answer to get the key */
    unsigned long key=parse_login_ack(login_ack);
    
    free(login_ack);
    
    return key;
}


int client_follow(int sock, char* id, int with_streaming)
{
    char buffer[BABBLE_BUFFER_SIZE];
    memset(buffer, 0, BABBLE_BUFFER_SIZE);

    if(strlen(id) > BABBLE_ID_SIZE){
        fprintf(stderr,"Error -- invalid client id (too long): %s\n", id);
        fprintf(stderr,"Max id size is %d\n", BABBLE_ID_SIZE);
        return -1;
    }

    if(with_streaming){
        snprintf(buffer, BABBLE_BUFFER_SIZE, "S %d %s\n", FOLLOW, id);
    }
    else{
        snprintf(buffer, BABBLE_BUFFER_SIZE, "%d %s\n", FOLLOW, id);
    }

    if (network_send(sock, strlen(buffer)+1, buffer) != strlen(buffer)+1){
        fprintf(stderr,"Error -- sending FOLLOW message\n");
        return -1;
    }

    if(!with_streaming){
        char* ack= recv_one_msg(sock);
        
        if(ack == NULL){
            fprintf(stderr, "ERROR in FOLLOW ack\n");
            close(sock);
            return -1;
        }

        /* check if answer is ok */
        if(strstr(ack, "follow")!=NULL){
            free(ack);
            return 0;
        }

        free(ack);
        return -1;
    }
    else{
        usleep(100);
    }


    return 0;
}


int client_follow_count(int sock)
{
    char buffer[BABBLE_BUFFER_SIZE];
    memset(buffer, 0, BABBLE_BUFFER_SIZE);

    snprintf(buffer, BABBLE_BUFFER_SIZE, "%d\n", FOLLOW_COUNT);

    if (network_send(sock, strlen(buffer)+1, buffer) != strlen(buffer)+1){
        fprintf(stderr,"Error -- sending FOLLOW_COUNT message\n");
        return -1;
    }

    char* count_ack=recv_one_msg(sock);
    
    if(count_ack == NULL){
        fprintf(stderr, "ERROR on FOLLOW_COUNT ack");
        close(sock);
        return 0;
    }
    
    /* parsing the answer to get the key */
    int count=parse_fcount_ack(count_ack);
    
    free(count_ack);
    
    return count;
}


int client_publish(int sock, char* msg, int with_streaming)
{
    char buffer[BABBLE_BUFFER_SIZE];
    memset(buffer, 0, BABBLE_BUFFER_SIZE);

    if(strlen(msg) > BABBLE_SIZE){
        fprintf(stderr,"Error -- invalid msg (too long): %s\n", msg);
        fprintf(stderr,"Max msg size is %d\n", BABBLE_SIZE);
        return -1;
    }

    if(with_streaming){
        snprintf(buffer, BABBLE_BUFFER_SIZE, "S %d %s\n", PUBLISH, msg);
    }
    else{
        snprintf(buffer, BABBLE_BUFFER_SIZE, "%d %s\n", PUBLISH, msg);
    }

    if (network_send(sock, strlen(buffer)+1, buffer) != strlen(buffer)+1){
        fprintf(stderr,"Error -- sending PUBLISH message\n");
        return -1;
    }

    if(!with_streaming){
        char* ack= recv_one_msg(sock);
        
        if(ack == NULL){
            fprintf(stderr, "ERROR in PUBLISH ack\n");
            close(sock);
            return -1;
        }

        /* check if answer is ok */
        if(strstr(ack, "{")!=NULL){
            free(ack);
            return 0;
        }

        free(ack);
        return -1;
    }
    else{
        usleep(1);
    }


    return 0;
}

/* return the size of the timeline */
/* if silent is set, do not print the timeline on the screen
 * return -1 in case of error */
int client_timeline(int sock, int silent)
{   
    char buffer[BABBLE_BUFFER_SIZE];
    memset(buffer, 0, BABBLE_BUFFER_SIZE);
    
    snprintf(buffer, BABBLE_BUFFER_SIZE, "%d\n", TIMELINE);

    if (network_send(sock, strlen(buffer)+1, buffer) != strlen(buffer)+1){
        fprintf(stderr,"Error -- sending TIMELINE message\n");
        return -1;
    }

    int total_items = recv_timeline_msg_and_print(sock, silent);
    
    if(total_items < 0){
        fprintf(stderr, "Error in timeline message\n");
        return -1;
    }

    return total_items;
}


int client_rdv(int sock)
{
    char buffer[BABBLE_BUFFER_SIZE];
    memset(buffer, 0, BABBLE_BUFFER_SIZE);

    snprintf(buffer, BABBLE_BUFFER_SIZE, "%d\n", RDV);
 
    if (network_send(sock, strlen(buffer)+1, buffer) != strlen(buffer)+1){
        fprintf(stderr,"Error -- sending RDV message\n");
        return -1;
    }
    
    char* ack=recv_one_msg(sock);
        
    if(ack == NULL){
        fprintf(stderr,"ERROR in RDV ack");
        close(sock);
        return -1;
    }

    /* check if answer is ok */
    if(strstr(ack, "rdv_ack")!=NULL){
        free(ack);
        return 0;
    }

    free(ack);
    return -1;
}
