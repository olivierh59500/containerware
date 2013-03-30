#ifndef P_DUMMY_H_
# define P_DUMMY_H_                    1

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

# define ENDPOINT_PLUGIN_INTERNAL      1
# define ENDPOINT_SCHEME               "dummy"

# include "containerware.h"

struct endpoint_server_struct
{
	ENDPOINT_SERVER_COMMON;
	pthread_mutex_t lock;
	size_t refcount;
};

struct endpoint_struct
{
	ENDPOINT_COMMON;
	ENDPOINT_SERVER *server;
	pthread_mutex_t lock;
	size_t refcount;
	int acquired;
};

struct container_request_struct
{
	CONTAINER_REQUEST_COMMON;
	ENDPOINT *endpoint;
	pthread_mutex_t lock;
	size_t refcount;
	struct timeval reqtime;
};

int containerware_module_init(CONTAINERWARE *cw);
ENDPOINT *endpoint_create_(ENDPOINT_SERVER *me, URI *uri);
CONTAINER_REQUEST *request_create_(ENDPOINT *ep);

#endif /*!P_DUMMY_H_*/