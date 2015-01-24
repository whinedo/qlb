#ifndef _SERVER
#define MAXSERVERS 50
#define IPMAXLEN 16

struct srv_args
{
	char ip[IPMAXLEN];
	int port;
	int mark; //mark in hex
	int dmark; //mark in decimal representation
	int status;
	int weight; //for scheduling algorithm
};

struct srv_args * srv_list;

int nSRVS; 

//Servers Functions

void initialize_servers(void);
void increment_servers(void);

int is_srv_up(int server);
//get functions
//int get_srv_status(int server);
struct srv_args* get_server(int server);
int get_servers();
int get_srv_port(int server);
int get_srv_dmark(int server);
int get_srv_weight(int server);
//set functions
int set_srv_status(int index,int status);
void set_server(int index,struct srv_args server);
#endif
