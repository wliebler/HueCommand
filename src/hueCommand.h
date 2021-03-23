#ifndef HUECOMMAND
#define HUECOMMAND
#define configPath "/.config/hueCommand/config"
#define configFolderPath "/.config/hueCommand"
int loadConfigInformation(char* ip, char* aToken);
int initialConfig();
struct arguments{
    int debugMode;
    int printHelp;
    int target;
    int optionB;
    //-1 is not provided
    //0 is off
    //1 is on
    int argOnOff;
};

int obtainAuthToken(char * ip, char* tokenStorage,struct arguments* args);
#endif
