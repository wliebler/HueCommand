#include <stdio.h>
#define responseBufferSize 10
#include <stdlib.h>
#include <unistd.h>
#include "requestHandler.h"
#include <string.h>
//Stores the reponse in chunks

struct responseStruct {
    char * rep;
    long long size;
};

int numOfResponses = 0;
char * requestHandler_response;
long long requestHandler_finalSize;
struct responseStruct *allResponses[responseBufferSize];


//Concats our repsonses into a single responseStruct
int concatResponseStructs(){
    fflush(stdout);
    long totalSize = 1;
    for(int i = 0; i < numOfResponses; i++){
        totalSize += allResponses[i]->size;
    }
    if(totalSize == 0){
        return -1;
    }
    fflush(stdout);
    char* newRep = malloc(totalSize * sizeof(char));
    strcpy(newRep,"");
    for(int i = 0; i < numOfResponses; i++){
        strcat(newRep,(allResponses[i]->rep));
    }
    //So we have our new size and rep
    struct responseStruct *newRepsonse = malloc(sizeof(struct responseStruct));
    newRepsonse->rep=newRep;
    newRepsonse->size = totalSize;
    //Freeing data
    for(int i = 0; i < numOfResponses; i++){
        free(allResponses[i]->rep);
        free(allResponses[i]);
    }
    allResponses[0] = newRepsonse;
    numOfResponses = 1;
    return 0;
}
int handleRequest(char *ptr, size_t size, size_t nmemb, void *userp){
    struct responseStruct* newResponse = malloc(sizeof(struct responseStruct));
    char* responseString = malloc((nmemb+1) * sizeof(char)); 
    strcpy(responseString,ptr);
    newResponse->rep = responseString;
    newResponse->size = nmemb+1;
    if(numOfResponses == responseBufferSize){
        concatResponseStructs();
    }

    allResponses[numOfResponses] = newResponse;
    numOfResponses += 1;
    return nmemb;
}
//Moves required info from responses into response and final size
int finishRequest(){
    //Checking response, 0 = Good, 1 = no final size
    int check = concatResponseStructs();
    if(check!= 0){
        requestHandler_finalSize = 0;
        requestHandler_response = "No Response";
        return 1;
    }
        //Allocating enough space for the final repsonse to be saved in
    requestHandler_finalSize = allResponses[0]->size;
    requestHandler_response = (char *)malloc(((size_t)(requestHandler_finalSize * sizeof(char))));

    strcpy(requestHandler_response,allResponses[0]->rep);
    //Freeing up this response now
    free(allResponses[0]->rep);
    free(allResponses[0]);
    numOfResponses = 0;
    return 0;
}
