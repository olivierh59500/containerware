#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_info.h"

static unsigned long worker_addref_(CONTAINER_WORKER *me);
static unsigned long worker_release_(CONTAINER_WORKER *me);
static int worker_process_(CONTAINER_WORKER *me);
static int worker_process_request_(CONTAINER_WORKER *me, CONTAINER_REQUEST *req);

static struct container_worker_api_struct worker_api_ =
{
	NULL,
	worker_addref_,
	worker_release_,
	worker_process_
};

CONTAINER_WORKER *
worker_create_(CONTAINER *container, CONTAINER_WORKER_HOST *host, const CONTAINER_WORKER_INFO *info)
{
	CONTAINER_WORKER *p;
	
	p = (CONTAINER_WORKER *) calloc(1, sizeof(CONTAINER_WORKER));
	if(!p)
	{
		return NULL;
	}
	container->api->addref(container);
	pthread_mutex_init(&(p->lock), NULL);
	p->refcount = 1;
	p->api = &worker_api_;
	memcpy(&(p->info), info, sizeof(CONTAINER_WORKER_INFO));
	p->host = host;
	p->container = container;
	container->cw->api->addref(container->cw);
	p->cw = container->cw;
	return p;
}

static unsigned long
worker_addref_(CONTAINER_WORKER *me)
{
	unsigned long r;
	
	pthread_mutex_lock(&(me->lock));
	me->refcount++;
	r = me->refcount;
	pthread_mutex_unlock(&(me->lock));
	return r;
}

static unsigned long
worker_release_(CONTAINER_WORKER *me)
{
	unsigned long r;
	
	pthread_mutex_lock(&(me->lock));
	me->refcount++;
	r = me->refcount;
	pthread_mutex_unlock(&(me->lock));
	if(!r)
	{
		me->cw->api->release(me->cw);
		me->container->api->release(me->container);
		free(me);
	}
	return r;
}

static int
worker_process_(CONTAINER_WORKER *me)
{
	CONTAINER_REQUEST *req;
	
	WDPRINTF(me->host, "thread started");
	for(;;)
	{
		req = NULL;
		if(me->host->api->request(me->host, &req) == -1)
		{
			WLPRINTF(me->host, CWLOG_CRIT, "container::request() failed: %s", strerror(errno));
			return -1;
		}
		if(!req)
		{
			break;
		}
		worker_process_request_(me, req);
	}
	WLPRINTF(me->host, CWLOG_INFO, "thread terminating");
	return 0;
}
	
static int
worker_process_request_(CONTAINER_WORKER *me, CONTAINER_REQUEST *req)
{
	const char *sub;
	
	sub = req->api->consume(req);
	if(!sub)
	{
		return worker_process_info(me, req);
	}
	if(!strcmp(sub, "global.css"))
	{
		return worker_process_global_css(me, req);
	}
	req->api->header(req, "Status", "404 Not found", 1);
	req->api->header(req, "Content-type", "text/html;charset=UTF-8", 1);
	req->api->puts(req, "<!DOCTYPE html>");
	req->api->puts(req, "<!DOCTYPE html>\n");
	req->api->puts(req, "<html>\n");
	req->api->puts(req, "<head>\n");
	req->api->puts(req, "<meta charset='UTF-8'>\n");
	req->api->puts(req, "<title>404 Not Found</title>\n");
	req->api->puts(req, "</head>\n");
	req->api->puts(req, "<body>\n");
	req->api->puts(req, "<h1>Not found</h1>\n");
	req->api->puts(req, "<p>The requested object could not be located on this server.\n");
	req->api->puts(req, "</body>");
	req->api->puts(req, "</html>");
	return 0;
}
