#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define LOG_FACILITY                   "worker"

#include "p_containerware.h"

static int worker_populate_info_(CONTAINER_HOST *host, CONTAINER_WORKER_INFO *info);
static int worker_request_(CONTAINER_WORKER_HOST *me, CONTAINER_REQUEST **req);
static int worker_log_request_(CONTAINER_WORKER_HOST *me, CONTAINER_REQUEST *req);

static struct container_worker_host_api_struct worker_api_ =
{
	NULL,
	NULL,
	NULL,
	worker_request_
};

static uint32_t worker_next_id = 100;

/* Create a new worker and add it to a container's list of active
 * workers.
 *
 * Threading: thread-safe
 */
CONTAINER_WORKER_HOST *
worker_create(CONTAINER_HOST *host)
{
	CONTAINER_WORKER_HOST *p;
	CONTAINER_WORKER_INFO info;
	int r;
	
	worker_populate_info_(host, &info);
	DPRINTF("creating new worker %s.%s/%s@%lu/%lu", info.app, info.instance, info.cluster, (unsigned long) info.pid, (unsigned long) info.workerid);
	p = (CONTAINER_WORKER_HOST *) calloc(1, sizeof(CONTAINER_WORKER_HOST));
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
	p->api = &worker_api_;
	p->container_host = host;
	p->state = WS_EMPTY;
	memcpy(&p->info, &info, sizeof(info));
	DPRINTF("obtaining worker from container");
	r = host->container->api->worker(host->container, p, &info, &(p->worker));
	if(r)
	{
		LPRINTF(LOG_CRIT, "failed to obtain new worker from container: %s", strerror(errno));
		p->worker = NULL;
	}
	else if(!p->worker)
	{
		LPRINTF(LOG_CRIT, "failed to obtain new worker from container (no error reported)");
	}
	if(!p->worker)
	{
		pthread_mutex_unlock(&(p->mutex));
		pthread_mutex_destroy(&(p->mutex));
		free(p->requests);
		free(p);	
		return NULL;
	}
	worker_list_wrlock(&(host->workers));
	r = worker_list_add_unlocked(&(host->workers), p);
	if(r)
	{
		worker_list_unlock(&(host->workers));
		pthread_mutex_unlock(&(p->mutex));
		pthread_mutex_destroy(&(p->mutex));
		free(p->requests);
		free(p);
		return NULL;
	}
	host->active++;
	pthread_cond_init(&(p->cond), NULL);
	worker_list_unlock(&(host->workers));
	worker_thread_create(p);
	DPRINTF("new worker %s.%s/%s@%lu/%lu created on thread 0x%08x", info.app, info.instance, info.cluster, (unsigned long) info.pid, (unsigned long) info.workerid, (unsigned long) p->thread);
	pthread_mutex_unlock(&(p->mutex));
	return p;
}

/* Locate the most suitable worker to service a request, creating a new
 * worker if required.
 *
 * Threading: thread-safe
 */
CONTAINER_WORKER_HOST *
worker_locate(CONTAINER_HOST *host)
{
	size_t c, nreqs;
	CONTAINER_WORKER_HOST *p;
	
	worker_list_rdlock(&(host->workers));
	DPRINTF("container has %lu active workers", (unsigned long) host->active);
	if(host->active)
	{
		/* Find the least-loaded child to service the request */
		/* lock */
		nreqs = 0;
		p = NULL;
		for(c = 0; c < host->workers.allocated; c++)
		{
			if(host->workers.list[c] &&
				(host->workers.list[c]->state == WS_IDLE ||
					host->workers.list[c]->state == WS_RUNNING))
			{
				if(!p || host->workers.list[c]->requestcount < nreqs)
				{
					nreqs = host->workers.list[c]->requestcount;
					p = host->workers.list[c];
				}
			}
		}		
		if(p && nreqs && host->active < host->maxchildren)
		{
			/* Create a new worker to service this request instead
			 * of queueing
			 */
			p = NULL;
		}
		if(p)
		{
			DPRINTF("located worker with %lu pending requests", (unsigned long) nreqs);
			/* XXX
			p->api->addref(p);
			*/
			worker_list_unlock(&(host->workers));
			return p;
		}
	}
	worker_list_unlock(&(host->workers));
	DPRINTF("no suitable worker found for request processing");
	return worker_create(host);
}

/* Forward a request to an worker
 *
 * Threading: thread-safe
 */
int
worker_queue_request(CONTAINER_WORKER_HOST *host, CONTAINER_REQUEST *req, LISTENER *source)
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

/* Populate an worker information structure
 *
 * Threading: the host's lock must be held
 */
static int
worker_populate_info_(CONTAINER_HOST *host, CONTAINER_WORKER_INFO *info)
{
	memset(info, 0, sizeof(CONTAINER_WORKER_INFO));
	strncpy(info->app, iniparser_getstring(host->config, "name", "default"), sizeof(info->app) - 1);
	strncpy(info->instance, iniparser_getstring(host->config, "instance", "localhost"), sizeof(info->instance) - 1);
	strncpy(info->cluster, iniparser_getstring(host->config, "cluster", "private"), sizeof(info->cluster) - 1);
	info->pid = getpid();
	info->workerid = worker_next_id;
	worker_next_id++;
	return 0;
}


/* Log a LOG_ACCESS message for the request
 *
 * Threading: the worker host's lock must be held
 */
static int
worker_log_request_(CONTAINER_WORKER_HOST *me, CONTAINER_REQUEST *req)
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

/* Wait for a request to be routed to an worker
 *
 * Threading: thread-safe; always invoked on a worker thread
 */
static int
worker_request_(CONTAINER_WORKER_HOST *me, CONTAINER_REQUEST **req)
{
	
	*req = NULL;
	pthread_mutex_lock(&(me->mutex));
	if(me->state == WS_RUNNING && me->current)
	{
		worker_log_request_(me, me->current);
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
	if(me->state == WS_ZOMBIE)
	{
		DHPRINTF(me, "this thread has already been terminated");
		pthread_mutex_unlock(&(me->mutex));
		errno = EPERM;
		return -1;		
	}
	me->state = WS_IDLE;
	if(me->requestcount)
	{
		DHPRINTF(me, "returning pending request");
		*req = me->current = me->requests[0];
		me->state = WS_RUNNING;
		pthread_mutex_unlock(&(me->mutex));
		return 0;
	}
	DHPRINTF(me, "waiting for a request");
	pthread_cond_wait(&(me->cond), &(me->mutex));
	if(!me->requestcount)
	{
		DHPRINTF(me, "this thread will terminate");
		me->state = WS_ZOMBIE;
		pthread_mutex_unlock(&(me->mutex));
		return 0;
	}
	DHPRINTF(me, "returning new request");
	*req = me->current = me->requests[0];
	me->state = WS_RUNNING;
	pthread_mutex_unlock(&(me->mutex));
	return 0;
}
