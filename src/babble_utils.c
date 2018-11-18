#include "babble_utils.h"
#include "babble_types.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "babble_registration.h"

/* split the string according to BABBLE_DELIMITER */
/* returns an array of char* with a copy of each item found */
/* number of items is stored in nb_found */
static char** split_string(char* str, int* nb_found)
{
    int count=0;
    int start=0;
    int i=0;

    char **result=NULL;
    
    for(i=0; i< strlen(str); i++){
        if(!strncmp(&str[i], BABBLE_DELIMITER, 1)){
            if(i-start > 0){
                count++;
                result = realloc(result, sizeof(char*)*count);
                char* new_item = malloc(sizeof(char*) * BABBLE_BUFFER_SIZE);
                memset(new_item, 0, BABBLE_BUFFER_SIZE);
                strncpy(new_item, &str[start], i-start);
                result[count-1]=new_item;
            }
            start = i+1;
        }
    }

    /* create final item */
    if(strlen(str)-start > 0){
        count++;
        result = realloc(result, sizeof(char*)*count);
        char* new_item = malloc(sizeof(char*) * BABBLE_BUFFER_SIZE);
        memset(new_item, 0, BABBLE_BUFFER_SIZE);
        strncpy(new_item, &str[start], strlen(str)-start);
        result[count-1]=new_item;
    }
    
    *nb_found = count;

    return result;
}

/* free an array generated by split_string() */
static void free_split_array(char** array, int size)
{
    int i=0;

    for(i=0; i<size; i++){
        free(array[i]);
    }

    free(array);
}


unsigned long hash(char *str){
    unsigned long hash = 5381;
    int c;
    
    while ((c = *str++) != 0){
        hash = ((hash << 5) + hash) + c;
    }
        
    return hash;
}

int str_to_command(char* str, int* ack_req)
{
    int nb_items=0;
    char** items=split_string(str, &nb_items);

    int cid_index=0;

    
    if(nb_items == 0){
        fprintf(stderr,"Error -- invalid request -> %s\n", str);
        return -1;
    }
    
    if(strlen(items[0]) == 1 && items[0][0] == 'S'){
        *ack_req=0;
        cid_index=1;
    }
    else{
        *ack_req=1;
    }    
 
    
    if(strlen(items[cid_index]) == 1){
        errno=0;
        int res = (int)strtol(items[cid_index], NULL, 10);

        if(errno || (res == 0 && *items[cid_index]!='0')){
            //fprintf(stderr,"Error -- invalid request -> %s\n", str);
            free_split_array(items, nb_items);
            return -1;
        }
        
        
        if( res < LOGIN || res > RDV){
            //fprintf(stderr,"Error -- invalid request -> %s\n", str);
            free_split_array(items, nb_items);
            return -1;
        }

        if(res == LOGIN || res == TIMELINE || res == FOLLOW_COUNT || res == RDV){
            if(*ack_req == 0){
                //fprintf(stderr,"Error -- invalid request -> %s\n", str);
                free_split_array(items, nb_items);
                return -1;
            }
        }

        free_split_array(items, nb_items);
        return res;
    }
    
    if(!strcmp(items[cid_index], "LOGIN")){
        free_split_array(items, nb_items);
        if(*ack_req == 0){
            //fprintf(stderr,"Error -- invalid request -> %s\n", str);
            return -1;
        }
        return LOGIN;
    }

    if(!strcmp(items[cid_index], "PUBLISH")){
        free_split_array(items, nb_items);
        return PUBLISH;
    }

    if(!strcmp(items[cid_index], "FOLLOW")){
        free_split_array(items, nb_items);
        return FOLLOW;
    }

    if(!strcmp(items[cid_index], "TIMELINE")){
        free_split_array(items, nb_items);
        if(*ack_req == 0){
            //fprintf(stderr,"Error -- invalid request -> %s\n", str);
            return -1;
        }
        return TIMELINE;
    }

    if(!strcmp(items[cid_index], "FOLLOW_COUNT")){
        free_split_array(items, nb_items);
        if(*ack_req == 0){
            //fprintf(stderr,"Error -- invalid request -> %s\n", str);
            return -1;
        }
        return FOLLOW_COUNT;
    }

    if(!strcmp(items[cid_index], "RDV")){
        free_split_array(items, nb_items);
        return RDV;
    }

    free_split_array(items, nb_items);
    
    return -1;
}

int str_to_payload(char* input, char* output, int size)
{
    int nb_items=0;
    char **items=split_string(input, &nb_items);
    
    int p_index=1;

    if(strlen(items[0]) == 1 && items[0][0] == 'S'){
        p_index=2;
    }
    
    if(nb_items <= p_index){
        fprintf(stderr,"Error -- invalid payload -> %s\n", input);
        free_split_array(items, nb_items);
        return -1;
    }    
    
    int payload_size = strlen(items[p_index]);

    if(payload_size > size){
        payload_size = size;
        fprintf(stderr," Warning -- truncated msg");
    }

    memset(output, 0, size);
    strncpy(output, items[p_index], payload_size);

    free_split_array(items, nb_items);
    
    return 0;
}

/* cut str to \r or \n*/
void str_clean(char* str)
{
    char* found= strstr(str, "\r");
    if(found){
        *found='\0';
    }

    found= strstr(str, "\n");
    if(found){
        *found='\0';
    }
}

unsigned long parse_login_ack(char* ack_msg)
{
    char* key_part=strstr(ack_msg, "key");
    unsigned long key=0;

    if(key_part==NULL){
        return 0;
    }
    
    sscanf(key_part,"key %lu\n", &key);

    return key;
}


int parse_fcount_ack(char* ack)
{
    char* part=strstr(ack, "has");
    int nb_followers=0;

    if(part==NULL){
        return -1;
    }
    
    sscanf(part,"has %d followers\n", &nb_followers);

    return nb_followers;
}



