#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define LOG_FACILITY                   "listener"

#include "p_containerware.h"

static struct listener_list_struct listeners;

/* Initialise listeners
 *
 * Threading: called only by the main thread
 */
int
listener_init(void)
{
	listener_list_init(&listeners, 16);
	listener_socket_init();
	return 0;
}

/* Create a new endpoint based on the supplied URI string, optionally
 * attaching it to a constainer host.
 */
ENDPOINT *
listener_add(const char *str, CONTAINER_HOST *host)
{
	URI *uri;
	ENDPOINT *ep;
	
	uri = uri_create_str(str, NULL);
	if(!uri)
	{
		return NULL;
	}
	ep = listener_add_uri(uri, host);
	uri_destroy(uri);
	return ep;
}

/* Create a new endpoint based on the supplied URI, optionally
 * attaching it to a container host.
 */
ENDPOINT *
listener_add_uri(URI *uri, CONTAINER_HOST *host)
{
	ENDPOINT *p;
	int r;

	p = NULL;
	r = server_endpoint_uri(uri, &p);
	if(r || !p)
	{
		return NULL;
	}
	r = listener_add_endpoint(p, host);
	if(r)
	{		
		p->api->release(p);
		return NULL;
	}
	return p;
}

/* Create a new listener associated with the supplied endpoint,
 * optionally attaching it to the supplied container host.
 *
 * We will already retain endpoint prior to invoking this function;
 * an error return should result in it being released.
 */
int
listener_add_endpoint(ENDPOINT *endpoint, CONTAINER_HOST *host)
{
	struct listener_struct *p;
	int r;
	
	DPRINTF("adding a new endpoint");
	p = (struct listener_struct *) calloc(1, sizeof(struct listener_struct));
	if(!p)
	{
		return -1;
	}
	pthread_mutex_init(&(p->mutex), NULL);
	pthread_mutex_lock(&(p->mutex));
	p->endpoint = endpoint;
	p->host = host;
	p->mode = endpoint->api->mode(endpoint);
	if(p->mode == EM_SOCKET)
	{
		p->fd = endpoint->api->fd(endpoint);
	}
	else
	{
		p->fd = -1;
	}
	r = listener_list_add(&listeners, p);
	if(r)
	{
		pthread_mutex_unlock(&(p->mutex));
		pthread_mutex_destroy(&(p->mutex));
		free(p);
		return r;
	}
	switch(p->mode)
	{
		case EM_SOCKET:
			p->state = LS_SOCKET;
			break;
		case EM_THREAD:
			if(listener_thread_create(p) == -1)
			{
				listener_list_remove(&listeners, p);
				pthread_mutex_unlock(&(p->mutex));
				pthread_mutex_destroy(&(p->mutex));
				free(p);
				return -1;
			}
			break;
	}
	pthread_mutex_unlock(&(p->mutex));
	DPRINTF("endpoint added");
	return 0;
}
