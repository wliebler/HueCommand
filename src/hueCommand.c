#include "cJSON/cJSON.h"
#include <asm-generic/errno-base.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include "hueCommand.h"
#include "requestHandler.h"
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define responseBufferSize 10
#define maxLightName 20
char ipAddress[50];
char authToken[256];

/*
 * Sets the values in the arguments struct to their default values
 */
int setDefaultArguments(struct arguments* args){
    args->debugMode = 0;
    args->printHelp = 0;
    args->target = -1;
    args->optionB = -1;
    args->argOnOff = -1;

    return 0;


}

//Returns 0 if successful, 1 otherwise
int stringToInt(char* s, int* i){
    char* end;
    int returnval = 0;
    long val = strtol(s,&end,10);
    if(val >= INT_MAX || val <= INT_MIN){
        val = 0;
        returnval = 1;
    }
    *i = (int)val;
    return returnval;
}

//0 is success
//1 is failed to convert string to int
//2 is not enough arguments
int intFromArgument(int argc, char* argv[], int startingArg, int* storage){
    if(argc > startingArg + 1){
        int succesVal = stringToInt(argv[startingArg+1],storage);
        return succesVal; 
    } 
    //Not enought args
    return 2;
}
int parseArgs(int argc, char *argv[],struct arguments* argStorage)
{
    setDefaultArguments(argStorage);
    //First setting all args to default values
    //Parsing extra arguments
    for (int i = 1; i < argc;)
    {
        //Incrimenting the loop will be done in here
        if (!strcmp(argv[i], "-v"))
        {
            //Verbose, prints debug information
            argStorage->debugMode = 1;
            printf("Debug mode enabled\n");
            //arg_debugMode = 1;
            i++;
        }
        else if (!strcmp(argv[i], "-h"))
        {
            argStorage->printHelp = 1;
            //arg_printHelpArg = 1;
            i++;
        }
        else if (!strcmp(argv[i], "-t"))
        {
            int j = intFromArgument(argc,argv,i,&argStorage->target);
                if(j == 0){
                    if(argStorage->debugMode){
                        printf("Target int set to %d\n",argStorage->target);
                    }
                    i += 2;
                }
                else if( j == 1){
                    printf("Issue parsing targeted value!\n");
                    i += 2;
                }
                else if(j == 2){
                    printf("Not enough arguments for -t argument!\n");
                    i += 1;
                }
        }
        else if(!strcmp(argv[i], "-b")){
            int j = intFromArgument(argc,argv,i,&argStorage->target);
            if(j == 0){
                i += 2;
            }
            else if(j == 1){
                i += 2;
                printf("Issue parsing targeted value!\n");
            }
            else if(j == 2){
                printf("Not enough arguments for -b argument!\n");
                i +=1;
            }
        }
        else if(!strcmp(argv[i],"off")){
            argStorage->argOnOff = 0;
            i++;
        }
        else if(!strcmp(argv[i],"on")){
            argStorage->argOnOff = 1;
            i++;
        }
        else
        {
            //Not an argument
            i++;
        }
    }
    return 0;
}
//Makes the following assumption
//Authtoken and ipAddress are loaded
//
int messageSender(char *messageType, char *header, char *body, char *targetPath)
{
    int returnCode = 0;
    CURL *newRequest = curl_easy_init();
    int addressLength = 60;
    addressLength += strlen(targetPath);
    //Allocating space for the target url
    char *targetUrl = malloc(addressLength);
    //Constructing our target url
    strcpy(targetUrl, ipAddress);
    strcat(targetUrl, "/api/");
    strcat(targetUrl, authToken);
    strcat(targetUrl, "/");
    strcat(targetUrl, targetPath);
    //printf("%s",targetUrl);
    fflush(stdout);
    struct curl_slist *headers = NULL;
    if (newRequest)
    {
        curl_easy_setopt(newRequest, CURLOPT_URL, targetUrl);
        curl_easy_setopt(newRequest, CURLOPT_WRITEFUNCTION, handleRequest);
        curl_slist_append(headers, header);
        curl_easy_setopt(newRequest, CURLOPT_CUSTOMREQUEST, messageType);
        curl_easy_setopt(newRequest, CURLOPT_POSTFIELDS, body);
        curl_easy_perform(newRequest);
        finishRequest();
        curl_easy_cleanup(newRequest);
    }
    else
    {
        printf("\n Error creating new request for message!\n");
        returnCode = -1;
    }
    free(targetUrl);
    return returnCode;
}
//All light commands
int lights_printCommands()
{
    printf("\n =====List of light commands=====");
    printf("\n| l = List all lights");
    printf("\n| s = Set the state of the light");
    printf("\n|\n| The following commands can be all used at the same time (with some exceptions). Make sure to select a target with -t <target number>");
    //printf("\n| Custom jSON can be sent with -j \"<JSON>\" all other commands will be added to the jSON sent");
    //printf("\n| Should two commands interer, the latest one in the command will take precendence\n|");
    printf("\n| on = Set target light on");
    printf("\n| off = Set target light off");
    printf("\n| -b <value> = Sets the brightness to the given value");
    printf("\n| -t <value> = Sets the light to target");
    printf("\n");
    return 0;
}

int setLightState(struct arguments* args)
{
    cJSON *body;
    //First grab arguments
    //Now creating our json object
    //First we check if we have been given a json object via the -j argument
    body = cJSON_CreateObject();
    //If setting the light on or off
    cJSON *newLightState;
    switch (args->argOnOff)
    {
        case 0:
            newLightState = cJSON_CreateBool(false);
            cJSON_AddItemToObject(body, "on", newLightState);
            break;
        case 1:
            newLightState = cJSON_CreateBool(true);
            cJSON_AddItemToObject(body, "on", newLightState);
            break;
        default:
            break;
    }
    if (args->optionB >= 0)
    {
        cJSON_AddItemToObject(body, "bri", cJSON_CreateNumber(args->optionB));
    }
    char address[50];
    int targetLength = snprintf(NULL,0,"%d",args->target);
    char* addressTarget = malloc(targetLength +1);
    snprintf(addressTarget,targetLength+1,"%d",args->target);
    strcpy(address, "lights/");
    strcat(address, addressTarget);
    strcat(address, "/state");
    messageSender("PUT", "Content-Type: application/json", cJSON_Print(body), address);
    if (args->debugMode)
    {
        printf("\nResponse from sending request to set state");
        printf("\n%s\n", requestHandler_response);
    }
    return 0;
}
//All args used for the light go here
//Used for priting out all the lights in the standard version
//Style is what syle we will print the details out in
int printListLightInfo(char *lightNum, cJSON *light, int style,struct arguments* args)
{
    //Limit to displaying the first 30 charcaters
    char lightName[maxLightName + 1];
    //First grabing the name of the light
    cJSON *jsonLightName = cJSON_GetObjectItem(light, "name");
    if (jsonLightName == NULL)
    {
        if (args->debugMode)
        {
            printf("DEBUG: Light is missing name!\n");
        }
    }
    else
    {
        if (args->debugMode)
        {
            printf("DEBUG: FULL LIGHT NAME IS %s\n", jsonLightName->valuestring);
        }
        strncpy(lightName, jsonLightName->valuestring, maxLightName);
        //Getting the length of the name
        int nameLength = strlen(lightName);
        for (int i = nameLength; i < 30; i++)
        {
            lightName[i] = ' ';
            //Padding the name with spaces
        }
        lightName[maxLightName] = '\0'; //Ensuring null termination
    }
    //Formatting the display for the number
    int numDisplayLength = strlen(lightNum);
    char numDisplay[5];
    //First we need to do some padding
    for (int i = 0; i < 4 - numDisplayLength; i++)
    {
        numDisplay[i] = ' ';
    }

    int j = 0;
    for (int i = 4 - numDisplayLength; i < 4; i++)
    {
        numDisplay[i] = lightNum[j];
        j++;
    }
    numDisplay[4] = '\0';
    //Getting light state
    char lightState[32];
    char brightnessDisplay[4];
    char reachableDisplay[32];
    cJSON *jsonLightState = cJSON_GetObjectItem(light, "state");
    if (jsonLightState != NULL)
    {
        cJSON *jsonLightOn = cJSON_GetObjectItem(jsonLightState, "on");
        if (jsonLightOn != NULL)
        {
            //JSON check
            if (cJSON_IsTrue(jsonLightOn))
            {
                strcpy(lightState, "\033[32;1mON\033[0m ");
            }
            else
            {
                strcpy(lightState, "\033[31mOFF\033[0m");
            }
            //If true, then this will be one
        }
        //Getting brightness
        cJSON *jsonBrighness = cJSON_GetObjectItem(jsonLightState, "bri");
        if (jsonBrighness != NULL)
        {
            char brighnessTmp[4];
            sprintf(brighnessTmp, "%d", jsonBrighness->valueint);
            int brightnessLength = strlen(brighnessTmp);
            for (int i = 0; i < 3 - brightnessLength; i++)
            {
                brightnessDisplay[i] = ' ';
            }
            int j = 0;
            for (int i = 3 - brightnessLength; i < 3; i++)
            {
                brightnessDisplay[i] = brighnessTmp[j];
                j++;
            }
            brightnessDisplay[3] = '\0';
        }
        else
        {
            if (args->debugMode)
            {
                printf("DEBUG issue getting brightness of light");
            }
            strcpy(brightnessDisplay, "ERR");
        }
        //Checking if the light is reachable
        cJSON *jsonReachable = cJSON_GetObjectItem(jsonLightState, "reachable");
        if (jsonReachable != NULL)
        {
            if (cJSON_IsTrue(jsonReachable))
            {
                //Light is reachable
                strcpy(reachableDisplay, "\033[1;32mYES\033[0m");
            }
            else
            {
                strcpy(reachableDisplay, "\033[1;31mNO\033[0m ");
            }
        }
    }

    else
    {
        if (args->debugMode)
        {
            printf("DEBUG: Issue getting state of light\n");
        }
    }

    //Doing the no args version first
    switch (style)
    {
        case 0:

            printf("%s | %s | %s | %s |   %s   |\n", numDisplay, lightName, lightState, brightnessDisplay, reachableDisplay);
            break;
    }
    return 0;
}
//Prints out the list of lights
//The style represents what style this will be printed in. 0 is default
int lights_list(int style,struct arguments* args)
{
    messageSender("GET", "Content-Type: application/json", "", "lights");
    printf("\nLights\n");
    cJSON *parsedResponse = cJSON_Parse(requestHandler_response);
    if (parsedResponse == NULL)
    {
        printf("!Issue parsing response form hue!");
        return -1;
    }
    //So the parse was good
    switch (style)
    {
        case 0:
            printf(" ID  | Light Name           |State| Bri |Reachable|\n");
            printf("-------------------------------------------------------\n");
            break;
        default:
            printf("\n Error with lising lights!");
            if (args->debugMode)
            {
                printf("\n Received %d as the style for printing the lights, which is not supported\n", style);
            }
            return -1;
            break;
    }
    if (parsedResponse->child != NULL)
    {
        //Grab the child JSON object
        cJSON *currentNode = parsedResponse->child;
        printListLightInfo(currentNode->string, currentNode, style,args);
        while (currentNode->next != NULL)
        {
            currentNode = currentNode->next;
            printListLightInfo(currentNode->string, currentNode, style,args);
        }
    }
    else
    {
        printf("\nNo lights appear to be connected to this hue\n!");
    }
    return 0;
}
int lights_Commands(int argc, char *argv[],struct arguments* args)
{
    if (argc <= 2 || args->printHelp)
    {
        lights_printCommands();
    }
    else
    {
        if (!strcmp(argv[2], "l"))
        {
            lights_list(0,args);
        }
        else if (!strcmp(argv[2], "s"))
        {
            setLightState(args);
        }
        else
        {
            lights_printCommands();
        }
    }

    return 0;
}
int printHelp()
{
    printf("\n\n=====List of all commands=====");
    printf("\n| l = All light commands");
    printf("\n| init-config = Peforms setup to retrieve authentiation token");
    printf("\n| To view more into about a command perform HueCommand <command> h\n");
    printf("\n\n ====List of extra arguments=====");
    printf("\n| -h Prints information about a command");
    printf("\n| -v Prints debug information");
    printf("\n| -t Sets the 'integer' target for commands. Use the int value for lights.");
    printf("\n");
    return 0;
}

int main(int argc, char *argv[])
{
    struct arguments args;
    parseArgs(argc, argv,&args);
    curl_global_init(CURL_GLOBAL_ALL);
    if (argc >= 2 && (!strcmp(argv[1], "init-config")))
    {
        if (args.printHelp)
        {
            printf("\n| Obtains an auth-token from the Hue Bridge to allow for requests to be authorised\n| Required for commands to function correctly\n");
            return 0;
        }
        else
        {
            initialConfig(&args);
            return 0;
        }
    }
    int loadConfigCode = loadConfigInformation(ipAddress, authToken);
    if (loadConfigCode == 0)
    {
        //Config load successful
        //Now to figure out which command was performed
        if (argc <= 1)
        {
            printHelp();
        }
        else
        {
            if (!strcmp(argv[1], "l"))
            {
                //Performing a light command
                lights_Commands(argc, argv,&args);
            }
            else
            {
                printHelp();
            }
        }
    }
    if (loadConfigCode == 1)
    {
        //Error loading ip
    }
    if (loadConfigCode == 2)
    {
        //Error loading authtoken
    }
    return 0;
}

int loadConfigInformation(char *ip, char *aToken)
{
    char *fileDir = getenv("HOME");
    strcat(fileDir, configPath);
    FILE *fPtr = fopen(fileDir, "r");
    fscanf(fPtr, "ip=%s\n", ip);
    fscanf(fPtr, "authToken=%s\n", aToken);
    if (!strcmp(ip, ""))
    {
        //So no ip loaded
        printf("ERROR, NO IP LOADED!\n");
        return 1;
    }
    if (!strcmp(aToken, ""))
    {
        //No token loaded
        printf("ERROR, NO AUTHTOKEN LOADED!\n");
        return 2;
    }
    return 0;
}

int obtainAuthToken(char *ip, char *tokenStorage,struct arguments* args)
{
    printf("Obtaining new auth token\n");
    //Constructing JSON format
    cJSON *body = cJSON_CreateObject();
    cJSON *bodyValue = cJSON_CreateString("hueCommand#user");
    cJSON_AddItemToObject(body, "devicetype", bodyValue);
    if (args->debugMode)
    {
        printf("JSON printing: \n %s", cJSON_Print(body));
        fflush(stdout);
    }
    CURL *authRequest = curl_easy_init();
    struct curl_slist *headers = NULL;
    int authRequestContinue = 1;
    char sendIp[55];
    strcpy(sendIp, ip);
    strcat(sendIp, "/api");
    while (authRequestContinue)
    {
        CURL *authRequest = curl_easy_init();
        if (authRequest)
        {

            if (args->debugMode)
            {
                printf("Ip address to send request to is: %s\n", sendIp);
            }
            curl_easy_setopt(authRequest, CURLOPT_URL, sendIp);
            curl_easy_setopt(authRequest, CURLOPT_WRITEFUNCTION, handleRequest);
            curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(authRequest, CURLOPT_CUSTOMREQUEST, "POST");
            curl_easy_setopt(authRequest, CURLOPT_POSTFIELDS, cJSON_Print(body));
            curl_easy_perform(authRequest);
            finishRequest();
            curl_easy_cleanup(authRequest);
            curl_slist_free_all(headers);
            if (args->debugMode)
            {
                printf("Finished with input");
                fflush(stdout);
                printf("\nResponse\n %s \n", requestHandler_response);
            }
            cJSON *parsedInput = cJSON_Parse(requestHandler_response);
            if (parsedInput == NULL)
            {
                printf("Error parsing response\n");
                printf("Ensure ip is correct\n");
                authRequestContinue = 0;
            }
            else
            {

                cJSON *innerElement;
                int arraySize = cJSON_GetArraySize(parsedInput);
                if (arraySize != 0)
                {
                    innerElement = cJSON_GetArrayItem(parsedInput, 0);
                }
                cJSON *errorJSONCheck;
                errorJSONCheck = cJSON_GetObjectItem(innerElement, "error");
                if (errorJSONCheck != NULL)
                {
                    cJSON *errorResponseCode = cJSON_GetObjectItem(errorJSONCheck, "type");
                    int errorCode;
                    if (cJSON_IsNumber(errorResponseCode))
                    {
                        if (errorResponseCode->valueint == 101)
                        {
                            printf("\n\n PRESS BUTTON ON HUE TO SYNC \n\n REATTEMPTING CONNECTION IN 5 SECONDS!\n\n");
                            sleep(5);
                        }
                        else
                        {
                            printf("\n\n ERROR CODE %d OCCURED! \n\n CANCELLING AUTH ATTEMPT!\n\n", errorResponseCode->valueint);
                            authRequestContinue = 0;
                        }
                    }
                }
                else
                {
                    //No error
                    cJSON *successResponse = cJSON_GetObjectItem(innerElement, "success");
                    if (successResponse != NULL)
                    {
                        cJSON *success_authCode = cJSON_GetObjectItem(successResponse, "username");
                        if (success_authCode != NULL)
                        {
                            strcpy(authToken, success_authCode->valuestring);
                            authRequestContinue = 0;
                        }
                    }
                }
                cJSON_Delete(parsedInput);
                free(requestHandler_response);
            }
        }
    }
    printf("Finished obtaining auth token\n");
    return 0;
}

int initialConfig(struct arguments* args)
{
    //Initial configuration
    //Config file location
    printf("\n ========== \nBeginning inital configuration\n");
    char *configFileLocation = getenv("HOME");

    char configFolderLocation[1000];
    strcpy(configFolderLocation,configFileLocation);
    strcat(configFileLocation, configPath);
    strcat(configFolderLocation,configFolderPath);
    printf("%s\n%s\n",configFileLocation,configFolderLocation);
    printf("Checking for existing configuration file :");
    if (!access(configFileLocation, R_OK))
    {
        //If the file currently exists
        printf(" EXISTS\nDelete? (y/n):");
        fflush(stdout);
        char tmp[3];
        fgets(tmp, 3, stdin);
        if (!strcmp(tmp, "y\n"))
        {
            //So we got a yes
            printf("\n");
        }
        else
        {
            //We stop
            printf("Stopping initial configuration!\n");
            return -1;
        }
        printf("Deleting old config file :");

        int deleteSuccessful = remove(configFileLocation);
        if (deleteSuccessful == 0)
        {
            printf("Successful! \n");
        }
        else
        {
            perror("Error: ");
            printf("\n");
        }
    }
    else{
        printf(" Not found!\n");
    }
    printf("Creating new config file\n");
    //Creating a new config file
    DIR *configDir = opendir(configFolderLocation);
    //Checking if we already have a folder
    if(!configDir && ENOENT == errno){
        //Folder doesnt exist so we need to create it
        int folderCreatedCheck = mkdir(configFolderLocation,0700);
        if(folderCreatedCheck){
            printf("Error creating folder for storage!\n");
            if(args->debugMode){
                printf("Error code was %d\n",errno);
                printf("Attempted path was %s\n",configFolderLocation);
            }
            return -2;
        }

    }
    else if (!configDir){
        printf("Error creating folder for storage!\n");
        return -2;
    }
    printf("\n IP address of hue:");
    fflush(stdout);
    fflush(stdin);
    fgets(ipAddress, 50, stdin);
    printf("\n");
    strtok(ipAddress, "\n");
    obtainAuthToken(ipAddress, authToken,args);
    //So authtoken is saved
    FILE *newConfigFile = fopen(configFileLocation, "w");
    fprintf(newConfigFile, "ip=%s\nauthToken=%s\n", ipAddress, authToken);
    fclose(newConfigFile);
    return 0;
}
