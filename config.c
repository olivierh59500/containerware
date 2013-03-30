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

#define LOG_FACILITY                   "config"

#include "p_containerware.h"

static dictionary *defaults;
static dictionary *overrides;
static dictionary *config;

int
config_init(void)
{
	defaults = dictionary_new(0);
	if(!defaults)
	{
		return -1;
	}
	overrides = dictionary_new(0);
	if(!overrides)
	{
		return -1;
	}
	return 0;
}

int
config_defaults(void)
{
	char hostname[128];
	char *t;
	
	hostname[0] = 0;
	gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = 0;
	t = strchr(hostname, '.');
	if(t)
	{
		*t = 0;
	}
	if(!strlen(hostname))
	{
		strcpy(hostname, "localhost");
	}
	config_set_default("global:instance", hostname);
	config_set_default("global:cluster", "private");
	return 0;
}

int
config_load(void)
{
	int n;

	config = iniparser_load("containerware.conf");
	if(!config)
	{
		return -1;
	}
	for(n = 0; n < overrides->n; n++)
	{
		iniparser_set(config, overrides->key[n], overrides->val[n]);
	}
	for(n = 0; n < defaults->n; n++)
	{
		if(!iniparser_getstring(config, defaults->key[n], NULL))
		{
			iniparser_set(config, defaults->key[n], defaults->val[n]);
		}
	}
	dictionary_del(overrides);
	overrides = NULL;
	dictionary_del(defaults);
	defaults = NULL;
	DPRINTF("configuration loaded");
	for(n = 0; n < config->n; n++)
	{
		DPRINTF("[%s]='%s'", config->key[n], config->val[n]);
	}
	return 0;
}

int
config_foreach_container(int (*callback)(const char *name, dictionary *dict))
{
	int n, c;
	size_t l;
	dictionary *dict;
	char *t;
	
	if(!config)
	{
		return -1;
	}
	for(n = 0; n < config->n; n++)
	{
		if(config->val[n])
		{
			continue;
		}
		/* Found a section */
		if(strncmp(config->key[n], "container", 9))
		{
			continue;
		}
		for(t = &(config->key[n][9]); *t; t++)
		{
			if(!isspace(*t))
			{
				break;
			}
		}
		if(*t != ':')
		{
			continue;
		}
		for(t++; *t; t++)
		{
			if(!isspace(*t))
			{
				break;
			}
		}
		if(!*t)
		{
			continue;
		}
		l = strlen(config->key[n]);
		dict = dictionary_new(0);
		for(c = n + 1; c < config->n; c++)
		{
			if(strncmp(config->key[c], config->key[n], l))
			{
				continue;
			}
			if(config->key[c][l] != ':')
			{
				continue;
			}
			iniparser_set(dict, &(config->key[c][l + 1]), config->val[c]);
		}
		callback(t, dict);
		dictionary_del(dict);
	}
	return 0;
}

int
config_set(const char *key, const char *value)
{
	iniparser_set((overrides ? overrides : config), key, value);
	return 0;
}

int
config_set_default(const char *key, const char *value)
{
	if(defaults)
	{
		iniparser_set(defaults, key, value);
		return 0;
	}
	if(!iniparser_getstring(config, key, NULL))
	{
		iniparser_set(config, key, value);
	}
	return 0;
}

const char *
config_get(const char *key, const char *defval)
{
	if(config)
	{
		return iniparser_getstring(config, key, (char *) defval);
	}
	return iniparser_getstring(config, key, iniparser_getstring(defaults, key, (char *) defval));
}

dictionary *
config_copydict(dictionary *src)
{
	dictionary *dest;
	int n;
	
	dest = dictionary_new(src->n);
	if(!dest)
	{
		return NULL;
	}
	for(n = 0; n < src->n; n++)
	{
		iniparser_set(dest, src->key[n], src->val[n]);
	}
	return dest;
	
}

int
config_load_modules(void)
{
	int n;
	
	if(!config)
	{
		errno = EINVAL;
		return -1;
	}
	for(n = 0; n < config->n; n++)
	{
		if(!strcmp(config->key[n], "modules:module") && config->val[n])
		{
			module_load(config->val[n]);
		}
	}
	return 0;
}
