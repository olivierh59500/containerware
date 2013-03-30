#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_dummy.h"

static unsigned long endpoint_addref_(ENDPOINT *me);
static unsigned long endpoint_release_(ENDPOINT *me);
static ENDPOINT_MODE endpoint_mode_(ENDPOINT *me);
static int endpoint_fd_(ENDPOINT *me);
static int endpoint_process_(ENDPOINT *me);
static int endpoint_acquire_(ENDPOINT *me, CONTAINER_REQUEST **req);

static struct endpoint_api_struct endpoint_api_ =
{
	NULL,
	endpoint_addref_,
	endpoint_release_,
	endpoint_mode_,
	endpoint_fd_,
	endpoint_process_,
	endpoint_acquire_
};

ENDPOINT *
endpoint_create_(ENDPOINT_SERVER *server, URI *uri)
{
	ENDPOINT *p;
	
	(void) uri;
	
	p = (ENDPOINT *) calloc(1, sizeof(ENDPOINT));
	if(!p)
	{
		return NULL;
	}
	server->api->addref(server);
	p->server = server;
	p->refcount = 1;
	p->api = &endpoint_api_;
	pthread_mutex_init(&(p->lock), NULL);
	return p;
}

static unsigned long
endpoint_addref_(ENDPOINT *me)
{
	unsigned long r;
	
	pthread_mutex_lock(&(me->lock));
	me->refcount++;
	r = me->refcount;
	pthread_mutex_unlock(&(me->lock));
	return r;
}

static unsigned long
endpoint_release_(ENDPOINT *me)
{
	unsigned long r;

	pthread_mutex_lock(&(me->lock));
	me->refcount--;
	r = me->refcount;
	pthread_mutex_unlock(&(me->lock));
	if(!r)
	{
		me->server->api->release(me->server);
		pthread_mutex_destroy(&(me->lock));
		free(me);
	}
	return r;
}

ENDPOINT_MODE
endpoint_mode_(ENDPOINT *me)
{
	(void) me;
	
	/* Return EM_THREAD to avoid having to provide a file descriptor
	 * to listen on.
	 */
	return EM_THREAD;
}

int
endpoint_fd_(ENDPOINT *me)
{
	(void) me;
	
	return -1;
}

int
endpoint_process_(ENDPOINT *me)
{
	for(;;)
	{
		sleep(5);
		me->acquired = 0;
		fprintf(stderr, "dummy: a new request is ready to be acquired\n");
		return 1;
	}
	return 0;
}

int
endpoint_acquire_(ENDPOINT *me, CONTAINER_REQUEST **req)
{
	*req = NULL;
	if(me->acquired)
	{
		errno = EWOULDBLOCK;
		return -1;
	}
	*req = request_create_(me);
	if(!*req)
	{
		return -1;
	}
	me->acquired = 1;
	fprintf(stderr, "dummy: returning new request 0x%08lx\n", (unsigned long) (*req));
	return 0;
}
