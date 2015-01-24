#include <sys/types.h>

typedef void * data_structure;

struct next_mark
{
	data_structure ds;

	data_structure (*init)(void);
	u_int32_t (*next_mark)();
};
	
