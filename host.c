#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define LOG_FACILITY                   "host"

#include "p_containerware.h"

static struct host_list_struct hosts;

int
host_init(void)
{
	host_list_init(&hosts, 8);
	return 0;
}

/* Create a new container, returning the host which encapsulates it.
 * The host and container are added to the global list of hosts.
 *
 * Threading: thread-safe
 */
CONTAINER_HOST *
host_add(dictionary *config)
{
	CONTAINER *c;
	CONTAINER_HOST *r;
	const char *handler;
	
	c = NULL;
	handler = iniparser_getstring(config, "handler", iniparser_getstring(config, "name", NULL));
	if(!handler)
	{
		return NULL;		
	}
	if(container_locate(handler, &c) == -1)
	{
		return -1;
	}
	if(!c)
	{
		errno = EINVAL;
		return NULL;
	}
	r = host_add_container(c, config);
	c->api->release(c);
	if(!r)
	{
		return NULL;
	}
	return r;
}

/* Create a host for a container. The container will be retained by
 * the host.
 *
 * The host and container are added to the global list of hosts.
 *
 * Threading: thread-safe
 */
CONTAINER_HOST *
host_add_container(CONTAINER *container, dictionary *config)
{
	CONTAINER_HOST *p;
	int r;
	
	DPRINTF("adding a new container");
	p = (CONTAINER_HOST *) calloc(1, sizeof(CONTAINER_HOST));
	if(!p)
	{	
		return NULL;
	}
	container->api->addref(container);
	pthread_mutex_init(&(p->lock), NULL);
	pthread_mutex_lock(&(p->lock));
	p->container = container;
	/* Supply some defaults */
	p->minchildren = 0;
	p->maxchildren = 128;
	p->config = config_copydict(config);
	worker_list_init(&(p->workers), 16);
	r = host_list_add(&hosts, p);
	if(r == -1)
	{
		/* worker_list_destroy(&(p->workers)); */
		container->api->release(container);
		pthread_mutex_unlock(&(p->lock));		
		pthread_mutex_destroy(&(p->lock));
		free(p);
		return NULL;
	}
	pthread_mutex_unlock(&(p->lock));		
	/* Configure the container */
	host_set_minworkers(p, iniparser_getint(p->config, "minworkers", p->minchildren));
/*	host_set_maxworkers(p, iniparser_getint(p->config, "maxworkers", p->minchildren)); */
	DPRINTF("new container has been added");
	return p;
}

/* Set the minimum number of instances this container will
 * have.
 *
 * Threading: XXX
 */
int
host_set_minworkers(CONTAINER_HOST *host, size_t minchildren)
{
	size_t c;
	
	/* XXX locking all over the place */
	if(host->maxchildren && minchildren > host->maxchildren)
	{
		minchildren = host->maxchildren;
	}
	DPRINTF("setting minimum number of workers to %lu", (unsigned long) minchildren);
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
			DPRINTF("will launch %lu additional workers", (unsigned long) c);
			host->minchildren = minchildren;
			for(; c; c--)
			{
				worker_create(host);
			}
		}
	}
	return 0;
}
