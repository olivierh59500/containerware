#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_info.h"

extern char **environ;

static int info_putstr_(CONTAINER_REQUEST *req, const char *header, const char *str);
static int info_putulong_(CONTAINER_REQUEST *req, const char *header, unsigned long l);
static int info_putvar_(CONTAINER_REQUEST *req, const char *header, const char *name);

int
worker_process_info(CONTAINER_WORKER *me, CONTAINER_REQUEST *req)
{
	size_t c, n;
	const char *t;
	jd_var env = JD_INIT, keys = JD_INIT, *k, *v;
	
	(void) me;
	
	req->api->header(req, "Content-type", "text/html;charset=UTF-8", 1);
	req->api->puts(req, "<!DOCTYPE html>\n");
	req->api->puts(req, "<html>\n");
	req->api->puts(req, "<head>\n");
	req->api->puts(req, "<meta charset='UTF-8'>\n");
	req->api->puts(req, "<meta name='robots' content='NOINDEX,NOFOLLOW,NOARCHIVE'>\n");
	req->api->puts(req, "<link rel='stylesheet' href='/global.css' type='text/css'>\n");
	req->api->puts(req, "<title>ContainerWare: Request Information</title>\n");
	req->api->puts(req, "</head>\n");
	req->api->puts(req, "<body>\n");
	req->api->puts(req, "<header>\n");
	req->api->puts(req, "<h1>ContainerWare</h1>\n");
	req->api->puts(req, "</header>\n");
	req->api->puts(req, "<article>\n");

	req->api->puts(req, "<section id='container'>\n");
	req->api->puts(req, "<h2>Container</h2>\n");
	req->api->puts(req, "<table>\n");
	req->api->puts(req, "<tbody>\n");
	info_putstr_(req, "Application", me->info.app);
	info_putstr_(req, "Instance", me->info.instance);
	info_putstr_(req, "Cluster", me->info.cluster);
	info_putulong_(req, "Process ID", me->info.pid);
	info_putulong_(req, "Worker ID", me->info.workerid);
	req->api->puts(req, "</tbody>\n");
	req->api->puts(req, "</table>\n");	
	req->api->puts(req, "</section>\n");
	
	req->api->puts(req, "<section id='request'>\n");
	req->api->puts(req, "<h2>Request</h2>\n");
	req->api->puts(req, "<table>\n");	
	req->api->puts(req, "<tbody>\n");
	info_putstr_(req, "Protocol", req->api->protocol(req));
	info_putstr_(req, "Method", req->api->method(req));
	info_putstr_(req, "URI", req->api->request_uri_str(req));
	info_putvar_(req, "HTTP host", "HTTP_HOST");
	info_putvar_(req, "Connection type", "HTTP_CONNECTION");
	info_putvar_(req, "Gateway interface", "GATEWAY_INTERFACE");
	info_putvar_(req, "User agent", "HTTP_USER_AGENT");
	info_putvar_(req, "Accepted types", "HTTP_ACCEPT");
	info_putvar_(req, "Accepted encodings", "HTTP_ACCEPT_ENCODING");
	info_putvar_(req, "Accepted languages", "HTTP_ACCEPT_LANGUAGE");
	info_putvar_(req, "Accepted character sets", "HTTP_ACCEPT_CHARSET");
	info_putvar_(req, "Query string", "QUERY_STRING");
	info_putvar_(req, "Remote address", "REMOTE_ADDR");
	info_putvar_(req, "Remote port", "REMOTE_PORT");	
	req->api->puts(req, "</tbody>\n");
	req->api->puts(req, "</table>\n");	
	req->api->puts(req, "</section>\n");

	req->api->puts(req, "<section id='server'>\n");
	req->api->puts(req, "<h2>Request source</h2>\n");
	req->api->puts(req, "<table>\n");	
	req->api->puts(req, "<tbody>\n");
	info_putvar_(req, "Software", "SERVER_SOFTWARE");
	info_putvar_(req, "Administrator", "SERVER_ADMIN");
	info_putvar_(req, "Signature", "SERVER_SIGNATURE");
	info_putvar_(req, "Public root", "DOCUMENT_ROOT");
	info_putvar_(req, "Hostname", "SERVER_NAME");
	info_putvar_(req, "Address", "SERVER_ADDR");
	info_putvar_(req, "Port", "SERVER_PORT");
	req->api->puts(req, "</tbody>\n");
	req->api->puts(req, "</table>\n");	
	req->api->puts(req, "</section>\n");

	req->api->environment(req, &env);
	jd_keys(&keys, &env);
	c = jd_count(&keys);
	req->api->puts(req, "<section id='env'>\n");
	req->api->puts(req, "<h2>Request Environment</h2>\n");
	req->api->puts(req, "<table>\n");
	req->api->puts(req, "<tbody>\n");
	for(n = 0; n < c; n++)
	{
		k = jd_get_idx(&keys, n);
		v = jd_get_key(&env, k, 0);
		if(v)
		{
			info_putstr_(req, jd_bytes(k, NULL), jd_bytes(v, NULL));
		}
	}
	req->api->puts(req, "</tbody>\n");
	req->api->puts(req, "</table>\n");	
	req->api->puts(req, "</section>\n");
	jd_release(&keys);
	jd_release(&env);

	req->api->puts(req, "<section id='cw-env'>\n");
	req->api->puts(req, "<h2>Environment</h2>\n");
	req->api->puts(req, "<table>\n");	
	req->api->puts(req, "<tbody>\n");
	for(c = 0; environ[c]; c++)
	{
		req->api->puts(req, "<tr>");
		t = strchr(environ[c], '=');
		if(t)
		{			
			req->api->puts(req, "<th scope='row'>");
			req->api->write(req, environ[c], t - environ[c]);
			req->api->puts(req, "</th>");
			req->api->puts(req, "<td>");
			req->api->puts(req, t + 1);
			req->api->puts(req, "</td>");
		}
		else
		{
			req->api->puts(req, "<th scope='row'>");
			req->api->puts(req, environ[c]);
			req->api->puts(req, "</th>");
			req->api->puts(req, "<td>(No value)</td>");
		}		
		req->api->puts(req, "</tr>\n");
		
	}
	req->api->puts(req, "</tbody>\n");
	req->api->puts(req, "</table>\n");	
	req->api->puts(req, "</section>");

	req->api->puts(req, "</article>\n");
	req->api->puts(req, "</body>\n");
	req->api->puts(req, "</html>\n");
	req->api->close(req);
	return 0;
}

static int
info_putstr_(CONTAINER_REQUEST *req, const char *header, const char *str)
{
	if(!str || !*str)
	{
		return 0;
	}
	req->api->puts(req, "<tr>");
	req->api->puts(req, "<th scope='row'>");
	req->api->puts(req, header);
	req->api->puts(req, "</th>");
	req->api->puts(req, "<td>");
	req->api->puts(req, str);
	req->api->puts(req, "</tr>\n");
	return 0;
}

static int
info_putulong_(CONTAINER_REQUEST *req, const char *header, unsigned long l)
{
	char buf[64];
	
	snprintf(buf, 63, "%lu", l);
	return info_putstr_(req, header, buf);
}

static int
info_putvar_(CONTAINER_REQUEST *req, const char *header, const char *name)
{
	const char *value;
	
	value = req->api->getenv(req, name);
	return info_putstr_(req, header, value);
}

