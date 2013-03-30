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
	CONTAINER_INSTANCE_HOST *instance;
	
	instance = instance_locate(host);
	if(!instance)
	{
		LPRINTF(LOG_ERR, "failed to locate an instance to process this request");
		return -1;
	}
	return instance_queue_request(instance, req, source);
}