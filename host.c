#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_containerware.h"

static struct host_list_struct hosts;

static int host_request_(CONTAINER_INSTANCE_HOST *me, CONTAINER_REQUEST **req);
static void *host_thread_(void *arg);

static struct container_instance_host_api_struct host_api_ =
{
	NULL,
	NULL,
	NULL,
	host_request_
};

int
host_init(void)
{
	host_list_init(&hosts, 8);
	return 0;
}

CONTAINER_HOST *
host_add(const char *name)
{
	CONTAINER *c;
	CONTAINER_HOST *r;
	
	c = NULL;
	if(!strcmp(name, "debug"))
	{
		c = debug_container();
	}
	if(!c)
	{
		errno = EINVAL;
		return NULL;
	}
	r = host_add_container(c);
	if(!r)
	{
		c->api->release(c);
		return NULL;
	}
	return r;
}

CONTAINER_HOST *
host_add_container(CONTAINER *container)
{
	CONTAINER_HOST *p;
	int r;
	
	fprintf(stderr, "host: adding new container\n");
	p = (CONTAINER_HOST *) calloc(1, sizeof(CONTAINER_HOST));
	if(!p)
	{	
		return NULL;
	}
	p->container = container;
	/* Supply some defaults */
	p->minchildren = 0;
	p->maxchildren = 128;
	instance_list_init(&(p->instances), 16);
	r = host_list_add(&hosts, p);
	if(r)
	{
		/* instance_list_destroy(&(p->instances)); */
		free(p);
		return NULL;
	}
	fprintf(stderr, "host: added container\n");
	return p;
}

int
host_set_minchildren(CONTAINER_HOST *host, size_t minchildren)
{
	size_t c;
	
	/* XXX locking all over the place */
	if(host->maxchildren && minchildren > host->maxchildren)
	{
		minchildren = host->maxchildren;
	}
	fprintf(stderr, "host: setting minimum number of instances to %lu\n", (unsigned long) minchildren);
	if(minchildren < host->minchildren)
	{
		host->minchildren = minchildren;
		return 0;
	}
	if(minchildren > host->minchildren)
	{
		if(minchildren > host->active)
		{
			c = minchildren - host->active;
			fprintf(stderr, "host: will launch %lu additional instances\n", (unsigned long) c);
			host->minchildren = minchildren;
			for(; c; c--)
			{
				host_create_instance(host);
			}
		}
	}
	return 0;
}

/* Locate the most suitable instance to service a request, creating a new
 * instance if required.
 */
CONTAINER_INSTANCE_HOST *
host_locate_instance(CONTAINER_HOST *host)
{
	size_t c, nreqs;
	CONTAINER_INSTANCE_HOST *p;
	
	instance_list_rdlock(&(host->instances));
	fprintf(stderr, "host: container has %lu active instances\n", (unsigned long) host->active);
	if(host->active)
	{
		/* Find the least-loaded child to service the request */
		/* lock */
		nreqs = 0;
		p = NULL;
		for(c = 0; c < host->instances.allocated; c++)
		{
			if(host->instances.list[c] &&
				(host->instances.list[c]->state == IS_IDLE ||
					host->instances.list[c]->state == IS_RUNNING))
			{
				if(!p || host->instances.list[c]->requestcount < nreqs)
				{
					nreqs = host->instances.list[c]->requestcount;
					p = host->instances.list[c];
				}
			}
		}		
		if(p && nreqs && host->active < host->maxchildren)
		{
			/* Create a new instance to service this request instead
			 * of queueing
			 */
			p = NULL;
		}
		if(p)
		{
			fprintf(stderr, "host: located instance with %lu pending requests\n", (unsigned long) nreqs);
			/* XXX
			p->api->addref(p);
			*/
			instance_list_unlock(&(host->instances));
			return p;
		}
	}
	instance_list_unlock(&(host->instances));
	fprintf(stderr, "host: no suitable instance found for request processing\n");
	return host_create_instance(host);
}

/* Create an instance */
CONTAINER_INSTANCE_HOST *
host_create_instance(CONTAINER_HOST *host)
{
	CONTAINER_INSTANCE_HOST *p;
	CONTAINER_INSTANCE_INFO info = { "localhost", "private" };
	int r;
	
	fprintf(stderr, "host: creating a new instance for [%s/%s]\n", info.name, info.cluster);
	p = (CONTAINER_INSTANCE_HOST *) calloc(1, sizeof(CONTAINER_INSTANCE_HOST));
	if(!p)
	{
		return NULL;
	}
	p->requestalloc = 32;
	p->requests = (CONTAINER_REQUEST **) calloc(p->requestalloc, sizeof(CONTAINER_REQUEST *));
	if(!p->requests)
	{
		free(p);
		return NULL;
	}
	pthread_mutex_init(&(p->mutex), NULL);
	pthread_mutex_lock(&(p->mutex));
	p->api = &host_api_;
	p->container_host = host;
	p->state = IS_EMPTY;
	fprintf(stderr, "host: obtaining instance from container\n");
	r = host->container->api->instance(host->container, p, &info, &(p->instance));
	if(r)
	{
		fprintf(stderr, "host: failed to obtain instance: %s\n", strerror(errno));
		p->instance = NULL;
	}
	else if(!p->instance)
	{
		fprintf(stderr, "host: failed to obtain instance with no error reported\n");		
	}
	if(!p->instance)
	{
		pthread_mutex_unlock(&(p->mutex));
		pthread_mutex_destroy(&(p->mutex));
		free(p->requests);
		free(p);	
		return NULL;
	}
	instance_list_wrlock(&(host->instances));
	r = instance_list_add_unlocked(&(host->instances), p);
	if(r)
	{
		instance_list_unlock(&(host->instances));
		pthread_mutex_unlock(&(p->mutex));
		pthread_mutex_destroy(&(p->mutex));
		free(p->requests);
		free(p);
		return NULL;
	}
	host->active++;
	pthread_cond_init(&(p->cond), NULL);
	instance_list_unlock(&(host->instances));	
	pthread_create(&(p->thread), NULL, host_thread_, p);
	fprintf(stderr, "host: new instance [%s/%s] (%lu/%lu) created and ready to execute\n", info.name, info.cluster, (unsigned long) getpid(), (unsigned long) p->thread);
	pthread_mutex_unlock(&(p->mutex));
	return p;
}

/* Forward a request to an instance */
int
host_queue_request(CONTAINER_INSTANCE_HOST *host, CONTAINER_REQUEST *req, LISTENER *source)
{
	(void) source;
	
	pthread_mutex_lock(&(host->mutex));
	if(host->requestcount + 1 > host->requestalloc)
	{
		fprintf(stderr, "host: failed to queue request, insufficient queue space\n");
		pthread_mutex_unlock(&(host->mutex));
		errno = ENOMEM;
		return -1;
	}
	req->api->addref(req);
	host->requests[host->requestcount] = req;
	host->requestcount++;
	fprintf(stderr, "host: queued new request, request count is now %d\n", (int) host->requestcount);
	pthread_cond_signal(&(host->cond));
	pthread_mutex_unlock(&(host->mutex));
	return 0;
}

/* Wait for a request to be routed to an instance */
static int
host_request_(CONTAINER_INSTANCE_HOST *me, CONTAINER_REQUEST **req)
{
	*req = NULL;
	pthread_mutex_lock(&(me->mutex));
	if(me->state == IS_RUNNING && me->current)
	{
		fprintf(stderr, "host: request processing complete\n");
		if(me->requestcount > 1)
		{
			memcpy(&me->requests, &(me->requests[1]), (me->requestcount - 1) * sizeof(CONTAINER_REQUEST *));
		}
		me->requestcount--;
		me->current->api->release(me->current);
		me->current = NULL;
		fprintf(stderr, "host: request count is now %d\n", (int) me->requestcount);
	}
	if(me->state == IS_ZOMBIE)
	{
		fprintf(stderr, "host: this thread has already been terminated\n");
		pthread_mutex_unlock(&(me->mutex));
		errno = EPERM;
		return -1;		
	}
	me->state = IS_IDLE;
	if(me->requestcount)
	{
		fprintf(stderr, "host: returning pending request\n");
		*req = me->current = me->requests[0];
		me->state = IS_RUNNING;
		pthread_mutex_unlock(&(me->mutex));
		return 0;
	}
	fprintf(stderr, "host: waiting for a request\n");
	pthread_cond_wait(&(me->cond), &(me->mutex));
	if(!me->requestcount)
	{
		fprintf(stderr, "host: this thread will terminate\n");
		me->state = IS_ZOMBIE;
		pthread_mutex_unlock(&(me->mutex));
		return 0;
	}
	fprintf(stderr, "host: returning new request\n");
	*req = me->current = me->requests[0];
	me->state = IS_RUNNING;
	pthread_mutex_unlock(&(me->mutex));
	return 0;
}

static void *
host_thread_(void *arg)
{
	CONTAINER_INSTANCE_HOST *me;
	int r;
	
	me = (CONTAINER_INSTANCE_HOST *) arg;
	fprintf(stderr, "host[%lu/%lu]: waiting for instance to become initialised\n", (unsigned long) getpid(), (unsigned long) pthread_self());
	pthread_mutex_lock(&(me->mutex));
	me->state = IS_IDLE;
	me->status = -1;
	if(!me->instance)
	{
		fprintf(stderr, "host[%lu/%lu]: instance host is not associated with any instance!\n", (unsigned long) getpid(), (unsigned long) pthread_self());
		me->state = IS_ZOMBIE;
		me->status = 255;
		pthread_mutex_unlock(&(me->mutex));
		return NULL;
	}
	fprintf(stderr, "host[%lu/%lu]: passing control to the instance\n", (unsigned long) getpid(), (unsigned long) pthread_self());
	pthread_mutex_unlock(&(me->mutex));
	r = me->instance->api->process(me->instance);
	fprintf(stderr, "host[%lu/%lu]: instance terminated with exit status %d\n", (unsigned long) getpid(), (unsigned long) pthread_self(), r);
	pthread_mutex_lock(&(me->mutex));
	me->state = IS_ZOMBIE;
	me->status = r;
	pthread_mutex_unlock(&(me->mutex));
	return NULL;	
}
