#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define LOG_FACILITY                   "server"

#include "p_containerware.h"

static struct server_list_struct servers;

int
server_init(void)
{
	server_list_init(&servers, 0);
}

/* Register a new listener
 *
 * Threading: thread-safe
 */
int
server_register(const char *scheme, ENDPOINT_SERVER *server)
{
	size_t c;
	
	struct server_list_entry_struct *p;
	
	DPRINTF("registering endpoint scheme '%s'", scheme);
	server->api->addref(server);
	server_list_wrlock(&servers);
	for(c = 0; c < servers.allocated; c++)
	{
		if(servers.list[c] && !strcmp(servers.list[c]->scheme, scheme))
		{
			LPRINTF(CWLOG_ERR, "failed to register server for scheme '%s': already registered", scheme);
			server_list_unlock(&servers);
			server->api->release(server);
			return -1;
		}
	}
	server_list_unlock(&servers);
	p = (struct server_list_entry_struct *) calloc(1, sizeof(struct server_list_entry_struct));
	if(!p)
	{
		server->api->release(server);
		return -1;
	}
	strncpy(p->scheme, scheme, sizeof(p->scheme) - 1);
	p->server = server;
	if(server_list_add(&servers, p) == -1)
	{
		free(p);
		server->api->release(server);
		return -1;
	}
	return 0;
}

/* Obtain an endpoint from the server for the given URI
 *
 * Threading: thread-safe
 */
int
server_endpoint_uri(URI *uri, ENDPOINT **ep)
{
	ENDPOINT_SERVER *server;
	int r;
	size_t c;
	char scheme[64];

	*ep = NULL;
	server = NULL;	
	if(uri_scheme(uri, scheme, sizeof(scheme)) == (size_t) -1)
	{	
		return -1;
	}
	server_list_rdlock(&servers);
	for(c = 0; c < servers.count; c++)
	{
		if(servers.list[c] && !strcmp(servers.list[c]->scheme, scheme))
		{
			server = servers.list[c]->server;
			break;
		}
	}
	if(!server)
	{
		server_list_unlock(&servers);
		DPRINTF("failed to locate an endpoint server for scheme '%s'", scheme);
		errno = EINVAL;
		return -1;
	}
	r = server->api->endpoint(server, uri, ep);
	server_list_unlock(&servers);
	return r;
}
