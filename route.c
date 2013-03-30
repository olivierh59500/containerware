#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define LOG_FACILITY                   "route"

#include "p_containerware.h"

/* Given a request, locate a suitable container to process it.
 * Note that this is invoked on a listener thread.
 */
int
route_request(CONTAINER_REQUEST *req, LISTENER *source)
{
	if(source->host)
	{
		DPRINTF("routing request directly to instance");
		return route_request_host(req, source, source->host);
	}
	DPRINTF("don't know how to route a request from this endpoint");
	return 0;
}

int
route_request_host(CONTAINER_REQUEST *req, LISTENER *source, CONTAINER_HOST *host)
{
	CONTAINER_WORKER_HOST *worker;
	
	worker = worker_locate(host);
	if(!worker)
	{
		LPRINTF(LOG_ERR, "failed to locate a worker to process this request");
		return -1;
	}
	return worker_queue_request(worker, req, source);
}