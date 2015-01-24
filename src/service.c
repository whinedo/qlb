#include "service.h"
#include <stdlib.h>


void initialize_services()
{
	nPORTS = 0; 
}

void increment_services()
{
	nPORTS++;
}

int get_services(void)
{
	return nPORTS;
}

int get_service_index(int dstport)
{
	int i;

	for (i=0;i<nPORTS;i++)
	{
		if (service_list[i].lb_port == dstport)
		{
			return i;
		}
			
	}
	return -1;
}

struct service_args * get_service(int index)
{
	if (index < 0 || index > nPORTS)
		return NULL;
	return &service_list[index];
}

int get_service_lbport(int service)
{
	if (service < 0 || service > nPORTS)
		return -1;

	return service_list[service].lb_port;
}

int get_service_last_srv(int service)
{
	if (service < 0 || service > nPORTS)
		return -1;

	return service_list[service].last_srv;
}

int get_service_last_mark(int service)
{
	if (service < 0 || service > nPORTS)
		return -1;

	return service_list[service].last_srv+1; // index in array starts in 0 but marks in 1
}

int get_service_min_srvidx(int service)
{

	if (service < 0 || service > nPORTS)
		return -1;

	return service_list[service].min_srv;
}

int get_service_max_srvidx(int service)
{
	if (service < 0 || service > nPORTS)
		return -1;

	return service_list[service].max_srv;
}

int set_service_last_srv(int service,int mark)
{
	if (service < 0 || service > nPORTS)
		return -1;

	service_list[service].last_srv = mark;
	return 1;
}

void set_service(int index, struct service_args service)
{
	if (index >= 0 || index < nPORTS)
	{
		service_list[index] = service;
	}
}
