#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define LOG_FACILITY                   "instance"

#include "p_containerware.h"

static int instance_populate_info_(CONTAINER_HOST *host, CONTAINER_INSTANCE_INFO *info);
static int instance_request_(CONTAINER_INSTANCE_HOST *me, CONTAINER_REQUEST **req);
static int instance_log_request_(CONTAINER_INSTANCE_HOST *me, CONTAINER_REQUEST *req);

static struct container_instance_host_api_struct instance_api_ =
{
	NULL,
	NULL,
	NULL,
	instance_request_
};

static uint32_t host_next_threadid = 100;

/* Create a new instance and add it to a container's list of active
 * instances.
 *
 * Threading: thread-safe
 */
CONTAINER_INSTANCE_HOST *
instance_create(CONTAINER_HOST *host)
{
	CONTAINER_INSTANCE_HOST *p;
	CONTAINER_INSTANCE_INFO info;
	int r;
	
	instance_populate_info_(host, &info);
	DPRINTF("creating new instance %s.%s/%s@%lu/%lu", info.app, info.instance, info.cluster, (unsigned long) info.pid, (unsigned long) info.threadid);
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
	p->api = &instance_api_;
	p->container_host = host;
	p->state = IS_EMPTY;
	memcpy(&p->info, &info, sizeof(info));
	DPRINTF("obtaining instance from container");
	r = host->container->api->instance(host->container, p, &info, &(p->instance));
	if(r)
	{
		LPRINTF(LOG_CRIT, "failed to obtain new instance from container: %s", strerror(errno));
		p->instance = NULL;
	}
	else if(!p->instance)
	{
		LPRINTF(LOG_CRIT, "failed to obtain new instance from container (no error reported)");
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
	instance_thread_create(p);
	DPRINTF("new instance %s.%s/%s@%lu/%lu created on thread 0x%08x", info.app, info.instance, info.cluster, (unsigned long) info.pid, (unsigned long) info.threadid, (unsigned long) p->thread);
	pthread_mutex_unlock(&(p->mutex));
	return p;
}

/* Locate the most suitable instance to service a request, creating a new
 * instance if required.
 *
 * Threading: thread-safe
 */
CONTAINER_INSTANCE_HOST *
instance_locate(CONTAINER_HOST *host)
{
	size_t c, nreqs;
	CONTAINER_INSTANCE_HOST *p;
	
	instance_list_rdlock(&(host->instances));
	DPRINTF("container has %lu active instances", (unsigned long) host->active);
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
			DPRINTF("located instance with %lu pending requests", (unsigned long) nreqs);
			/* XXX
			p->api->addref(p);
			*/
			instance_list_unlock(&(host->instances));
			return p;
		}
	}
	instance_list_unlock(&(host->instances));
	DPRINTF("no suitable instance found for request processing");
	return instance_create(host);
}

/* Forward a request to an instance
 *
 * Threading: thread-safe
 */
int
instance_queue_request(CONTAINER_INSTANCE_HOST *host, CONTAINER_REQUEST *req, LISTENER *source)
{
	(void) source;
	
	pthread_mutex_lock(&(host->mutex));
	if(host->requestcount + 1 > host->requestalloc)
	{
		DHPRINTF(host, "failed to queue request due to insufficient queue space");
		pthread_mutex_unlock(&(host->mutex));
		errno = ENOMEM;
		return -1;
	}
	req->api->addref(req);
	host->requests[host->requestcount] = req;
	host->requestcount++;
	DHPRINTF(host, "queued new request; request count is now %d", (int) host->requestcount);
	pthread_cond_signal(&(host->cond));
	pthread_mutex_unlock(&(host->mutex));
	return 0;
}

/* Populate an instance information structure
 *
 * Threading: the host's lock must be held
 */
static int
instance_populate_info_(CONTAINER_HOST *host, CONTAINER_INSTANCE_INFO *info)
{
	memset(info, 0, sizeof(CONTAINER_INSTANCE_INFO));
	strncpy(info->app, iniparser_getstring(host->config, "name", "default"), sizeof(info->app) - 1);
	strncpy(info->instance, iniparser_getstring(host->config, "instance", "localhost"), sizeof(info->instance) - 1);
	strncpy(info->cluster, iniparser_getstring(host->config, "cluster", "private"), sizeof(info->cluster) - 1);
	info->pid = getpid();
	info->threadid = host_next_threadid;
	host_next_threadid++;
	return 0;
}


/* Log a LOG_ACCESS message for the request
 *
 * Threading: the instance host's lock must be held
 */
static int
instance_log_request_(CONTAINER_INSTANCE_HOST *me, CONTAINER_REQUEST *req)
{
	const char *host, *ident, *user, *method, *path, *protocol;
	int status;
	size_t bytes;
	char bbuf[48], tbuf[64];
	struct timeval tv;
	struct tm tm;

	host = req->api->getenv(req, "REMOTE_ADDR");
	if(!host)
	{
		host = "-";
	}
	ident = "-";
	user = req->api->getenv(req, "REMOTE_USER");
	if(!user)
	{
		user = "-";
	}
	method = req->api->method(req);
	if(!method)
	{
		method = "-";
	}
	path = req->api->request_uri_str(req);
	if(!path)
	{
		path = "-";
	}
	protocol = req->api->getenv(req, "SERVER_PROTOCOL");
	if(!protocol)
	{
		protocol = "-";
	}
	status = 200;
	bytes = (size_t) -1;
	if(bytes == (size_t) -1)
	{
		strcpy(bbuf, "-");
	}
	else
	{
		snprintf(bbuf, sizeof(bbuf), "%lu", (unsigned long) bytes);
	}
	req->api->timestamp(req, &tv);
	gmtime_r((time_t *) &(tv.tv_sec), &tm);
	strftime(tbuf, sizeof(tbuf), "%d/%b/%Y:%H:%M:%S %z", &tm);
	HLPRINTF(me, LOG_ACCESS, "%s %s %s [%s] \"%s %s %s\" %d %s", host, ident, user, tbuf, method, path, protocol, status, bbuf);
	return 0;
}

/* Wait for a request to be routed to an instance
 *
 * Threading: thread-safe; always invoked on an instance thread
 */
static int
instance_request_(CONTAINER_INSTANCE_HOST *me, CONTAINER_REQUEST **req)
{
	
	*req = NULL;
	pthread_mutex_lock(&(me->mutex));
	if(me->state == IS_RUNNING && me->current)
	{
		instance_log_request_(me, me->current);
		DHPRINTF(me, "request processing complete");
		if(me->requestcount > 1)
		{
			memcpy(&me->requests, &(me->requests[1]), (me->requestcount - 1) * sizeof(CONTAINER_REQUEST *));
		}
		me->requestcount--;
		me->current->api->release(me->current);
		me->current = NULL;
		DHPRINTF(me, "request count is now %d", (int) me->requestcount);
	}
	if(me->state == IS_ZOMBIE)
	{
		DHPRINTF(me, "this thread has already been terminated");
		pthread_mutex_unlock(&(me->mutex));
		errno = EPERM;
		return -1;		
	}
	me->state = IS_IDLE;
	if(me->requestcount)
	{
		DHPRINTF(me, "returning pending request");
		*req = me->current = me->requests[0];
		me->state = IS_RUNNING;
		pthread_mutex_unlock(&(me->mutex));
		return 0;
	}
	DHPRINTF(me, "waiting for a request");
	pthread_cond_wait(&(me->cond), &(me->mutex));
	if(!me->requestcount)
	{
		DHPRINTF(me, "this thread will terminate");
		me->state = IS_ZOMBIE;
		pthread_mutex_unlock(&(me->mutex));
		return 0;
	}
	DHPRINTF(me, "returning new request");
	*req = me->current = me->requests[0];
	me->state = IS_RUNNING;
	pthread_mutex_unlock(&(me->mutex));
	return 0;
}
