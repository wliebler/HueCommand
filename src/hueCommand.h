#ifndef HUECOMMAND
#define HUECOMMAND
#define configPath "/.config/hueCommand/config"
int loadConfigInformation(char* ip, char* aToken);
int obtainAuthToken(char * ip, char* tokenStorage);
int initialConfig();
#endif