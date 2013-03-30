#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define LOG_FACILITY                   NULL

#include "p_containerware.h"

int debug_console = 0;

static int init_host_(const char *name, dictionary *dict);

int
main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	
	log_init(argc, argv);
	config_init();
	config_defaults();
	DPRINTF("ContainerWare is starting up");
	container_init();
	server_init();
	listener_init();
	host_init();
	worker_init();
	config_load();
	config_load_modules();
	config_foreach_container(init_host_);
	
/*	host = host_add("info");
	host_set_minchildren(host, 2);
	listener_add("fcgi://localhost:9995", host); */
	
	if(debug_console)
	{
		/* Run the debug console on the main thread */
	}
	else
	{
		DPRINTF("main thread is now idle");
		for(;;)
		{
			sleep(10);
		}
	}
	return 0;
}

static int
init_host_(const char *name, dictionary *dict)
{
	CONTAINER_HOST *host;
	char *ep;
	
	DPRINTF("will add a new host from section [%s]", name);
	iniparser_set(dict, "name", name);
	if(!iniparser_getstring(dict, "instance", NULL))
	{
		iniparser_set(dict, "instance", config_get("global:instance", NULL));
	}
	if(!iniparser_getstring(dict, "cluster", NULL))
	{
		iniparser_set(dict, "cluster", config_get("global:cluster", NULL));
	}
	host = host_add(dict);
	if(!host)
	{
		LPRINTF(CWLOG_CRIT, "failed to create host from section [%s]: %s", name, strerror(errno));
		return 0;
	}
	LPRINTF(CWLOG_INFO, "container '%s' has been created", name);
	ep = iniparser_getstring(dict, "endpoint", NULL);
	if(ep)
	{
		LPRINTF(CWLOG_INFO, "adding endpoint '%s' to container '%s'", ep, name);
		if(listener_add(ep, host) == NULL)
		{
			LPRINTF(CWLOG_CRIT, "failed to create endpoint '%s': %s", ep, strerror(errno));
		}
	}
	return 0;
}
