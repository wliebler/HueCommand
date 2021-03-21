#include "cJSON/cJSON.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include "hueCommand.h"
#include "requestHandler.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#define responseBufferSize 10
#define maxLightName 20
char ipAddress[50];
char authToken[256];
//Whether to print debug information
int arg_debugMode = 0;
//Whether to print help information
int arg_printHelpArg = 0;
//The target integer for any commands
char arg_targetint[4];
//Index of the char* for the json input if given
int arg_jSONIndex = -1;
int parseArgs(int argc, char *argv[])
{
    //Parsing extra arguments
    for (int i = 1; i < argc;)
    {
        //Incrimenting the loop will be done in here
        if (!strcmp(argv[i], "-v"))
        {
            //Verbose, prints debug information
            arg_debugMode = 1;
            i++;
        }
        else if (!strcmp(argv[i], "-h"))
        {
            arg_printHelpArg = 1;
            i++;
        }
        else if (!strcmp(argv[i], "-t"))
        {
            //Making sure there is a next arg
            if (argc > i + 1)
            {
                if (strlen(argv[i + 1]) < 4)
                {
                    strcpy(arg_targetint, argv[i + 1]);
                    i += 2;
                }
                i += 2;
            }
            else
            {
                printf("\nNot enough arguments for -t argument!\n");
                i++;
            }
        }
        else if (!strcmp(argv[i], "-j"))
        {
            if (argc > i + 1)
            {
                arg_jSONIndex = i + 1;
                i += 2;
            }
            else
            {
                printf("\nNot enough arguments for -j argument!\n");
                i += 2;
            }
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

int setLightState(int argc, char *argv[])
{
    cJSON *body;
    //First grab arguments
    int arg_onoff = -1;     //0 is off, 1 is on
    int brightnessSet = -1; //-1 is not provided
    for (int i = 1; i < argc;)
    {
        if (!strcmp(argv[i], "on"))
        {
            arg_onoff = 1;
            i++;
        }
        else if (!strcmp(argv[i], "off"))
        {
            arg_onoff = 0;
            i++;
        }
        else if (!strcmp(argv[i], "-b"))
        {
            i++;
            if (i < argc)
            {
                brightnessSet = atoi(argv[i]);
                i++;
            }
            else
            {
                printf("\n Not enough arguments to set brightness");
            }
        }
        else
        {
            //No args
            i++;
        }
    }
    //Now creating our json object
    //First we check if we have been given a json object via the -j argument
    if (arg_jSONIndex != -1)
    {
        //JSON argument given, need to make sure it is valid
        body = cJSON_Parse(argv[arg_jSONIndex]);
        if (body == NULL)
        {
            printf("\nError parsing passed jSON! Ignoring it");
            body = cJSON_CreateObject();
        }
    }
    else
    {
        //No json argument given
        body = cJSON_CreateObject();
    }
    //If setting the light on or off
    cJSON *newLightState;
    switch (arg_onoff)
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
    if (brightnessSet >= 0)
    {
        cJSON_AddItemToObject(body, "bri", cJSON_CreateNumber(brightnessSet));
    }
    char address[50];
    strcpy(address, "lights/");
    strcat(address, arg_targetint);
    strcat(address, "/state");
    messageSender("PUT", "Content-Type: application/json", cJSON_Print(body), address);
    if (arg_debugMode)
    {
        printf("\nResponse from sending request to set state");
        printf("\n%s\n", requestHandler_response);
    }
    return 0;
}
//All args used for the light go here
//Used for priting out all the lights in the standard version
//Style is what syle we will print the details out in
int printListLightInfo(char *lightNum, cJSON *light, int style)
{
    //Limit to displaying the first 30 charcaters
    char lightName[maxLightName + 1];
    //First grabing the name of the light
    cJSON *jsonLightName = cJSON_GetObjectItem(light, "name");
    if (jsonLightName == NULL)
    {
        if (arg_debugMode)
        {
            printf("DEBUG: Light is missing name!\n");
        }
    }
    else
    {
        if (arg_debugMode)
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
            if (arg_debugMode)
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
        if (arg_debugMode)
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
int lights_list(int argc, char *argv[], int style)
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
        if (arg_debugMode)
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
        printListLightInfo(currentNode->string, currentNode, style);
        while (currentNode->next != NULL)
        {
            currentNode = currentNode->next;
            printListLightInfo(currentNode->string, currentNode, style);
        }
    }
    else
    {
        printf("\nNo lights appear to be connected to this hue\n!");
    }
    return 0;
}
int lights_Commands(int argc, char *argv[])
{
    if (argc <= 2 || arg_printHelpArg)
    {
        lights_printCommands();
    }
    else
    {
        if (!strcmp(argv[2], "l"))
        {
            lights_list(argc, argv, 0);
        }
        else if (!strcmp(argv[2], "s"))
        {
            setLightState(argc, argv);
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
    parseArgs(argc, argv);
    curl_global_init(CURL_GLOBAL_ALL);
    if (argc >= 2 && (!strcmp(argv[1], "init-config")))
    {
        if (arg_printHelpArg == 1)
        {
            printf("\n| Obtains an auth-token from the Hue Bridge to allow for requests to be authorised\n| Required for commands to function correctly\n");
            return 0;
        }
        else
        {
            initialConfig();
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
                lights_Commands(argc, argv);
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

int obtainAuthToken(char *ip, char *tokenStorage)
{
    printf("Obtaining new auth token\n");
    //Constructing JSON format
    cJSON *body = cJSON_CreateObject();
    cJSON *bodyValue = cJSON_CreateString("hueCommand#user");
    cJSON_AddItemToObject(body, "devicetype", bodyValue);
    if (arg_debugMode)
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

            if (arg_debugMode)
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
            if (arg_debugMode)
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

int initialConfig()
{
    //Initial configuration
    //Config file location
    printf("\n ========== \nBeginning inital configuration\n");
    char *configFileLocation = getenv("HOME");
    strcat(configFileLocation, configPath);
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
    printf("Creating new config file\n");
    //Creating a new config file

    printf("\n IP address of hue:");
    fflush(stdout);
    fflush(stdin);
    fgets(ipAddress, 50, stdin);
    printf("\n");
    strtok(ipAddress, "\n");
    obtainAuthToken(ipAddress, authToken);
    //So authtoken is saved
    FILE *newConfigFile = fopen(configFileLocation, "w");
    fprintf(newConfigFile, "ip=%s\nauthToken=%s\n", ipAddress, authToken);
    fclose(newConfigFile);
    return 0;
}
