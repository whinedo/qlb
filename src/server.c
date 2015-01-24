#include "server.h"
#include "stdlib.h"

void initialize_servers(void)
{
	srv_list = (struct srv_args *)malloc(MAXSERVERS*sizeof(struct srv_args)); //TODO: free this
	nSRVS = 0;
}

int get_servers(void)
{
	return nSRVS;
}

void increment_servers(void)
{
	nSRVS++;
}

struct srv_args * get_server(int server)
{
	if (server < 0 || server > nSRVS)
		return NULL;

	return &srv_list[server];
}

int get_srv_status(int server)
{
	if (server < 0 || server > nSRVS)
		return -1;

	return srv_list[server].status;
}



int get_srv_port(int server)
{
	if (server < 0 || server > nSRVS)
		return -1;

	return srv_list[server].port;
}

int get_srv_dmark(int server)
{
	if (server < 0 || server > nSRVS)
		return -1;

	return srv_list[server].dmark;
}

int get_srv_weight(int server)
{
	if (server < 0 || server > nSRVS)
		return -1;

	return srv_list[server].weight;
}


int is_srv_up(int server)
{
	if (get_srv_status(server) == 1)
		return 1;
	return 0;
}

void set_server(int index,struct srv_args server)
{
	if (index >= 0 && index < nSRVS)
	{
		srv_list[index] = server;	
	}
}
