#ifndef REQUESTHANDLER
#define REQUESTHANDLER
extern char * requestHandler_response;
extern long long requestHandler_finalSize;
int handleRequest(char *, size_t,size_t,void *);
//Call after every request to ensure it is stored into requestHandler_response
int finishRequest();
#endif
