#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_info.h"

static CONTAINER *container_create_(CONTAINERWARE *cw);
static unsigned long container_addref_(CONTAINER *me);
static unsigned long container_release_(CONTAINER *me);
static int container_instance_(CONTAINER *me, CONTAINER_INSTANCE_HOST *host, const CONTAINER_INSTANCE_INFO *info, CONTAINER_INSTANCE **out);

static struct container_api_struct container_api_ =
{
	NULL,
	container_addref_,
	container_release_,
	container_instance_
};

int
containerware_module_init(CONTAINERWARE *cw)
{
	CONTAINER *p;
	int r;
	
	p = container_create_(cw);
	if(!p)
	{
		return -1;
	}
	r = cw->api->register_container(cw, CONTAINER_NAME, p);
	p->api->release(p);
	return r;
}

static CONTAINER *
container_create_(CONTAINERWARE *cw)
{
	CONTAINER *p;
	
	p = (CONTAINER *) calloc(1, sizeof(CONTAINER));
	if(!p)
	{
		return NULL;
	}
	pthread_mutex_init(&(p->lock), NULL);
	p->refcount = 1;
	p->api = &container_api_;
	cw->api->addref(cw);
	p->cw = cw;
	return p;
}

static unsigned long
container_addref_(CONTAINER *me)
{
	unsigned long r;
	
	pthread_mutex_lock(&(me->lock));
	me->refcount++;
	r = me->refcount;
	pthread_mutex_unlock(&(me->lock));
	return r;
}

static unsigned long
container_release_(CONTAINER *me)
{
	unsigned long r;

	pthread_mutex_lock(&(me->lock));
	me->refcount--;
	r = me->refcount;
	pthread_mutex_unlock(&(me->lock));
	if(!r)
	{
		me->cw->api->release(me->cw);
		pthread_mutex_destroy(&(me->lock));
		free(me);
	}
	return r;
}

static int
container_instance_(CONTAINER *me, CONTAINER_INSTANCE_HOST *host, const CONTAINER_INSTANCE_INFO *info, CONTAINER_INSTANCE **out)
{
	CONTAINER_INSTANCE *p;
	
	(void) me;
	
	*out = instance_create_(me, host, info);
	if(!*out)
	{
		return -1;
	}
	return 0;
}
