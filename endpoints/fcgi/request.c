#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_fcgi.h"

static unsigned long request_addref_(CONTAINER_REQUEST *me);
static unsigned long request_release_(CONTAINER_REQUEST *me);
static int request_timestamp_(CONTAINER_REQUEST *me, struct timeval *tv);
static const char *request_protocol_(CONTAINER_REQUEST *me);
static const char *request_method_(CONTAINER_REQUEST *me);
static const char *request_uri_str_(CONTAINER_REQUEST *me);
static const char *request_getenv_(CONTAINER_REQUEST *me, const char *var);
static int request_header_(CONTAINER_REQUEST *me, const char *name, const char *value, int replace);
static int request_write_(CONTAINER_REQUEST *me, const char *buf, size_t buflen);
static int request_puts_(CONTAINER_REQUEST *me, const char *str);
static int request_close_(CONTAINER_REQUEST *me);

static struct container_request_api_struct request_api_ =
{
	NULL,
	request_addref_,
	request_release_,
	request_timestamp_,
	request_protocol_,
	request_method_,
	request_uri_str_,
	request_getenv_,
	request_header_,
	request_write_,
	request_puts_,
	request_close_
};

CONTAINER_REQUEST *
request_create_(ENDPOINT *ep)
{
	CONTAINER_REQUEST *p;
	
	p = (CONTAINER_REQUEST *) calloc(1, sizeof(CONTAINER_REQUEST));
	if(!p)
	{
		return NULL;
	}
	pthread_mutex_init(&(p->lock), NULL);
	ep->api->addref(ep);
	p->endpoint = ep;
	p->refcount = 1;
	p->api = &request_api_;
	p->request = ep->request;
    p->in = ep->request->in;
	p->out = ep->request->out;
	p->err = ep->request->err;
	p->params = ep->request->envp;
	memcpy(&(p->reqtime), &(ep->reqtime), sizeof(struct timeval));
	ep->request = NULL;
	return p;
}

static unsigned long
request_addref_(CONTAINER_REQUEST *me)
{
	unsigned long r;
	
	pthread_mutex_lock(&(me->lock));
	me->refcount++;
	r = me->refcount;
	pthread_mutex_unlock(&(me->lock));
	return r;
}

static unsigned long
request_release_(CONTAINER_REQUEST *me)
{
	unsigned long r;

	pthread_mutex_lock(&(me->lock));
	me->refcount--;
	r = me->refcount;
	pthread_mutex_unlock(&(me->lock));
	if(!r)
	{
		fprintf(stderr, "fcgi: freeing request %08lx\n", (unsigned long) me);
		FCGX_Finish_r(me->request);
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

static const char *
request_protocol_(CONTAINER_REQUEST *me)
{
	return FCGX_GetParam("SERVER_PROTOCOL", me->params);
}

static const char *
request_method_(CONTAINER_REQUEST *me)
{
	return FCGX_GetParam("REQUEST_METHOD", me->params);
}

static const char *
request_uri_str_(CONTAINER_REQUEST *me)
{
	return FCGX_GetParam("REQUEST_URI", me->params);
}

static const char *
request_getenv_(CONTAINER_REQUEST *me, const char *name)
{
	return FCGX_GetParam(name, me->params);
}

static int
request_header_(CONTAINER_REQUEST *me, const char *name, const char *value, int replace)
{
	if(me->headers_sent)
	{
		/* Headers already sent */
		return -1;
	}
	FCGX_PutS(name, me->out);
	FCGX_PutChar(':', me->out);
	FCGX_PutChar(' ', me->out);
	FCGX_PutS(value, me->out);
	FCGX_PutChar('\n', me->out);
	return 0;
}

static int
request_write_(CONTAINER_REQUEST *me, const char *buf, size_t buflen)
{
	if(!me->headers_sent)
	{
		FCGX_PutChar('\n', me->out);
		me->headers_sent = 1;
	}
	return FCGX_PutStr(buf, buflen, me->out);
}

static int
request_puts_(CONTAINER_REQUEST *me, const char *str)
{
	return me->api->write(me, str, strlen(str));
}

static int
request_close_(CONTAINER_REQUEST *me)
{
	FCGX_FClose(me->in);
	FCGX_FClose(me->err);
	FCGX_FClose(me->out);
	return 0;
}
