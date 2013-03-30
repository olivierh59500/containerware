#ifndef P_INFO_H_
# define P_INFO_H_                     1

# include <stdio.h>
# include <stdlib.h>
# include <stdarg.h>
# include <time.h>
# include <inttypes.h>
# include <sys/types.h>
# include <sys/time.h>
# include <sys/select.h>
# include <unistd.h>
# include <errno.h>
# include <string.h>
# include <pthread.h>
# include <ctype.h>

# include "liburi.h"

# define CONTAINER_PLUGIN_INTERNAL     1
# define CONTAINER_NAME                "info"

# include "containerware.h"

struct container_struct
{
	CONTAINER_COMMON;
	pthread_mutex_t lock;
	unsigned long refcount;
	CONTAINERWARE *cw;
};

struct container_instance_struct
{
	CONTAINER_INSTANCE_COMMON;
	CONTAINER_INSTANCE_INFO info;
	CONTAINER *container;
	CONTAINER_INSTANCE_HOST *host;
	pthread_mutex_t lock;
	unsigned long refcount;
};

int containerware_module_init(CONTAINERWARE *cw);
CONTAINER_INSTANCE *instance_create_(CONTAINER *container, CONTAINER_INSTANCE_HOST *host, const CONTAINER_INSTANCE_INFO *info);

#endif /*!P_INFO_H_*/
