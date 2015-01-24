#ifndef _SERVICE
#define MAXSERVICES 20
#include <sys/types.h>

struct service_args
{
	int min_srv;
	int max_srv;
	u_int32_t last_srv ;
	int lb_port;
};

struct service_args service_list[MAXSERVICES];

int nPORTS;

//Services Functions
void initialize_services(void);
void increment_services(void);
// get functions
int get_services(void);
int get_service_index(int dstport);
struct service_args * get_service(int dstport);
int get_service_lbport(int service);
int get_service_last_srv(int service);
int get_service_min_srvidx(int service);
int get_service_max_srvidx(int service);
int get_service_last_mark(int service);
//set functions
int set_service_last_srv(int service,int mark);
int set_srv_up(int server);
int set_srv_down(int server);
void set_service(int index, struct service_args service);
#endif
