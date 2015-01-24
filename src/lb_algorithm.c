#include "queue.h"

u_int32_t get_mark(int dstport)
{
	int service = -1;
	int res;
	int index;
	int i;

	init_get_mark();
	if ((service  = get_service(dstport)) == -1)
		res = -1;
	else
	{
		index = get_service_last_srv(service);
		printf("service: %d\n",service);
		printf("service prev index: %d\n",service_list[service].last_srv);
		printf("service min srv: %d\n",service_list[service].min_srv);
		printf("service max srv: %d\n",service_list[service].max_srv);

		if(index > get_service_max_srvidx( service)) // dont go far away from limits
			//index = 0;
			index = get_service_min_srvidx(service);

		int all_down = 0; // if 1 all servers are down

		i = index+1; //next server is selected

		while (!all_down)
		{
			if (i> get_service_max_srvidx(service)) //same as before
				//i = 0; //reset index loop
				i = get_service_min_srvidx(service);


			if (is_srv_up(i))
			{
				
				 set_service_last_srv(service,i);//+1 because marks start in 1 and array index in 0
				break; 
			}
			i++;
	
			if (i==index) //all servers scanned ,but there's not any active
			{
				//TODO: revisar esto. Que pasa si es la primera iteracion??
				LogEntry(0,"ERROR:all servers are down\n");
				all_down = 1;
			}
		}

		if (all_down)
			res = -1;
		else
			res = get_srv_dmark(i);

	}
		
	end_get_mark();
	return res;
}
