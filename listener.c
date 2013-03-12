#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_containerware.h"

static ENDPOINT_SERVER *dummy_server;
static struct listener_list_struct listeners;
static pthread_t listener_socket_thread;

static void *listener_socket_handler_(void *dummy);
static void *listener_thread_handler_(void *ptr);

int
listener_init(void)
{
	listener_list_init(&listeners, 16);
	dummy_server = dummy_endpoint_server();
	pthread_create(&listener_socket_thread, NULL, listener_socket_handler_, NULL);
	return 0;
}

ENDPOINT *
listener_add(const char *name, CONTAINER_HOST *host)
{
	ENDPOINT_SERVER *server;
	ENDPOINT *p;
	int r;

	p = NULL;
	server = NULL;
	
	if(!strcmp(name, "dummy"))
	{
		server = dummy_server;
	}
	if(!server)
	{
		errno = EINVAL;
		return NULL;
	}
	r = server->api->endpoint(server, &p);
	if(r)
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

int
listener_add_endpoint(ENDPOINT *endpoint, CONTAINER_HOST *host)
{
	struct listener_struct *p;
	int r;
	
	fprintf(stderr, "listener: Adding new endpoint\n");
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
			if(pthread_create(&(p->thread), NULL, listener_thread_handler_, (void *) p))
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
	fprintf(stderr, "listener: Endpoint added\n");
	return 0;
}

static void *
listener_socket_handler_(void *dummy)
{
	fd_set fds;
	struct timeval tv;

	(void) dummy;

	fprintf(stderr, "listener_socket_thread: thread started\n");
	for(;;)
	{
		fprintf(stderr, "listener_socket_thread: listening...\n");
		FD_ZERO(&fds);
		/* Iterate all of the listeners which use sockets and add their
		 * descriptors to the set
		 * listener_socket_set_all_();
		 */
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		select(FD_SETSIZE, &fds, NULL, NULL, &tv);
		/* Check select() return value */
		/* Iterate all of the listeners which use sockets and check if
		 * there is been activity in the set
		 * listener_socket_check_all_();
		 */
	}
	return NULL;
}

static void *
listener_thread_handler_(void *ptr)
{
	CONTAINER_REQUEST *req;
	struct listener_struct *me;
	int r;
	
	me = (struct listener_struct *) ptr;
	fprintf(stderr, "listener: starting up\n");
	pthread_mutex_lock(&(me->mutex));
	me->state = LS_RUNNING;
	for(;;)
	{
		r = me->endpoint->api->process(me->endpoint);
		if(r == -1)
		{
			fprintf(stderr, "listener: endpoint returned error status: %s\n", strerror(errno));
			break;
		}
		if(!r)
		{
			fprintf(stderr, "listener: ended normally\n");
			break;
		}
		fprintf(stderr, "listener: request is ready\n");
		r = me->endpoint->api->acquire(me->endpoint, &req);
		if(r)
		{
			fprintf(stderr, "listener: failed to obtain request: %s\n", strerror(errno));
			break;
		}
		fprintf(stderr, "listener: now have request\n");
		route_request(req, me);
		req->api->release(req);
		break;
	}
	me->status = LS_ZOMBIE;
	me->status = r;
	me->thread = 0;
	pthread_mutex_unlock(&(me->mutex));
	fprintf(stderr, "listener: thread is terminating\n");
	return me;
}
