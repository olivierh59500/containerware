#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_info.h"

extern char **environ;

static unsigned long worker_addref_(CONTAINER_WORKER *me);
static unsigned long worker_release_(CONTAINER_WORKER *me);
static int worker_process_(CONTAINER_WORKER *me);
static int worker_process_request_(CONTAINER_WORKER *me, CONTAINER_REQUEST *req);

static int info_putstr_(CONTAINER_REQUEST *req, const char *header, const char *str);
static int info_putulong_(CONTAINER_REQUEST *req, const char *header, unsigned long l);
static int info_putvar_(CONTAINER_REQUEST *req, const char *header, const char *name);

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
	size_t c, n;
	const char *t;
	jd_var env = JD_INIT, keys = JD_INIT, *k, *v;
	
	(void) me;
	
	req->api->header(req, "Content-type", "text/html;charset=UTF-8", 1);
	req->api->puts(req, "<!DOCTYPE html>\n");
	req->api->puts(req, "<html>\n");
	req->api->puts(req, "<head>\n");
	req->api->puts(req, "<style type='text/css'>\n");
	req->api->puts(req, 
	"/*\n"
	"Copyright (c) 2010, Yahoo! Inc. All rights reserved.\n"
	"Code licensed under the BSD License:\n"
	"http://developer.yahoo.com/yui/license.html\n"
	"version: 3.2.0\n"
	"build: 2676\n"
	"*/\n"
	"html{color:#000;background:#FFF;}body,div,dl,dt,dd,ul,ol,li,h1,h2,h3,h4,h5,h6,pre,code,form,fieldset,legend,input,textarea,p,blockquote,th,td{margin:0;padding:0;}table{border-collapse:collapse;border-spacing:0;}fieldset,img{border:0;}address,caption,cite,code,dfn,em,strong,th,var{font-style:normal;font-weight:normal;}li{list-style:none;}caption,th{text-align:left;}h1,h2,h3,h4,h5,h6{font-size:100%;font-weight:normal;}q:before,q:after{content:'';}abbr,acronym{border:0;font-variant:normal;}sup{vertical-align:text-top;}sub{vertical-align:text-bottom;}input,textarea,select{font-family:inherit;font-size:inherit;font-weight:inherit;}input,textarea,select{*font-size:100%;}legend{color:#000;}\n"
	"body{font:13px/1.231 'Helvetica Neue',arial,helvetica,clean,sans-serif;*font-size:small;*font:x-small;}select,input,button,textarea{font:99% arial,helvetica,clean,sans-serif;}table{font-size:inherit;font:100%;}pre,code,kbd,samp,tt{font-family:monospace;*font-size:108%;line-height:100%;}\n"
	"\n"
	"em, i { font-style: italic; }\n"
	"strong, b { font-weight: bold; }\n"
	"a { text-decoration: none; color: #007; }\n"
	"a:hover, a:active { text-decoration: underline; }\n"
	"\n"
	"header, article, footer {\n"
	"	display: block;\n"
	"	width: 976px;\n"
	"	margin: 1em auto;\n"
	"}\n"
	"header {\n"
	"	margin: 0 auto 16px auto;\n"
	"	border-bottom: solid #ccc 1px;\n"
	"	padding: 8px 0;\n"
	"}\n"
	"header p, header ul, header ul>li {\n"
	"	display: inline;\n"
	"	color: #666;\n"
	"	margin: 0;\n"
	"}\n"
	"header li:after {\n"
	"	content: ', ';\n"
	"}\n"
	"header li:last-child:after {\n"
	"	content: '.';\n"
	"}\n"
	"footer {\n"
	"    margin: 16px auto 0 auto;\n"
	"    border-top: solid #ccc 1px;\n"
	"    font-family: 'Gill Sans', 'Arial';\n"
	"	padding-top: 8px;\n"
	"	color: #666;\n"
	"	min-height: 48px;\n"
	"}\n"
	"footer strong {\n"
	"	color: #000;\n"
	"}\n"
	"h1 {\n"
	"	font-family: 'Gill Sans';\n"
	"	font-size: 150%;\n"
	"}\n"
	"table, td, th {\n"
	"	font-size: 100%;\n"
	"}\n"
	"\n"
	"table {\n"
	"	border-collapse: collapse;\n"
	"	margin: 1em 0 1em 0;\n"
	"	border: solid #666 1px;\n"
	"	table-layout: fixed;\n"
	"	width: 100%;\n"
	"}\n"
	"th, td {\n"
	"	padding: 6px;\n"
	"	vertical-align: top;\n"
	"}\n"
	"thead, th {\n"
	"	background: #666;\n"
	"	color: #fff;\n"
	"	border: solid #666 1px;\n"
	"}\n"
	"thead th {\n"
	"	font-weight: normal;\n"
	"	padding: 6px 4px;\n"
	"}\n"
	"thead th.predicate {\n"
	"	width: 14em;\n"
	"}\n"
	"td {\n"
	"	vertical-align: top;\n"
	"	padding: 4px;\n"
	"	border: solid #666 1px;\n"
	"	word-wrap: break-word;\n"
	"}\n"
	"thead {\n"
	"	padding-right: 8px;\n"
	"}\n"
	"table > tbody > tr > th {\n"
	"	width: 12em;\n"
	"	word-wrap: break-word;\n"
	"}\n"
	"td p {\n"
	"	margin: 0 0 0.5em 0;\n"
	"}\n"
	"q {\n"
	"	color: #700;\n"
	"}\n"
	"p, ul, dl, dt {\n"
	"	margin: 1em 0;\n"
	"}\n"
	"dl>dd, ul>li {\n"
	"	display: list-item;\n"
	"	margin: 0.25em 0 0.25em 1.5em;\n"
	"	list-style: square outside;\n"
	"}\n"
	"article.photo img {\n"
	"	display: block;\n"
	"	max-width: 100%;\n"
	"	margin: 0 auto;\n"
	"}\n"
	"article h2 {\n"
	"	font-size: 150%;\n"
	"    font-family: 'Gill Sans', 'Arial';\n"
	"	margin: 12px 0;\n"
	"}\n"
	"article h3 {\n"
	"	font-size: 140%;\n"
	"    font-family: 'Gill Sans', 'Arial';\n"
	"	margin: 12px 0;\n"
	"	color: #666;\n"
	"}\n"
	"pre {\n"
	"	background: #eee;\n"
	"	border: solid #666 1px;\n"
	"	padding: 12px;\n"
	"}\n"
	"section.metadata {\n"
	"	width: 48%;\n"
	"	margin: 4px;\n"
	"	float: left;\n"
	"}\n"
	".clear {\n"
	"	display: block;\n"
	"	height: 1px;\n"
	"	clear: both;\n"
	"}\n"
	"form {\n"
	"	display: block;\n"
	"	padding: 12px;\n"
	"}\n"
	"form label {\n"
	"	display: block;\n"
	"	font-weight: bold;\n"
	"	margin: 0.25em 0;\n"
	"}\n"
	"section.form {\n"
	"	border-bottom: solid #ccc 1px;\n"
	"	margin-bottom: 12px;\n"
	"	padding-bottom: 18px;\n"
	"}\n");
	req->api->puts(req, "</style>\n");
	req->api->puts(req, "<meta charset='UTF-8'>\n");
	req->api->puts(req, "<meta name='robots' content='NOINDEX,NOFOLLOW,NOARCHIVE'>\n");
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
	WDPRINTF(me->host, "%lJ", &env);
/*	WDPRINTF(me->host, "%s", jd_bytes(jd_printf(jd_nv(), "%lJ", &env), NULL)); */
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

