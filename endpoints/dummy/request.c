#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_dummy.h"

static unsigned long request_addref_(CONTAINER_REQUEST *me);
static unsigned long request_release_(CONTAINER_REQUEST *me);
static int request_timestamp_(CONTAINER_REQUEST *me, struct timeval *tv);
static int request_status_(CONTAINER_REQUEST *me);
static size_t request_rbytes_(CONTAINER_REQUEST *me);
static size_t request_wbytes_(CONTAINER_REQUEST *me);
static const char *request_protocol_(CONTAINER_REQUEST *me);
static const char *request_method_(CONTAINER_REQUEST *me);
static const char *request_uri_str_(CONTAINER_REQUEST *me);
static const char *request_consume_(CONTAINER_REQUEST *me);
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
	request_status_,
	request_rbytes_,
	request_wbytes_,
	request_protocol_,
	request_method_,
	request_uri_str_,
	request_consume_,
	request_getenv_,
	NULL,
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
	ep->api->addref(ep);
	p->endpoint = ep;
	p->refcount = 1;
	p->api = &request_api_;
	pthread_mutex_init(&(p->lock), NULL);
	gettimeofday(&(p->reqtime), NULL);
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
		fprintf(stderr, "dummy: freeing request 0x%08lx\n", (unsigned long) me);
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
	(void) me;
	return 200;
}

static size_t
request_rbytes_(CONTAINER_REQUEST *me)
{
	(void) me;
	
	return (size_t) -1;
}

static size_t
request_wbytes_(CONTAINER_REQUEST *me)
{
	(void) me;
	
	return (size_t) -1;
}

static const char *
request_protocol_(CONTAINER_REQUEST *me)
{
	(void) me;
	
	return "HTTP/1.1";
}

static const char *
request_method_(CONTAINER_REQUEST *me)
{
	(void) me;
	
	return "GET";
}

static const char *
request_uri_str_(CONTAINER_REQUEST *me)
{
	(void) me;
	
	return "/server-status";
}

static const char *
request_consume_(CONTAINER_REQUEST *me)
{
	(void) me;
	
	return NULL;
}

static const char *
request_getenv_(CONTAINER_REQUEST *me, const char *var)
{
	if(!strcmp(var, "SERVER_PROTOCOL"))
	{
		return request_protocol_(me);
	}
	if(!strcmp(var, "REQUEST_METHOD"))
	{
		return request_method_(me);
	}
	if(!strcmp(var, "REQUEST_URI"))
	{
		return request_uri_str_(me);
	}
	return NULL;
}

static int
request_header_(CONTAINER_REQUEST *me, const char *name, const char *value, int replace)
{
	(void) me;
	
	fprintf(stderr, "dummy: Setting header '%s' to '%s' (replace=%d)\n", name, value, replace);
	return 0;
}

static int
request_write_(CONTAINER_REQUEST *me, const char *buf, size_t buflen)
{
	(void) me;
	
	fputs("dummy: ", stderr);
	fwrite(buf, buflen, 1, stderr);
	return buflen;
}

static int
request_puts_(CONTAINER_REQUEST *me, const char *str)
{
	return me->api->write(me, str, strlen(str));
}

static int
request_close_(CONTAINER_REQUEST *me)
{
	(void) me;
	
	fprintf(stderr, "dummy: closing request\n");
	return 0;
}
