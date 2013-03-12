#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define ENDPOINT_PLUGIN_INTERNAL       1

#include "p_containerware.h"

/* This is a dummy endpoint: it generates a single request and then
 * does nothing else.
 */

static int dummy_server_endpoint_(ENDPOINT_SERVER *me, ENDPOINT **ep);

/* static unsigned long dummy_addref_(ENDPOINT *me); */
static unsigned long dummy_release_(ENDPOINT *me);
static ENDPOINT_MODE dummy_mode_(ENDPOINT *me);
static int dummy_fd_(ENDPOINT *me);
static int dummy_process_(ENDPOINT *me);
static int dummy_acquire_(ENDPOINT *me, CONTAINER_REQUEST **req);

static unsigned long dummy_request_addref_(CONTAINER_REQUEST *me);
static unsigned long dummy_request_release_(CONTAINER_REQUEST *me);

struct endpoint_server_struct
{
	ENDPOINT_SERVER_COMMON;
	size_t refcount;
};

struct endpoint_struct
{
	ENDPOINT_COMMON;
	size_t refcount;
	int acquired;
};

struct container_request_struct
{
	CONTAINER_REQUEST_COMMON;
	size_t refcount;
};

static struct endpoint_server_api_struct dummy_server_api_ =
{
	NULL,
	NULL,
	NULL,
	dummy_server_endpoint_
};

static struct endpoint_api_struct dummy_api_ =
{
	NULL,
	NULL,
	dummy_release_,
	dummy_mode_,
	dummy_fd_,
	dummy_process_,
	dummy_acquire_
};

static struct container_request_api_struct dummy_request_api_ =
{
	NULL,
	dummy_request_addref_,
	dummy_request_release_
};

ENDPOINT_SERVER *
dummy_endpoint_server(void)
{
	ENDPOINT_SERVER *p;
	
	p = (ENDPOINT_SERVER *) calloc(1, sizeof(ENDPOINT_SERVER));
	if(!p)
	{
		return NULL;
	}
	p->refcount = 1;
	p->api = &dummy_server_api_;
	return p;
}

static int
dummy_server_endpoint_(ENDPOINT_SERVER *me, ENDPOINT **ep)
{
	(void) me;
	
	*ep = (ENDPOINT *) calloc(1, sizeof(ENDPOINT));
	if(!*ep)
	{
		return -1;
	}
	(*ep)->refcount = 1;
	(*ep)->api = &dummy_api_;
	return 0;
}

static unsigned long
dummy_release_(ENDPOINT *me)
{
	free(me);
	return 0;
}

ENDPOINT_MODE
dummy_mode_(ENDPOINT *me)
{
	(void) me;
	
	/* Return EM_THREAD to avoid having to provide a file descriptor
	 * to listen on.
	 */
	return EM_THREAD;
}

int
dummy_fd_(ENDPOINT *me)
{
	(void) me;
	
	return -1;
}

int
dummy_process_(ENDPOINT *me)
{
	if(!me->acquired)
	{
		return 1;
	}
	for(;;)
	{
		sleep(10);
	}
	return 0;
}

int
dummy_acquire_(ENDPOINT *me, CONTAINER_REQUEST **req)
{
	*req = NULL;
	if(me->acquired)
	{
		errno = EWOULDBLOCK;
		return -1;
	}
	me->acquired = 1;
	*req = (CONTAINER_REQUEST *) calloc(1, sizeof(CONTAINER_REQUEST));
	if(!*req)
	{
		return -1;
	}
	(*req)->refcount = 1;
	(*req)->api = &dummy_request_api_;
	return 0;
}

static unsigned long
dummy_request_addref_(CONTAINER_REQUEST *me)
{
	me->refcount++;
	return me->refcount;
}

static unsigned long
dummy_request_release_(CONTAINER_REQUEST *me)
{
	me->refcount--;
	if(!me->refcount)
	{
		free(me);
		return 0;
	}
	return me->refcount;
}
