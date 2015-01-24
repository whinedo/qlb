#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>            /* for NF_ACCEPT */
#include <errno.h>

#include <libnetfilter_queue/libnetfilter_queue.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>


#define  __USE_GNU //hash table
#include <search.h>

#include "queue.h"
#include "logger.h"
#include "server.h"
#include "service.h"


//#include "lb_algorithm" //TODO: check this shit => cicle with source files


//#define HBTIME 3

u_int32_t last_srv = 0;
sem_t scdt_lock; // to protect server list

int hbtime = 10;

char iface [5] = "";

typedef struct _ENTRY
{
	unsigned int used;
	ENTRY entry;
}_ENTRY;

struct data
{
	char key [21];
	u_int32_t mark;
};




struct nfqueueDS
{
	int fd;
	struct nfq_handle *h;
        struct nfq_q_handle *qh;
        struct nfnl_handle *nh;
};


struct pkt_info
{
	u_int32_t id;
	int dstport;
};
struct nfqueueDS data_sts; //netfilter queue data structures

struct next_mark next_mark;

void init_get_mark(void)
{
	sem_wait(&scdt_lock);
}
void end_get_mark(void)
{
	sem_post(&scdt_lock);
}

int LogEntry(int headType,char * msg,...)
{
	//TODO esta funcion es la que debe implementarse dependiendo de los mensajes

	char logmsg[MAXMSG];
	char buff[MAXMSG-MAXHEADER];
	time_t result;
	struct tm * now;


	result = time(NULL);
	now = localtime(&result);

	if (headType == 0) //log message generated in main function
	{
		sprintf(logmsg,"%s: %02d:%02d:%02d =>","[MAIN]",now->tm_hour,now->tm_min,now->tm_sec);
	}
	else if (headType == 1) //log message generated in main function
	{
		sprintf(logmsg,"%s: %02d:%02d:%02d =>","[HEARTBEAT]",now->tm_hour,now->tm_min,now->tm_sec);
	}

	va_list varglist;
	va_start(varglist,msg);
	vsnprintf(buff,sizeof(buff),msg,varglist);
	va_end(varglist);

	strcat(logmsg,buff);


	return writeLog(logmsg);
}



int parse_servers()
{
	/*
	 * this function parses the file and initializes the structure
	 */
	FILE * fd;
	int i = -1;
	int j = 0;

	int lb_port;
	int old_lb_port = -1;
	int offset = 0x1000000;
	int mark = offset;
	struct srv_args * server;
	int port,weight;
	char ip [IPMAXLEN];

	memset(ip,'\0',IPMAXLEN);

	fd = fopen("/etc/qlb.conf","r"); //open config file where are set scdt ips
	if (fd == NULL)
	{
		LogEntry(0,strerror(errno));
		return -1;
	}

	while(fscanf(fd,"%d %s %d %d\n",&lb_port,ip,&port,&weight) != EOF)
	{
		server = get_server(j);
		strcpy(server->ip,ip);
		server->port = port;
		server->weight = weight;

		//TODO check if null

		//printf("%s %d\n",srv_list[i].ip,srv_list[i].port);
		increment_servers();
		// DEBUG
		printf("---------\n");
		printf("servers:%d\n",get_servers());
		printf("lb_port:%d\n",lb_port);
		printf("lb_ol_port:%d\n",old_lb_port);
		// FIN DEBUG

		if (lb_port != old_lb_port) //new service
		{
			increment_services();
		
			// DEBUG
			printf("service:%d\n",get_services());
			// FIN DEBUG

			old_lb_port = lb_port;
			i++; //update services index

			struct service_args * service = get_service(i);

			service->lb_port = lb_port;
			service->min_srv = j;
			service->last_srv = j+1;

			//DEBUG
			printf("lb_port:%d\n",service->lb_port);
			printf("min_srv:%d\n",service->min_srv);
			printf("last_srv:%d\n",service->last_srv);
			//FINDEBUG

			if (i > 0)
			{
				struct service_args * former_service = get_service(i-1);
				former_service->max_srv = j-1;
				set_service(i-1,*former_service);
			}

			set_service(i,*service);

			//DEBUG
			struct service_args * service_dbg = get_service(i);
			if(service_dbg == NULL)
			{
				printf("null\n");
			}
			printf("dbg_lb_port:%d\n",service_dbg->lb_port);
			printf("dbg_min_srv:%d\n",service_dbg->min_srv);
			printf("dbg_last_srv:%d\n",service_dbg->last_srv);
			//FINDEBUG

		}
		server->status = 0; //0 down, >0 active
		server->mark = mark; //mark must be > 0
		server->dmark = j+1;

		set_server(j,*server);

		//DEBUG
		struct srv_args * server_dbg = get_server(j);
		printf("status:%d\n",server_dbg->status);
		printf("mark:%o\n",server_dbg->mark);
		printf("dmark:%d\n",server_dbg->dmark);
		//FIN DEBUG

		mark += offset;
		j++;

	}
	fclose(fd);
	
/*
	if (i > 0 && j > 0) //to set last scdt values in service structure
		service_list[i-1].max_srv = j-1; //TODO CHECK THIS
*/

	struct service_args * service_ms = get_service(i);
	printf("i:%d\n",i);
	service_ms->max_srv = j-1; //TODO CHECK THIS
	set_service(i,*service_ms);
	return j; //return srv_list index which is nSRVS

}

int iptables_rules()
{
	int i;
//	int queue,j = 0;
//	int lb_port;
//	int service_index = 0;

	char nat_rule[1024];
	char mangle_rule[2048];
	char mark_discard[512];
	char mark_aux[100];
	char port_aux[100];
	char dports[512];
	struct srv_args * server;
	struct service_args * service;

	FILE * rule_file;

	if((rule_file = fopen("rules","w")) == NULL)
		return -1;

	

	system("iptables -t nat -F"); //let's clean up
	system("iptables -t mangle -F");
	system("iptables -t filter -F");

	memset(mark_discard,'\0',512);
	memset(dports,'\0',512);


	//nat rules 
	for (i = 0; i < nSRVS; i++)
	{

		server = get_server(i);
		memset(nat_rule,'\0',1024);

		sprintf(nat_rule,"iptables -t nat -A PREROUTING -p tcp -m mark --mark 0x%07x -j DNAT --to-destination %s:%d\n",server->mark,server->ip,server->port);
		memset(mark_aux,'\0',100);
		sprintf(mark_aux,"-m mark ! --mark 0x%07x ",server->mark);
		strcat(mark_discard,mark_aux);
		fputs(nat_rule,rule_file);
		system(nat_rule);
	}

	//mangle rules 
	for (i = 0; i < nPORTS; i++)
	{
		service = get_service(i);
		memset(port_aux,'\0',100);

		if(i > 0)
			sprintf(port_aux,",%d",service->lb_port);
		else
			sprintf(port_aux,"%d",service->lb_port);

		strcat(dports,port_aux);
		
	}

	sprintf(mangle_rule,"iptables -t mangle -A PREROUTING -p tcp -m multiport --dports %s -m conntrack --ctstate NEW  %s -j NFQUEUE --queue-num 1\n",dports,mark_discard);
	system(mangle_rule);
	fputs(mangle_rule,rule_file);


	if (!strcmp(iface,""))
	{
		return -1; //iface not set
	}

	memset(nat_rule,'\0',1024); //last rule: MASQUERADE
	sprintf(nat_rule,"iptables -t nat -A POSTROUTING -o %s -j MASQUERADE \n",iface);
	system(nat_rule);
	fputs(nat_rule,rule_file);
	

	fclose(rule_file);
	return 0;
}


void term(int signo)
{
	LogEntry(0,"stopping load balancer\n");
	nfq_destroy_queue(data_sts.qh);

	nfq_close(data_sts.h);

	sig_term(0);

        exit(0);

}

void hearbeat()
{
	int i;
	struct sockaddr_in sin;
	int s;
	int old_status;

	while(1)
	{
		sem_wait(&scdt_lock);
	
		for (i=0;i<nSRVS;i++) /* pinging servers*/
		{		
			
			struct srv_args * server = get_server(i);

			s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
			sin.sin_family = AF_INET;
	
			sin.sin_port = htons(server->port);
			sin.sin_addr.s_addr = inet_addr(server->ip);
			old_status = server->status;

			server->status = connect(s,(struct sockaddr *)&sin, sizeof(sin)) + 1; // if error status = 0 and continue

			if (srv_list[i].status == 1)
				shutdown(s,SHUT_RDWR);
			
			else
				LogEntry(1," SCDT[%d] ip:%s port:%d is down\n",i,server->ip);

			if (old_status == 0 && srv_list[i].status == 1)
				LogEntry(1," SCDT[%d] ip:%s port:%d recovered\n",i,server->ip,server->port);

			set_server(i,*server);

			close(s);
		}
		sem_post(&scdt_lock);
		sleep(hbtime);
	}
}


u_int32_t get_mark(int dstport)
{
	int service_idx = -1;
	int res;
	int index;
	int i;

	init_get_mark();

	if ((service_idx  = get_service_index(dstport)) == -1)
		res = -1;
	else
	{
		index = get_service_last_srv(service_idx);
		
		if(index > get_service_max_srvidx(service_idx)) // don't go beyond limits
			index = get_service_min_srvidx(service_idx);

		int all_down = 0; // if 1 all servers are down

		i = index+1; //next server is selected

		while (all_down != 1)
		{
			if (i > get_service_max_srvidx(service_idx)) //same as before
				i = get_service_min_srvidx(service_idx);


			if (is_srv_up(i))
			{
				
				set_service_last_srv(service_idx,i);//+1 because marks start in 1 and array index in 0
				break; 
			}

	
			if (i==index) //all servers scanned ,but there's not any active
			{
				LogEntry(0,"ERROR:all servers are down\n");
				all_down = 1;
			}

			i++;
		}

		if (all_down)
			res = -1;
		else
			res = get_srv_dmark(i);

	}

	end_get_mark();

	return res;
}

int get_dstport(char *d)
{
	int hsize = (d[0]&0x0F)*4;
	return (d[hsize+2]&0xFF)*0x100+(d[hsize+3]&0xFF);
}

static struct pkt_info get_pkt_info (struct nfq_data * tb)
{
        struct nfqnl_msg_packet_hdr *ph;
        char *data;
	struct pkt_info info;
        data = NULL; 
        ph = nfq_get_msg_packet_hdr(tb);

	int len = nfq_get_payload(tb, &data);
	
        if (ph){
                info.id = ntohl(ph->packet_id);
               // printf("hw_protocol=0x%04x hook=%u id=%u ",
               //         ntohs(ph->hw_protocol), ph->hook, id);
        }
	info.dstport = get_dstport(data);

        return info;
}
        

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data)
{
	int res;
        struct pkt_info  info = get_pkt_info(nfa);

        u_int32_t mark = nfq_get_nfmark(nfa);


	// printf("entering callback\n");

        if (!mark)
	{
	
		int nmark = get_mark(info.dstport);
		if (nmark > 0)
		{
        		res = nfq_set_verdict_mark(qh,info.id,NF_REPEAT,nmark,0,NULL);
		}
		else
		{
			LogEntry(0,"ERROR: can not get a mark for packet. Are all servers down?\n");
			res = -1;
        		res = nfq_set_verdict_mark(qh,info.id,NF_DROP,nmark,0,NULL);
		}
	}
	else
	{
		//this should never be executed. If so, then there is an error in iptables rules
		printf("MARK:%u\n",mark);
		//exit(-1);
	}	
	//DEBUG
//        printf("exiting callback\n");
	//FINDEBUG
	return res;
}

struct nfqueueDS setQueue(int queue_num)
{
	struct nfq_handle *h;
        struct nfq_q_handle *qh;
        struct nfnl_handle *nh = NULL;
        int fd;

	struct nfqueueDS data_sts;

      //  LogEntry(0,"opening library handle\n");
        h = nfq_open();
        if (!h)
	{
                LogEntry(0, "error during nfq_open in nfqueue %d\n",queue_num);
                exit(1);
        }

        LogEntry(0,"unbinding existing nf_queue handler for AF_INET (if any) for nfqueue %d\n",queue_num);
        if (nfq_unbind_pf(h, AF_INET) < 0)
	{
                LogEntry(0, "error during nfq_unbind_pf for nfqueue %d\n",queue_num);
        //        exit(1);
        }

        LogEntry(0,"binding nfnetlink_queue as nf_queue handler for AF_INET for nfqueue %d\n",queue_num);
        if (nfq_bind_pf(h, AF_INET) < 0)
	{
                LogEntry(0, "error during nfq_bind_pf for nfqueue %d\n",queue_num);
                exit(1);
        }

        LogEntry(0,"binding this socket to queue %d\n",queue_num);
        qh = nfq_create_queue(h,  queue_num, &cb, NULL);
        if (!qh)
	{
                LogEntry(0, "error during nfq_create_queue for nfqueue %d\n",queue_num);
                exit(1);
        }


        LogEntry(0,"setting copy_packet mode for nfqueue %d\n",queue_num);
        if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
                LogEntry(0, "can't set packet_copy mode nfqueue %d\n",queue_num);
                exit(1);
        }

        fd = nfq_fd(h);
	data_sts.fd = fd;
	data_sts.h = h;
	data_sts.qh = qh;
	data_sts.nh = nh;

	return data_sts;

}





int main(int argc, char **argv)
{
        int rv;
        char buf[4096] __attribute__ ((aligned));

	struct sigaction action;
	int i,fd_size,res;
	struct timeval timeout;

	
	fd_set queue_fd_set;


#ifdef DAEMON
//	daemon(0,0);
#endif

#ifndef VERSION
	#define VERSION "UNDEFINED"
#endif

#ifndef BUILD
	#define BUILD "UNDEFINED"
#endif
	
	signal(SIGPIPE,SIG_IGN); // it it's kill some actions must be done

	action.sa_flags = 0;
	
	//action.sa_handler = sig_term;
	action.sa_handler = term;

	sigaction(SIGTERM,&action,NULL);
	sigaction(SIGINT,&action,NULL);
	

	pthread_t hb_thread; //servers heartbeat 
	pthread_t logger_thread; //logger


	initialize_servers();
	initialize_services();

//	service_list = (struct service_args *)malloc(13*sizeof(struct service_args)); //TODO: free this

	if ((logger_thread = create_logger("/var/log/qlb"))== -1)
	{
		perror("cannot create logger");
		exit(-1);
	}

	LogEntry(0,"Starting queue manager version:%s build:%s\n",VERSION,BUILD);

	if (argc < 3)
	{
		LogEntry(0,"Not enought init parameters\n");
		exit(-1);
	}

	hbtime = atoi(argv[1]);

	if (hbtime > 3600) //more than an hour???
	{
		LogEntry(1,"Not a valid heartbeat time. Too long\n");
	}


	strcpy(iface,argv[2]);

	LogEntry(0,"iface:%s heartbeat time:%d\n",iface,hbtime);


	//printf("Starting queue manager\n");

	sem_init(&scdt_lock,0,1); //binary

	nSRVS = parse_servers(); //get ip and port for each scdt

	printf("nSRVS:%d\n",nSRVS);
	printf("HOLA\n");
	if (nSRVS == -1)
	{
		LogEntry(0,"Cannot parse servers file\n");
		exit(-1);
	}

	LogEntry(0,"parsed servers file\n");

	if (iptables_rules() == -1)
	{

		LogEntry(0,"Cannot set iptables rules\n");
		exit(-1);
	}

	LogEntry(0,"iptables rules actived\n");

	//create hb for servers
	if (pthread_create(&hb_thread,NULL,(void *)hearbeat,NULL)< 0)
	{
		LogEntry(1,"cannot create hearbeat thread\n");
		exit(-1);
	}

	LogEntry(1,"Heartbeat thread created\n");

	FD_ZERO(&queue_fd_set);

	
	data_sts = setQueue(1);

	if (data_sts.fd < 0)
	{
		LogEntry(0,"can't retrieve queue file descritor\n");
        	exit(1);
	}

	FD_SET(data_sts.fd,&queue_fd_set);


	fd_size = data_sts.fd + 1;

	
	//DEBUG
	LogEntry(0,"nPORTS:%d\n",nPORTS);
	LogEntry(0,"fd_size%d\n",fd_size);
	//FINDEBUG
	
	while(1)
	{

		timeout.tv_sec = 10; //timeout for select
		timeout.tv_usec = 0;


		res = select(fd_size, &queue_fd_set, 0,0, 0);

		if (res < 0) //look for the queue to be used
		{
			LogEntry(0,"Error while getting packet from queue\n");
		}
		else if (res == 0)
			LogEntry(0,"timeout select\n");
		else
		{
	
			for (i=0;i<fd_size; i++)
			{
				rv = -1;
				if (FD_ISSET(data_sts.fd, &queue_fd_set))
				{
					memset(buf,'\0',strlen(buf));
					rv = recv(data_sts.fd, buf, sizeof(buf), 0);
					if (rv > 0)
					{
	        				nfq_handle_packet(data_sts.h, buf, rv);
					}
				}

				FD_SET(data_sts.fd,&queue_fd_set);

			}
		}
	}
}

