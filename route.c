#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_containerware.h"

/* Given a request, locate a suitable container to process it.
 * Note that this is invoked on a listener thread.
 */
int
route_request(CONTAINER_REQUEST *req, LISTENER *source)
{
	if(source->host)
	{
		fprintf(stderr, "route: will route request to host\n");
		return route_request_host(req, source, source->host);
	}
	fprintf(stderr, "route: nothing to do, bailing\n");
	return 0;
}

int
route_request_host(CONTAINER_REQUEST *req, LISTENER *source, CONTAINER_HOST *host)
{
	CONTAINER_INSTANCE_HOST *instance;
	
	instance = host_locate_instance(host);
	if(!instance)
	{
		fprintf(stderr, "route: failed to locate an instance for the host\n");
		return -1;
	}
	return host_queue_request(instance, req, source);
}