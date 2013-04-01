#ifndef P_FCGI_H_
# define P_FCGI_H_                     1

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

# include "fcgiapp.h"

# define ENDPOINT_PLUGIN_INTERNAL      1
# define ENDPOINT_SCHEME               "fcgi"
# define DEFAULT_FASTCGI_BACKLOG       15

# include "containerware.h"

struct endpoint_server_struct
{
	ENDPOINT_SERVER_COMMON;
	pthread_mutex_t lock;
	size_t refcount;
	CONTAINERWARE *cw;
};

struct endpoint_struct
{
	ENDPOINT_COMMON;
	ENDPOINT_SERVER *server;
	pthread_mutex_t lock;
	size_t refcount;
	CONTAINERWARE *cw;
	/* FastCGI listening socket */
	int fd;
	/* The next request to be acquired */
	FCGX_Request *request;
	struct timeval reqtime;	
};

struct container_request_struct
{
	CONTAINER_REQUEST_COMMON;
	CONTAINERWARE *cw;
	ENDPOINT *endpoint;
	pthread_mutex_t lock;
	size_t refcount;
	struct timeval reqtime;
	FCGX_Request *request;
	FCGX_Stream *in;
	FCGX_Stream *out;
	FCGX_Stream *err;
	jd_var env;
	jd_var headers;
	int headers_sent;
	struct cw_request_info_struct info;
};

int containerware_module_init(CONTAINERWARE *cw);
ENDPOINT *endpoint_create_(ENDPOINT_SERVER *me, URI *uri);
CONTAINER_REQUEST *request_create_(ENDPOINT *ep);

#endif /*!P_FCGI_H_*/