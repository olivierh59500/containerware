#ifndef P_CONTAINERWARE_H_
# define P_CONTAINERWARE_H_            1

# include <stdio.h>
# include <stdlib.h>
# include <stdarg.h>
# include <time.h>
# include <inttypes.h>
# include <sys/types.h>
# include <sys/time.h>
# include <sys/select.h>
# include <unistd.h>
# include <errno.h>
# include <string.h>
# include <pthread.h>
# include <ctype.h>
# include <dlfcn.h>

# include "liburi.h"

# include "iniparser.h"

# define LISTENER_STRUCT_DEFINED_      1
# define CONTAINER_INSTANCE_HOST_STRUCT_DEFINED_ 1

# include "containerware.h"

/* Log levels */
# define LOG_DEBUG                     8
# define LOG_ACCESS                    7
# define LOG_INFO                      6
# define LOG_NOTICE                    5
# define LOG_WARN                      4
# define LOG_ERR                       3
# define LOG_CRIT                      2
# define LOG_ALERT                     1
# define LOG_EMERG                     0

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
DECLARE_PTRLIST(server_list, struct server_list_entry_struct);
DECLARE_PTRLIST(container_list, struct container_list_entry_struct);

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

/* Internal structure encapsulating a container */
struct container_host_struct
{
	CONTAINER_HOST_COMMON;
	CONTAINER *container;
	pthread_mutex_t lock;
	size_t minchildren;
	size_t maxchildren;
	size_t active;
	dictionary *config;
	struct instance_list_struct instances;
};

typedef enum
{
	IS_EMPTY,
	IS_IDLE,
	IS_RUNNING,
	IS_ZOMBIE
} CONTAINER_INSTANCE_STATE;

/* Internal structure encapsulating an individual instance */
struct container_instance_host_struct
{
	CONTAINER_INSTANCE_HOST_COMMON;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	size_t refcount;
	CONTAINER_INSTANCE *instance;
	CONTAINER_HOST *container_host;
	CONTAINER_INSTANCE_INFO info;
	pthread_t thread;
	pid_t child;
	size_t requestcount;
	size_t requestalloc;
	CONTAINER_REQUEST **requests;
	CONTAINER_REQUEST *current;
	CONTAINER_INSTANCE_STATE state;
	int status;
};

struct server_list_entry_struct
{
	char scheme[48];
	ENDPOINT_SERVER *server;
};

struct container_list_entry_struct
{
	char name[48];
	CONTAINER *container;
};

/* Configuration */
int config_init(void);
int config_load(void);
int config_load_modules(void);
int config_defaults(void);
int config_set_default(const char *key, const char *value);
int config_set(const char *key, const char *value);
const char *config_get(const char *key, const char *defval);
int config_foreach_container(int (*callback)(const char *name, dictionary *dict));
dictionary *config_copydict(dictionary *src);

/* Modules */
int module_load(const char *path);

/* Logging */

# define DFPRINTF(fac, ...) log_printf(fac, LOG_DEBUG, __VA_ARGS__)
# define DHPRINTF(host, ...) log_hprintf(host, LOG_DEBUG, __VA_ARGS__)
# define HLPRINTF(host, severity, ...) log_hprintf(host, severity, __VA_ARGS__)
# ifdef LOG_FACILITY
#  define DPRINTF(...) log_printf(LOG_FACILITY, LOG_DEBUG, __VA_ARGS__)
#  define LPRINTF(severity, ...) log_printf(LOG_FACILITY, severity, __VA_ARGS__)
# endif

int log_init(int argc, char **argv);
int log_puts(const char *facility, int severity, const char *str);
int log_vprintf(const char *facility, int severity, const char *fmt, va_list ap);
int log_printf(const char *facility, int severity, const char *fmt, ...);

int log_hputs(CONTAINER_INSTANCE_HOST *host, int severity, const char *str);
int log_hvprintf(CONTAINER_INSTANCE_HOST *host, int severity, const char *fmt, va_list ap);
int log_hprintf(CONTAINER_INSTANCE_HOST *host, int severity, const char *fmt, ...);

/* Listeners  */
int listener_init(void);
ENDPOINT *listener_add(const char *name, CONTAINER_HOST *host);
ENDPOINT *listener_add_uri(URI *uri, CONTAINER_HOST *host);
int listener_add_endpoint(ENDPOINT *endpoint, CONTAINER_HOST *host);

/* Listener socket thread */
int listener_socket_init(void);

/* Listener endpoint threads */
int listener_thread_create(LISTENER *p);

/* Servers */
int server_init(void);
int server_register(const char *scheme, ENDPOINT_SERVER *server);
int server_endpoint_uri(URI *uri, ENDPOINT **ep);

/* Routing */
int route_request(CONTAINER_REQUEST *req, LISTENER *source);
int route_request_host(CONTAINER_REQUEST *req, LISTENER *source, CONTAINER_HOST *host);

/* Containers */
int container_init(void);
int container_register(const char *name, CONTAINER *container);
int container_locate(const char *name, CONTAINER **out);

/* Container hosts */
int host_init(void);
int host_start(void);
CONTAINER_HOST *host_add(dictionary *config);
CONTAINER_HOST *host_add_container(CONTAINER *container, dictionary *config);
int host_set_minchildren(CONTAINER_HOST *host, size_t minchildren);

/* Instances */
CONTAINER_INSTANCE_HOST *instance_create(CONTAINER_HOST *host);
CONTAINER_INSTANCE_HOST *instance_locate(CONTAINER_HOST *host);
int instance_queue_request(CONTAINER_INSTANCE_HOST *instance, CONTAINER_REQUEST *req, LISTENER *source);

/* Instance threads */
int instance_thread_create(CONTAINER_INSTANCE_HOST *host);

#endif /*!P_CONTAINERWARE_H_*/