/*
 * Copyright 2013 Mo McRoberts.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define LOG_FACILITY                   "module"
#define CONTAINERWARE_STRUCT_DEFINED_  1

#include "p_containerware.h"

struct containerware_struct
{
	CONTAINERWARE_COMMON;
	pthread_mutex_t lock;
	unsigned long refcount;
	void *dl;
	char *name;
};

static CONTAINERWARE *module_api_create_(void *dl, const char *name);
static unsigned long module_api_addref_(CONTAINERWARE *me);
static unsigned long module_api_release_(CONTAINERWARE *me);
static int module_api_register_endpoint_(CONTAINERWARE *me, const char *scheme, ENDPOINT_SERVER *server);
static int module_api_register_container_(CONTAINERWARE *me, const char *name, CONTAINER *container);
static int module_api_lputs_(CONTAINERWARE *me, int severity, const char *str);
static int module_api_lvprintf_(CONTAINERWARE *me, int severity, const char *fmt, va_list ap);
static int module_api_lprintf_(CONTAINERWARE *me, int severity, const char *fmt, ...);

static struct containerware_api_struct module_api_ = {
	NULL,
	module_api_addref_,
	module_api_release_,
	module_api_register_endpoint_,
	module_api_register_container_,
	module_api_lputs_,
	module_api_lvprintf_,
	module_api_lprintf_
};

int
module_load(const char *path)
{
	CONTAINERWARE *cw;
	void *h;
	int (*initfn)(CONTAINERWARE *me);
	
	DPRINTF("loading %s", path);
	h = dlopen(path, RTLD_NOW|RTLD_LOCAL);
	if(!h)
	{
		LPRINTF(CWLOG_ERR, "failed to load %s: %s", path, dlerror());
		return -1;
	}
	DPRINTF("loaded module %s as 0x%08x", path, (unsigned long) h);
	cw = module_api_create_(h, path);
	if(!cw)
	{
		LPRINTF(CWLOG_CRIT, "failed to create API instance to pass to module: %s", strerror(errno));
		dlclose(h);
		return -1;
	}
	initfn = dlsym(h, "containerware_module_init");
	if(!initfn)
	{
		LPRINTF(CWLOG_CRIT, "module %s cannot be initialised", path);
		return -1;
	}
	initfn(cw);
	cw->api->release(cw);
	return 0;
}


static CONTAINERWARE *
module_api_create_(void *dl, const char *name)
{
	CONTAINERWARE *p;
	const char *t;
	
	p = (CONTAINERWARE *) calloc(1, sizeof(CONTAINERWARE));
	if(!p)
	{
		return NULL;
	}
	t = strrchr(name, '/');
	if(t)
	{
		t++;
	}
	else
	{
		t = name;
	}
	p->name = strdup(t);
	if(!p->name)
	{
		free(p);
		return NULL;
	}
	pthread_mutex_init(&(p->lock), NULL);
	p->api = &module_api_;
	p->dl = dl;
	p->refcount = 1;
	return p;
}

static unsigned long
module_api_addref_(CONTAINERWARE *me)
{
	(void) me;
	
	return 2;
}

static unsigned long
module_api_release_(CONTAINERWARE *me)
{
	(void) me;
	
	return 1;
}

static int
module_api_register_endpoint_(CONTAINERWARE *me, const char *scheme, ENDPOINT_SERVER *server)
{
	(void) me;
	
	return server_register(scheme, server);
}

static int
module_api_register_container_(CONTAINERWARE *me, const char *name, CONTAINER *container)
{
	(void) me;
	
	return container_register(name, container);
}

static int
module_api_lputs_(CONTAINERWARE *me, int severity, const char *str)
{
	return log_puts(me->name, severity, str);
}

static int
module_api_lvprintf_(CONTAINERWARE *me, int severity, const char *fmt, va_list ap)
{
	return log_vprintf(me->name, severity, fmt, ap);
}

static int
module_api_lprintf_(CONTAINERWARE *me, int severity, const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	return log_vprintf(me->name, severity, fmt, ap);
}
