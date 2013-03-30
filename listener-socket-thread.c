#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define LOG_FACILITY                   "listener-socket"

#include "p_containerware.h"

static pthread_t listener_socket_thread;

static void *listener_socket_handler_(void *dummy);

/* Invoked by listener_init() */
int
listener_socket_init(void)
{
	pthread_create(&listener_socket_thread, NULL, listener_socket_handler_, NULL);
}

/* Handler function for the main listener thread */
static void *
listener_socket_handler_(void *dummy)
{
	fd_set fds;
	struct timeval tv;

	(void) dummy;

	DPRINTF("listener thread has started");
	for(;;)
	{
		FD_ZERO(&fds);
		/* Iterate all of the listeners which use sockets and add their
		 * descriptors to the set
		 * listener_socket_set_all_();
		 */
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		select(FD_SETSIZE, &fds, NULL, NULL, &tv);
		/* Check select() return value */
		/* Iterate all of the listeners which use sockets and check if
		 * there is been activity in the set
		 * listener_socket_check_all_();
		 */
	}
	return NULL;
}