#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_dummy.h"

static unsigned long server_addref_(ENDPOINT_SERVER *me);
static unsigned long server_release_(ENDPOINT_SERVER *me);
static int server_endpoint_(ENDPOINT_SERVER *me, URI *uri, ENDPOINT **ep);

static struct endpoint_server_api_struct server_api_ =
{
	NULL,
	server_addref_,
	server_release_,
	server_endpoint_
};

int
containerware_module_init(CONTAINERWARE *cw)
{
	ENDPOINT_SERVER *p;
	int r;
	
	p = (ENDPOINT_SERVER *) calloc(1, sizeof(ENDPOINT_SERVER));
	if(!p)
	{
		return -1;
	}
	pthread_mutex_init(&(p->lock), NULL);
	p->refcount = 1;
	p->api = &server_api_;
	r = cw->api->register_endpoint(cw, ENDPOINT_SCHEME, p);
	p->api->release(p);
	return r;
}

static unsigned long
server_addref_(ENDPOINT_SERVER *me)
{
	unsigned long r;
	
	pthread_mutex_lock(&(me->lock));
	me->refcount++;
	r = me->refcount;
	pthread_mutex_unlock(&(me->lock));
	return r;
}

static unsigned long
server_release_(ENDPOINT_SERVER *me)
{
	unsigned long r;

	pthread_mutex_lock(&(me->lock));
	me->refcount--;
	r = me->refcount;
	pthread_mutex_unlock(&(me->lock));
	if(!r)
	{
		pthread_mutex_destroy(&(me->lock));
		free(me);
	}
	return r;
}

static int
server_endpoint_(ENDPOINT_SERVER *me, URI *uri, ENDPOINT **ep)
{	
	*ep = endpoint_create_(me, uri);
	if(!*ep)
	{
		return -1;
	}
	return 0;
}
