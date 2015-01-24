#include "logger.h"

#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>



pthread_mutex_t lastLock;
pthread_t logThread;

sem_t newelemLock;


int end = 0;
int last=0; //index for producer
int toget=0;//index for consumer
int nelem=0;//number of elements in the buffer
char log_buffer [MAXBUF][MAXMSG];
int LOG = 1;
FILE * logFd = NULL; //file descriptor for logging file

void waitElem()
{
	/*
	 * wait newelenLock  untill any data in the log buffer
	 */
	sem_wait(&newelemLock);
}

void newElem()
{
	/*
	 * post newelenLock. There is data in the buffer
	 */
	sem_post(&newelemLock);
}

void sig_term(int signo)
{
	end = 1;
	newElem();	
	pthread_join(logThread,NULL);
	exit(0);
}

int writeLog(char * msg)
{
	if (LOG == -1)
		return -1;

	//producer
	if (end) //if end no new message allow
		return 0;
	pthread_mutex_lock(&lastLock); //lock of the buffer 
	if ((nelem+1) == MAXBUF)
	{
		pthread_mutex_unlock(&lastLock);
		return 0;
	}
	if (strcpy(log_buffer[last],msg) == NULL)
	{
#ifdef DEBUG
	//printf("error en strcpy al insertar:%s\n",msg);
#endif
		pthread_mutex_unlock(&lastLock);
		return -1;
	}

	nelem ++;
	last ++;

#ifdef DEBUG
	//printf("insertados:%d\n",nelem);
#endif
	if (last == MAXBUF)
		last = 0;	
	pthread_mutex_unlock(&lastLock);
	//sem_post(&newelemLock); //wake up the consumer
	newElem();
	return 1;
}




void getLogName(char *logname)
{
	time_t result;
	//struct tm * now = (struct tm *)malloc(sizeof(struct tm));
	struct tm * now;
	result = time(NULL);
	now = localtime(&result);
	sprintf(logname,"%d%d%d",now->tm_mday,now->tm_mon+1,now->tm_year+1900); //FIXME: check month + 1, year + 1900
	//free(now);
}

int openLogFile(char * logpath)
{
	if ((logFd = fopen(logpath,"a")) == NULL)
	{
#ifdef DEBUG
		perror("fopen");
		//printf("Consumer cannot open log file\n");
#endif
		return 0;
	}
	return 1;
}

void consumer(void * path)
{
	int go_on = 1;

	char  logname [9];//new logfile
	char  lognameAux[9]; 

	char  logpath [30]; //absolute path to log file
#ifdef DEBUG
	//printf("Starting consumer\n");
	fflush(stdout);


	//printf("Using %s as logging file\n",logpath);
#endif
	printf("path:%s\n",(char *)path);

	while(go_on == 1)
	{
		getLogName(lognameAux);

		if (strcmp(lognameAux,logname)) //if differnt lognames, it's a new day
		{
#ifdef DEBUG
			//printf("Changing logname:%s %s\n",logname,lognameAux);
#endif
			sprintf(logname,"%s",lognameAux);
			sprintf(logpath,"%s/%s.log",(char *)path,logname);

//DEBUG
		printf("logpath:%s\n",logpath);
//FINDEBUG

			if (logFd != NULL)
				fclose(logFd);

			if (!openLogFile(logpath))
				return;

		}

		waitElem();
		pthread_mutex_lock(&lastLock);
		//if (write(logFd,log_buffer[toget],strlen(log_buffer[toget])) < 0)
		if (nelem > 0) // write new log message
		{
			
			if (fputs(log_buffer[toget],logFd)==EOF)
			{
	
	#ifdef DEBUG
				perror("fputs");
				fputs("Cannot retrieve msg from log buffer",logFd); //will this be written in the log file?
	#endif
				LOG = -1;
				pthread_mutex_unlock(&lastLock);
				fclose(logFd);
				return;
			}
			fflush(logFd);
			nelem --;
			toget ++; //free spaces
			if (toget == MAXBUF)
				toget = 0;
		}
		if (end && nelem == 0) //sent sigterm
		{
			go_on = 0;
		}
		pthread_mutex_unlock(&lastLock);
	}

	fclose(logFd);
	return;
	
}

pthread_t create_logger(char * logpath)
{

	sem_init(&newelemLock,0,0); // lock any element in log buffer		

	if (pthread_create(&logThread,NULL,(void *)consumer,(void *)logpath)< 0)
	{
		LOG = -1;
		return -1;
	}
	return logThread;
	
}
