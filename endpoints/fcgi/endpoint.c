#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_fcgi.h"

static unsigned long endpoint_addref_(ENDPOINT *me);
static unsigned long endpoint_release_(ENDPOINT *me);
static ENDPOINT_MODE endpoint_mode_(ENDPOINT *me);
static int endpoint_fd_(ENDPOINT *me);
static int endpoint_process_(ENDPOINT *me);
static int endpoint_acquire_(ENDPOINT *me, CONTAINER_REQUEST **req);

static struct endpoint_api_struct endpoint_api_ =
{
	NULL,
	endpoint_addref_,
	endpoint_release_,
	endpoint_mode_,
	endpoint_fd_,
	endpoint_process_,
	endpoint_acquire_
};

static int fcgi_sockpath_(URI *uri, char **ptr);
static int fcgi_hostport_(URI *uri, char **ptr);

ENDPOINT *
endpoint_create_(ENDPOINT_SERVER *server, URI *uri)
{
	ENDPOINT *ep;
	int fd;
	char *p;
	
	(void) uri;
	
	p = NULL;
	if(fcgi_sockpath_(uri, &p) == -1)
	{
		return NULL;
	}
	if(!p)
	{
		if(fcgi_hostport_(uri, &p) == -1)
		{
			return NULL;
		}
		if(!p)
		{
/*			LPRINTF(LOG_ERR, "failed to obtain either a socket path or host:port from URI"); */
			fprintf(stderr, "fcgi: failed to obtain either a socket path or host:port from URI\n");
			return NULL;
		}
	}
	ep = (ENDPOINT *) calloc(1, sizeof(ENDPOINT));
	if(!ep)
	{
		return NULL;
	}	
	fprintf(stderr, "fcgi: opening socket with specifier '%s'\n", p);
	fd = FCGX_OpenSocket(p, DEFAULT_FASTCGI_BACKLOG);
	free(p);
	if(fd == -1)
	{
		free(ep);
		return NULL;
	}	
	server->api->addref(server);
	ep->server = server;
	ep->refcount = 1;
	ep->api = &endpoint_api_;
	ep->fd = fd;
	pthread_mutex_init(&(ep->lock), NULL);
	return ep;
}

static unsigned long
endpoint_addref_(ENDPOINT *me)
{
	unsigned long r;
	
	pthread_mutex_lock(&(me->lock));
	me->refcount++;
	r = me->refcount;
	pthread_mutex_unlock(&(me->lock));
	return r;
}

static unsigned long
endpoint_release_(ENDPOINT *me)
{
	unsigned long r;

	pthread_mutex_lock(&(me->lock));
	me->refcount--;
	r = me->refcount;
	pthread_mutex_unlock(&(me->lock));
	if(!r)
	{
		me->server->api->release(me->server);
		pthread_mutex_destroy(&(me->lock));
		free(me);
	}
	return r;
}

ENDPOINT_MODE
endpoint_mode_(ENDPOINT *me)
{
	(void) me;
	
	/* FastCGI doesn't provide non-blocking APIs, so we must run the
	 * accept loop in a dedicated thread.
	 */
	return EM_THREAD;
}

int
endpoint_fd_(ENDPOINT *me)
{
	(void) me;

	/* As mode is EM_THREAD, we don't return the actual file descriptor */	
	return -1;
}

/* Endpoint thread handlers. Note that ContainerWare guarantees to
 * invoke ::acquire() on the same thread as ::process(), immediately
 * after it returns, and that there'll only ever be one thread per
 * endpoint. In other words, fcgi_process_() and fcgi_acquire_() have
 * exclusive access to the ENDPOINT and are executed sequentially.
 */
int
endpoint_process_(ENDPOINT *me)
{
	int r;

	me->request = (FCGX_Request *) calloc(1, sizeof(FCGX_Request));
	if(!me->request)
	{
		return -1;
	}
	FCGX_InitRequest(me->request, me->fd, 0);	
	r = FCGX_Accept_r(me->request);
	gettimeofday(&(me->reqtime), NULL);
	if(r == -1)
	{
		free(me->request);
		me->request = NULL;
		return -1;
	}
	return 1;
}

int
endpoint_acquire_(ENDPOINT *me, CONTAINER_REQUEST **req)
{
	*req = NULL;
	if(!me->request)
	{
		errno = EWOULDBLOCK;
		return -1;
	}
	*req = request_create_(me);
	if(!*req)
	{
		return -1;
	}
	fprintf(stderr, "fcgi: returning new request %08lx\n", (unsigned long) (*req));
	return 0;
}

/* Obtain a filesystem path from a URI; returns -1 if an error
 * occurs, or 0 if there wasn't a valid path found
 */
static int
fcgi_sockpath_(URI *uri, char **ptr)
{
	char *p, *t;
	size_t l;
	
	*ptr = NULL;
	l = uri_path(uri, NULL, 0);
	if(l == (size_t) -1)
	{
		return -1;
	}
	if(l < 2)
	{		
		return 0;
	}
	p = (char *) malloc(l);
	if(!p)
	{
		return -1;
	}
	if(uri_path(uri, p, l) != l)
	{
		free(p);
		return -1;
	}
	if(p[0] != '/')
	{
		free(p);
		return 0;	
	}
	for(t = p; *t; t++)
	{
		if(*t != '/')
		{
			break;
		}
	}
	if(!*t)
	{
		/* The path was obviously empty */
		free(p);
		return 0;
	}
	*ptr = p;
	return 0;
}

static int
fcgi_hostport_(URI *uri, char **ptr)
{
	char *p;
	size_t l, hl, pl;
	
	*ptr = NULL;
	hl = uri_host(uri, NULL, 0);
	pl = uri_port(uri, NULL, 0);
	if(hl == (size_t) -1 || pl == (size_t) - 1)
	{
		return -1;
	}
	if(pl < 2)
	{
		return 0;
	}
	l = hl + pl + 1;
	p = (char *) malloc(l);
	if(!p)
	{
		return -1;
	}
	if(uri_host(uri, p, hl) != hl)
	{
		free(p);
		return -1;
	}
	p[hl - 1] = ':';
	if(uri_port(uri, &(p[hl]), pl) != pl)
	{
		free(p);
		return -1;
	}
	*ptr = p;
	return 0;
}
