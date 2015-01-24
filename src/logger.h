#include <pthread.h>

#define MAXBUF 2048
#define MAXMSG 2048
#define MNMSGLEN 5000
#define MAXHEADER 100

int writeLog(char *);
pthread_t create_logger(char*);

void sig_term(int);

void waitElem();
void newElem();
