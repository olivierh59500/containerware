#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_fcgi.h"

static unsigned long request_addref_(CONTAINER_REQUEST *me);
static unsigned long request_release_(CONTAINER_REQUEST *me);
static int request_timestamp_(CONTAINER_REQUEST *me, struct timeval *tv);
static int request_status_(CONTAINER_REQUEST *me);
static const char *request_uri_str_(CONTAINER_REQUEST *me);
static const char *request_getenv_(CONTAINER_REQUEST *me, const char *var);
static const char *request_consume_(CONTAINER_REQUEST *me);
static int request_environment_(CONTAINER_REQUEST *me, jd_var *out);
static int request_header_(CONTAINER_REQUEST *me, const char *name, const char *value, int replace);
static int request_write_(CONTAINER_REQUEST *me, const char *buf, size_t buflen);
static int request_write_header_(void *data, const char *name, const char *value, int more);
static int request_close_(CONTAINER_REQUEST *me);

static struct container_request_api_struct request_api_ =
{
	NULL,
	request_addref_,
	request_release_,
	request_timestamp_,
	request_status_,
	cw_request_protocol,
	cw_request_method,
	request_uri_str_,
	request_consume_,
	request_getenv_,
	request_environment_,
	request_header_,
	request_write_,
	cw_request_puts,
	request_close_
};

CONTAINER_REQUEST *
request_create_(ENDPOINT *ep)
{
	CONTAINER_REQUEST *p;
	size_t n;
	const char *t;
	
	p = (CONTAINER_REQUEST *) calloc(1, sizeof(CONTAINER_REQUEST));
	if(!p)
	{
		return NULL;
	}
	pthread_mutex_init(&(p->lock), NULL);
	ep->cw->api->addref(ep->cw);
	p->cw = ep->cw;
	ep->api->addref(ep);
	p->endpoint = ep;
	p->refcount = 1;
	p->api = &request_api_;
	p->request = ep->request;
    p->in = ep->request->in;
	p->out = ep->request->out;
	p->err = ep->request->err;
	memcpy(&(p->reqtime), &(ep->reqtime), sizeof(struct timeval));
	cw_request_env_init(ep->request->envp, &(p->env));
	cw_request_headers_init(&(p->headers));
	cw_request_info_init(&(p->info), &(p->env));
	ep->request = NULL;
	return p;
}

static unsigned long
request_addref_(CONTAINER_REQUEST *me)
{
	return cw_addref(&(me->lock), &(me->refcount));
}

static unsigned long
request_release_(CONTAINER_REQUEST *me)
{
	unsigned long r;

	r = cw_release(&(me->lock), &(me->refcount));
	if(!r)
	{
		DPRINTF(me->cw, "freeing request %08lx", (unsigned long) me);
		FCGX_Finish_r(me->request);
		cw_request_info_destroy(&(me->info));
		me->cw->api->release(me->cw);
		me->endpoint->api->release(me->endpoint);
		pthread_mutex_destroy(&(me->lock));
		free(me);
	}
	return r;
}

static int
request_timestamp_(CONTAINER_REQUEST *me, struct timeval *tv)
{
	memcpy(tv, &(me->reqtime), sizeof(struct timeval));
	return 0;
}

static int 
request_status_(CONTAINER_REQUEST *me)
{
	return cw_request_headers_status(&(me->headers));
}

static const char *
request_uri_str_(CONTAINER_REQUEST *me)
{
	return request_getenv_(me, "REQUEST_URI");
}

static const char *
request_consume_(CONTAINER_REQUEST *me)
{
	return cw_request_info_consume(&(me->info));
}

static const char *
request_getenv_(CONTAINER_REQUEST *me, const char *name)
{
	return cw_request_getenv(&(me->env), name);
}

static int
request_environment_(CONTAINER_REQUEST *me, jd_var *out)
{
	return cw_request_environment(&(me->env), out);
}

static int
request_header_(CONTAINER_REQUEST *me, const char *name, const char *value, int replace)
{
	if(me->headers_sent)
	{
		/* Headers already sent */
		errno = EINVAL;
		return -1;
	}
	cw_request_headers_set(&(me->headers), name, value, replace);
	return 0;
}

static int
request_write_(CONTAINER_REQUEST *me, const char *buf, size_t buflen)
{
	if(!me->headers_sent)
	{
		cw_request_headers_send(&(me->headers), request_write_header_, me);
		FCGX_PutChar('\n', me->out);
		me->headers_sent = 1;
	}
	return FCGX_PutStr(buf, buflen, me->out);
}

static int
request_write_header_(void *data, const char *name, const char *value, int more)
{
	CONTAINER_REQUEST *me = (CONTAINER_REQUEST *) data;
	
	FCGX_PutS(name, me->out);
	FCGX_PutChar(':', me->out);
	FCGX_PutChar(' ', me->out);
	FCGX_PutS(value, me->out);
	FCGX_PutChar('\n', me->out);
	return 0;
}

static int
request_close_(CONTAINER_REQUEST *me)
{
	FCGX_FClose(me->in);
	FCGX_FClose(me->err);
	FCGX_FClose(me->out);
	return 0;
}
