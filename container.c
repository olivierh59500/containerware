#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define LOG_FACILITY                   "container"

#include "p_containerware.h"

static struct container_list_struct containers;

int
container_init(void)
{
	container_list_init(&containers, 0);
}

/* Register a new container type. The container instance will
 * be retained for the duration of it being included in the
 * list.
 *
 * Threading: thread-safe
 */
int
container_register(const char *name, CONTAINER *container)
{
	size_t c;
	
	struct container_list_entry_struct *p;
	
	container->api->addref(container);
	container_list_wrlock(&containers);
	for(c = 0; c < containers.allocated; c++)
	{
		if(containers.list[c] && !strcmp(containers.list[c]->name, name))
		{
			LPRINTF(LOG_ERR, "failed to register container '%s': already registered", name);
			container_list_unlock(&containers);
			container->api->release(container);
			return -1;
		}
	}
	container_list_unlock(&containers);
	p = (struct container_list_entry_struct *) calloc(1, sizeof(struct container_list_entry_struct));
	if(!p)
	{
		container->api->release(container);
		return -1;
	}
	strncpy(p->name, name, sizeof(p->name) - 1);
	p->container = container;
	if(container_list_add(&containers, p) == -1)
	{
		LPRINTF(LOG_CRIT, "failed to register container '%s': %s", name, strerror(errno));
		free(p);
		container->api->release(container);
		return -1;
	}
	DPRINTF("registered container '%s'", name);
	return 0;
}

/* Obtain a named container type
 *
 * Threading: thread-safe
 */
int
container_locate(const char *name, CONTAINER **out)
{
	CONTAINER *container;
	int r;
	size_t c;

	*out = NULL;
	container = NULL;	
	container_list_rdlock(&containers);
	for(c = 0; c < containers.count; c++)
	{
		if(containers.list[c] && !strcmp(containers.list[c]->name, name))
		{
			container = containers.list[c]->container;
			break;
		}
	}
	if(!container)
	{
		container_list_unlock(&containers);
		DPRINTF("failed to locate container '%s'", name);
		errno = EINVAL;
		return -1;
	}
	container->api->addref(container);
	*out = container;
	container_list_unlock(&containers);
	return 0;
}
