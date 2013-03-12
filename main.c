#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_containerware.h"

int debug_console = 0;

int
main(int argc, char **argv)
{
	CONTAINER_HOST *host;
	
	(void) argc;
	(void) argv;
	
	listener_init();
	host_init();
	host = host_add("debug");
	listener_add("dummy", host);
	
	if(debug_console)
	{
		/* Run the debug console on the main thread */
	}
	else
	{
		fprintf(stderr, "main: sleeping\n");
		for(;;)
		{
			sleep(10);
		}
	}
	return 0;
}