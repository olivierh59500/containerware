#ifndef P_CONTAINERWARE_H_
# define P_CONTAINERWARE_H_            1

# include <stdio.h>
# include <stdlib.h>
# include <time.h>
# include <sys/time.h>
# include <sys/select.h>
# include <unistd.h>
# include <errno.h>
# include <string.h>
# include <pthread.h>

# define LISTENER_STRUCT_DEFINED_      1
# define CONTAINER_INSTANCE_HOST_STRUCT_DEFINED_ 1

# include "containerware.h"

typedef struct listener_struct LISTENER;
typedef struct container_host_struct CONTAINER_HOST;

# define DECLARE_PTRLIST(name, target) \
	struct name##_struct \
	{ \
		target **list; \
		pthread_rwlock_t lock; \
		size_t count; \
		size_t blocksize; \
		size_t allocated; \
	}; \
	int name##_init(struct name##_struct *list, size_t blocksize); \
	int name##_destroy(struct name##_struct *list); \
	int name##_rdlock(struct name##_struct *list); \
	int name##_tryrdlock(struct name##_struct *list); \
	int name##_wrlock(struct name##_struct *list); \
	int name##_trywrlock(struct name##_struct *list); \
	int name##_unlock(struct name##_struct *list); \
	int name##_add(struct name##_struct *list, target *item); \
	int name##_add_unlocked(struct name##_struct *list, target *item); \
	int name##_remove(struct name##_struct *list, target *item); \
	int name##_remove_unlocked(struct name##_struct *list, target *item); \
	int name##_iterate(struct name##_struct *list, int (*callback)(target *item, void *data), void *data);

DECLARE_PTRLIST(listener_list, LISTENER);
DECLARE_PTRLIST(host_list, CONTAINER_HOST);
DECLARE_PTRLIST(instance_list, CONTAINER_INSTANCE_HOST);

/* A listener wraps a plugin-provided endpoint */
typedef enum
{
	LS_EMPTY,
	LS_SOCKET,
	LS_RUNNING,
	LS_ZOMBIE
} LISTENER_STATE;

struct listener_struct
{
	void *reserved;
	LISTENER_STATE state;
	ENDPOINT *endpoint;
	ENDPOINT_MODE mode;
	CONTAINER_HOST *host;
	int fd;
	pthread_mutex_t mutex;
	pthread_t thread;
	int status;
};

# define CONTAINER_HOST_COMMON \
	struct container_host_api_struct *api;

struct container_host_api_struct
{
	void *reserved1;
	void *reserved2;
	void *reserved3;
};

struct container_host_struct
{
	CONTAINER_HOST_COMMON;
	CONTAINER *container;
	size_t minchildren;
	size_t maxchildren;
	size_t active;
	struct instance_list_struct instances;
};

typedef enum
{
	IS_EMPTY,
	IS_IDLE,
	IS_RUNNING,
	IS_ZOMBIE
} CONTAINER_INSTANCE_STATE;

struct container_instance_host_struct
{
	CONTAINER_INSTANCE_HOST_COMMON;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	size_t refcount;
	CONTAINER_INSTANCE *instance;
	CONTAINER_HOST *container_host;
	pthread_t thread;
	pid_t child;
	size_t requestcount;
	size_t requestalloc;
	CONTAINER_REQUEST **requests;
	CONTAINER_REQUEST *current;
	CONTAINER_INSTANCE_STATE state;
	int status;
};

/* Listener API */
int listener_init(void);
ENDPOINT *listener_add(const char *name, CONTAINER_HOST *host);
int listener_add_endpoint(ENDPOINT *endpoint, CONTAINER_HOST *host);

/* Routing API */
int route_request(CONTAINER_REQUEST *req, LISTENER *source);
int route_request_host(CONTAINER_REQUEST *req, LISTENER *source, CONTAINER_HOST *host);

/* Container host API */
int host_init(void);
int host_start(void);
CONTAINER_HOST *host_add(const char *name);
CONTAINER_HOST *host_add_container(CONTAINER *container);
CONTAINER_INSTANCE_HOST *host_locate_instance(CONTAINER_HOST *host);
CONTAINER_INSTANCE_HOST *host_create_instance(CONTAINER_HOST *host);
int host_queue_request(CONTAINER_INSTANCE_HOST *instance, CONTAINER_REQUEST *req, LISTENER *source);
int host_set_minchildren(CONTAINER_HOST *host, size_t minchildren);

/* Built-in endpoint servers */
ENDPOINT_SERVER *dummy_endpoint_server(void);

/* Built-in containers */
CONTAINER *debug_container(void);

#endif /*!P_CONTAINERWARE_H_*/