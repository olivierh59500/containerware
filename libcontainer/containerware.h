#ifndef CONTAINERWARE_H_
# define CONTAINERWARE_H_              1

# include <stdarg.h>
# include <inttypes.h>
# include <sys/types.h>
# include <sys/time.h>

# include "liburi.h"
# include "jsondata.h"

# if (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L) && !defined(restrict)
#  define restrict
# endif

# if defined(__cplusplus)
extern "C" {
# endif
	
/* Log levels */
# define CWLOG_DEBUG                   8
# define CWLOG_ACCESS                  7
# define CWLOG_INFO                    6
# define CWLOG_NOTICE                  5
# define CWLOG_WARN                    4
# define CWLOG_ERR                     3
# define CWLOG_CRIT                    2
# define CWLOG_ALERT                   1
# define CWLOG_EMERG                   0

# ifdef CONTAINER_PLUGIN_INTERNAL
#  define CONTAINER_STRUCT_DEFINED_    1
#  define CONTAINER_WORKER_STRUCT_DEFINED_ 1
# endif

# ifdef ENDPOINT_PLUGIN_INTERNAL
#  define ENDPOINT_SERVER_STRUCT_DEFINED_ 1
#  define ENDPOINT_STRUCT_DEFINED_     1
#  define CONTAINER_REQUEST_STRUCT_DEFINED_ 1
# endif
	
/* CONTAINERWARE: API exported by ContainerWare itself */
typedef struct containerware_struct CONTAINERWARE;

# define CONTAINERWARE_COMMON \
	struct containerware_api_struct *api;

# ifndef CONTAINERWARE_STRUCT_DEFINED_
struct containerware_struct { CONTAINERWARE_COMMON };
# endif

/* CONTAINER: Something which creates workers to process requests */
typedef struct container_struct CONTAINER;

# define CONTAINER_COMMON \
	struct container_api_struct *api;

# ifndef CONTAINER_STRUCT_DEFINED_
struct container_struct { CONTAINER_COMMON };
# endif

/* CONTAINER_WORKER: An individual worker thread for a container */
typedef struct container_worker_struct CONTAINER_WORKER;
typedef struct container_worker_info_struct CONTAINER_WORKER_INFO;

# define CONTAINER_WORKER_COMMON \
	struct container_worker_api_struct *api;

# ifndef CONTAINER_WORKER_STRUCT_DEFINED_
struct container_worker_struct { CONTAINER_WORKER_COMMON };
# endif

/* ENDPOINT: A single "listener", such as as TCP/IP socket */
typedef struct endpoint_struct ENDPOINT;

# define ENDPOINT_COMMON \
	struct endpoint_api_struct *api;

# ifndef ENDPOINT_STRUCT_DEFINED_
struct endpoint_struct { ENDPOINT_COMMON };
# endif

/* ENDPOINT_SERVER: Something which creates endpoints */
typedef struct endpoint_server_struct ENDPOINT_SERVER;

# define ENDPOINT_SERVER_COMMON \
	struct endpoint_server_api_struct *api;

# ifndef ENDPOINT_SERVER_STRUCT_DEFINED_
struct endpoint_server_struct { ENDPOINT_SERVER_COMMON };
# endif

/* CONTAINER_REQUEST: A request acquired from an endpoint to be processed
 * by a worker.
 */
typedef struct container_request_struct CONTAINER_REQUEST;

# define CONTAINER_REQUEST_COMMON \
	struct container_request_api_struct *api;

# ifndef CONTAINER_REQUEST_STRUCT_DEFINED_
struct container_request_struct { CONTAINER_REQUEST_COMMON };
# endif

/* CONTAINER_WORKER_HOST: Obtains requests so that they may be processed by
 * workers; implemented by ContainerWare.
 */
typedef struct container_worker_host_struct CONTAINER_WORKER_HOST;

# define CONTAINER_WORKER_HOST_COMMON \
	struct container_worker_host_api_struct *api;

# ifndef CONTAINER_WORKER_HOST_STRUCT_DEFINED_
struct container_worker_host_struct { CONTAINER_WORKER_HOST_COMMON };
# endif

/* API exported by ContainerWare */
struct containerware_api_struct
{
	void *reserved;
	unsigned long (*addref)(CONTAINERWARE *me);
	unsigned long (*release)(CONTAINERWARE *me);
	int (*register_endpoint)(CONTAINERWARE *me, const char *scheme, ENDPOINT_SERVER *server);
	int (*register_container)(CONTAINERWARE *me, const char *name, CONTAINER *container);
	int (*lputs)(CONTAINERWARE *me, int severity, const char *str);
	int (*lvprintf)(CONTAINERWARE *me, int severity, const char *fmt, va_list ap);
	int (*lprintf)(CONTAINERWARE *me, int severity, const char *fmt, ...);
};

/* Logging macros */
# define DPRINTF(cw, ...) (cw)->api->lprintf(cw, CWLOG_DEBUG, __VA_ARGS__)
# define LPRINTF(cw, severity, ...) (cw)->api->lprintf(cw, severity, __VA_ARGS__)

/* Information passed to a container about a worker */
struct container_worker_info_struct
{
	/* Application name */
	char app[64];
	/* Instance name */
	char instance[64];
	/* Cluster or grouping name */
	char cluster[64];
	/* Process ID */
	pid_t pid;
	/* Thread ID */
	uint32_t workerid;
};

/* A container engine, which allows creation of request-processing
 * workers; implemented by container plug-ins
 */
struct container_api_struct
{
	void *reserved1;
	unsigned long (*addref)(CONTAINER *me);	
	unsigned long (*release)(CONTAINER *me);
	/* Create a new worker */
	int (*worker)(CONTAINER *me, CONTAINER_WORKER_HOST *host, const CONTAINER_WORKER_INFO *info, CONTAINER_WORKER **out);
};

/* A worker which is responsible for processing requests; implemented
 * by engine plug-ins.
 */
struct container_worker_api_struct
{
	void *reserved1;
	unsigned long (*addref)(CONTAINER_WORKER *me);
	unsigned long (*release)(CONTAINER_WORKER *me);
	int (*process)(CONTAINER_WORKER *me);
};

/* Nature of an endpoint (e.g., socket [file descriptor], dedicated
 * listening thread, etc.)
 */
typedef enum
{
	EM_SOCKET,
	EM_THREAD
} ENDPOINT_MODE;

/* Something which listens for requests; implemented by endpoint plug-ins */
struct endpoint_api_struct
{
	void *reserved1;
	unsigned long (*addref)(ENDPOINT *me);
	unsigned long (*release)(ENDPOINT *me);
	/* Return the mode of the endpoint */
	ENDPOINT_MODE (*mode)(ENDPOINT *me);
	/* EM_SOCKET: Obtain a file descriptor corresponding to the endpoint */
	int (*fd)(ENDPOINT *me);
	/* EM_THREAD: Block until a request is ready to be acquired */
	int (*process)(ENDPOINT *me);
	/* Acquire a request */
	int (*acquire)(ENDPOINT *me, CONTAINER_REQUEST **req);
};

/* A server which creates endpoints */
struct endpoint_server_api_struct
{
	void *reserved1;
	unsigned long (*addref)(ENDPOINT_SERVER *me);
	unsigned long (*release)(ENDPOINT_SERVER *me);
	int (*endpoint)(ENDPOINT_SERVER *me, URI *uri, ENDPOINT **endpoint);
};

/* A request which will be processed; implemented by endpoint plug-ins */
struct container_request_api_struct
{
	void *reserved1;
	unsigned long (*addref)(CONTAINER_REQUEST *me);
	unsigned long (*release)(CONTAINER_REQUEST *me);

	/* High-level introspection methods */
	int (*timestamp)(CONTAINER_REQUEST *me, struct timeval *tv);
	int (*status)(CONTAINER_REQUEST *me);
	size_t (*rbytes)(CONTAINER_REQUEST *me);
	size_t (*wbytes)(CONTAINER_REQUEST *me);
	const char *(*protocol)(CONTAINER_REQUEST *me);
	const char *(*method)(CONTAINER_REQUEST *me);
	const char *(*request_uri_str)(CONTAINER_REQUEST *me);
	
	/* Path information */
	/*
	request URI:        uri()       [the request URI as a URI]
	request URI string: uri_str()   [request URI as a string]
	request URI string: uri_vstr()  [request URI string as a jd_var]
	request array:      array()     [path components relative to app root]
	request base:       base()      [always has a trailing slash]
	request resource:   resource()  [no trailing slash; 'index' is added if at root]
    */
	
	const char *(*consume)(CONTAINER_REQUEST *me);

	/* Low-level introspection methods */
	const char *(*getenv)(CONTAINER_REQUEST *me, const char *name);
	int (*environment)(CONTAINER_REQUEST *me, jd_var *out);
	
	/* I/O */
	int (*header)(CONTAINER_REQUEST *me, const char *name, const char *value, int replace);
	int (*write)(CONTAINER_REQUEST *me, const char *buf, size_t buflen);
	int (*puts)(CONTAINER_REQUEST *me, const char *str);
	int (*close)(CONTAINER_REQUEST *me);
	int (*freadfile)(CONTAINER_REQUEST *me, FILE *fin);
	int (*readfile)(CONTAINER_REQUEST *me, const char *path);
};


/* A container host obtains requests so that they may be processed by
 * workers; implemented by ContainerWare
 */
struct container_worker_host_api_struct
{
	void *reserved1;
	void *reserved2;
	void *reserved3;
	/* Block waiting for the next request */
	int (*request)(CONTAINER_WORKER_HOST *me, CONTAINER_REQUEST **req);
	int (*lputs)(CONTAINER_WORKER_HOST *me, int severity, const char *str);
	int (*lvprintf)(CONTAINER_WORKER_HOST *me, int severity, const char *fmt, va_list ap);
	int (*lprintf)(CONTAINER_WORKER_HOST *me, int severity, const char *fmt, ...);
};

/* Logging macros */
# define WDPRINTF(host, ...) (host)->api->lprintf(host, CWLOG_DEBUG, __VA_ARGS__)
# define WLPRINTF(host, severity, ...) (host)->api->lprintf(host, severity, __VA_ARGS__)

/* *** APIs for module implementors *** */

/* Reference counting */
unsigned long cw_addref(pthread_mutex_t *lock, unsigned long *refcount);
unsigned long cw_release(pthread_mutex_t *lock, unsigned long *refcount);

/* Utilities for endpoint implementors */
struct cw_request_info_struct
{
	URI *request_uri;
	jd_var request_vstr;
	const char *request_str;
	jd_var request_array;
	
	jd_var params_array;
	
	jd_var current_array;
};

int cw_request_puts(CONTAINER_REQUEST *me, const char *str);
const char *cw_request_protocol(CONTAINER_REQUEST *me);
const char *cw_request_method(CONTAINER_REQUEST *me);
int cw_request_environment(jd_var *env, jd_var *out);
const char *cw_request_getenv(jd_var *env, const char *name);
int cw_request_env_init(char *const *env, jd_var *out);
int cw_request_headers_init(jd_var *out);
int cw_request_headers_set(jd_var *headers, const char *name, const char *value, int replace);
int cw_request_headers_vset(jd_var *headers, jd_var *name, jd_var *value, int replace);
int cw_request_headers_status(jd_var *headers);
int cw_request_headers_send(jd_var *headers, int (*callback)(void *data, const char *name, const char *value, int more), void *data);
int cw_request_info_init(struct cw_request_info_struct *info, jd_var *env);
int cw_request_info_destroy(struct cw_request_info_struct *info);
const char *cw_request_info_consume(struct cw_request_info_struct *info);
jd_var *cw_request_info_vconsume(struct cw_request_info_struct *info);

# if defined(__cplusplus)
}
# endif

#endif /*!CONTAINERWARE_H_*/
