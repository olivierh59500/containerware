#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define CONTAINER_PLUGIN_INTERNAL      1

#include "p_containerware.h"

struct container_struct
{
	CONTAINER_COMMON;
};

struct container_instance_struct
{
	CONTAINER_INSTANCE_COMMON;
	CONTAINER_INSTANCE_INFO info;
	CONTAINER_INSTANCE_HOST *host;
	int terminated;
};

static int debug_container_instance_(CONTAINER *me, CONTAINER_INSTANCE_HOST *host, const CONTAINER_INSTANCE_INFO *info, CONTAINER_INSTANCE **out);
static unsigned long debug_release_(CONTAINER_INSTANCE *me);
static int debug_process_(CONTAINER_INSTANCE *me);
static int debug_process_request_(CONTAINER_INSTANCE *me, CONTAINER_REQUEST *req);

static struct container_api_struct debug_api_ =
{
	NULL,
	NULL,
	NULL,
	debug_container_instance_
};

static struct container_instance_api_struct debug_instance_api_ =
{
	NULL,
	NULL,
	debug_release_,
	debug_process_
};

/* Implementation of the debug container. All a container needs to do is create an
 * instance when requested.
 */
	 
CONTAINER *
debug_container(void)
{
	CONTAINER *p;
	
	p = (CONTAINER *) calloc(1, sizeof(CONTAINER));
	if(!p)
	{
		return NULL;
	}
	p->api = &debug_api_;
	return p;
}
	 
static int
debug_container_instance_(CONTAINER *me, CONTAINER_INSTANCE_HOST *host, const CONTAINER_INSTANCE_INFO *info, CONTAINER_INSTANCE **out)
{
	CONTAINER_INSTANCE *p;
	
	(void) me;
	
	*out = NULL;
	p = (CONTAINER_INSTANCE *) calloc(1, sizeof(CONTAINER_INSTANCE));
	if(!p)
	{
		return -1;
	}
	p->api = &debug_instance_api_;
	memcpy(&(p->info), info, sizeof(CONTAINER_INSTANCE_INFO));
	p->host = host;
	*out = p;
	return 0;
}

/* Implementation of the debug instance. Note that all activity within
 * process() is synchronous: multiprocessing is dealt with by ContainerWare
 * itself.
 */
	 
unsigned long
debug_release_(CONTAINER_INSTANCE *me)
{
 	me->terminated = 1;
	return 0;
}

int
debug_process_(CONTAINER_INSTANCE *me)
{
	CONTAINER_REQUEST *req;
	
	fprintf(stderr, "debug: processing started\n");
	while(!me->terminated)
	{
		req = NULL;
		if(me->host->api->request(me->host, &req) == -1)
		{
			fprintf(stderr, "debug: container::request() failed: %s\n", strerror(errno));
			return -1;
		}
		debug_process_request_(me, req);
	}
	fprintf(stderr, "debug: terminating\n");
	return 0;
}

static int
debug_process_request_(CONTAINER_INSTANCE *me, CONTAINER_REQUEST *req)
{
	(void) me;
	
	fprintf(stderr, "debug: --- processing request (%08lx) on thread %lu/%lu\n", (unsigned long) req, (unsigned long) getpid(), (unsigned long) pthread_self());
	fprintf(stderr, "debug: protocol =    %s\n", req->api->protocol(req));
	fprintf(stderr, "debug: method =      %s\n", req->api->method(req));
	fprintf(stderr, "debug: Request-URI = %s\n", req->api->request_uri_str(req));
	fprintf(stderr, "debug: sleeping to simulate processing delay...\n");
	sleep(8);
	fprintf(stderr, "debug: --- completed processing request (%08lx) on thread %lu/%lu\n", (unsigned long) req, (unsigned long) getpid(), (unsigned long) pthread_self());
	return 0;
}