#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_fcgi.h"

static ENDPOINT_SERVER *server_create_(CONTAINERWARE *cw);
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
	
	FCGX_Init();
	p = server_create_(cw);
	if(!p)
	{
		LPRINTF(cw, CWLOG_ERR, "failed to create endpoint server");
		return -1;
	}
	r = cw->api->register_endpoint(cw, ENDPOINT_SCHEME, p);
	p->api->release(p);
	return r;
}

static ENDPOINT_SERVER *
server_create_(CONTAINERWARE *cw)
{
	ENDPOINT_SERVER *p;
	
	p = (ENDPOINT_SERVER *) calloc(1, sizeof(ENDPOINT_SERVER));
	if(!p)
	{
		return NULL;
	}
	pthread_mutex_init(&(p->lock), NULL);
	p->refcount = 1;
	p->api = &server_api_;
	cw->api->addref(cw);
	p->cw = cw;
	return p;
}

static unsigned long
server_addref_(ENDPOINT_SERVER *me)
{
	return cw_addref(&(me->lock), &(me->refcount));
}

static unsigned long
server_release_(ENDPOINT_SERVER *me)
{
	unsigned long r;

	r = cw_release(&(me->lock), &(me->refcount));
	if(!r)
	{
		me->cw->api->release(me->cw);
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
