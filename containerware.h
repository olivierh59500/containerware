#ifndef CONTAINERWARE_H_
# define CONTAINERWARE_H_              1

# include <inttypes.h>
# include <sys/types.h>
# include <sys/time.h>

# include "liburi.h"

# if (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L) && !defined(restrict)
#  define restrict
# endif

# if defined(__cplusplus)
extern "C" {
# endif

# ifdef CONTAINER_PLUGIN_INTERNAL
#  define CONTAINER_STRUCT_DEFINED_    1
#  define CONTAINER_INSTANCE_STRUCT_DEFINED_ 1
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

/* CONTAINER: Something which creates instances to process requests */
typedef struct container_struct CONTAINER;

# define CONTAINER_COMMON \
	struct container_api_struct *api;

# ifndef CONTAINER_STRUCT_DEFINED_
struct container_struct { CONTAINER_COMMON };
# endif

/* CONTAINER_INSTANCE: An instance of a container */
typedef struct container_instance_struct CONTAINER_INSTANCE;
typedef struct container_instance_info_struct CONTAINER_INSTANCE_INFO;

# define CONTAINER_INSTANCE_COMMON \
	struct container_instance_api_struct *api;

# ifndef CONTAINER_INSTANCE_STRUCT_DEFINED_
struct container_instance_struct { CONTAINER_INSTANCE_COMMON };
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
 * by an instance.
 */
typedef struct container_request_struct CONTAINER_REQUEST;

# define CONTAINER_REQUEST_COMMON \
	struct container_request_api_struct *api;

# ifndef CONTAINER_REQUEST_STRUCT_DEFINED_
struct container_request_struct { CONTAINER_REQUEST_COMMON };
# endif

/* CONTAINER_INSTANCE_HOST: Obtains requests so that they may be processed by
 * instances; implemented by ContainerWare.
 */
typedef struct container_instance_host_struct CONTAINER_INSTANCE_HOST;

# define CONTAINER_INSTANCE_HOST_COMMON \
	struct container_instance_host_api_struct *api;

# ifndef CONTAINER_INSTANCE_HOST_STRUCT_DEFINED_
struct container_instance_host_struct { CONTAINER_INSTANCE_HOST_COMMON };
# endif

/* API exported by ContainerWare */
struct containerware_api_struct
{
	void *reserved;
	unsigned long (*addref)(CONTAINERWARE *me);
	unsigned long (*release)(CONTAINERWARE *me);
	int (*register_endpoint)(CONTAINERWARE *me, const char *scheme, ENDPOINT_SERVER *server);
	int (*register_container)(CONTAINERWARE *me, const char *name, CONTAINER *container);
};

/* Information passed to a container about an instance */
struct container_instance_info_struct
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
	uint32_t threadid;
};

/* A container engine, which allows creation of request-processing
 * instances; implemented by container plug-ins
 */
struct container_api_struct
{
	void *reserved1;
	unsigned long (*addref)(CONTAINER *me);	
	unsigned long (*release)(CONTAINER *me);
	/* Create a new instance */
	int (*instance)(CONTAINER *me, CONTAINER_INSTANCE_HOST *host, const CONTAINER_INSTANCE_INFO *info, CONTAINER_INSTANCE **out);
};

/* An instance which is responsible for processing requests; implemented
 * by engine plug-ins.
 */
struct container_instance_api_struct
{
	void *reserved1;
	unsigned long (*addref)(CONTAINER_INSTANCE *me);
	unsigned long (*release)(CONTAINER_INSTANCE *me);
	int (*process)(CONTAINER_INSTANCE *me);
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
	const char *(*protocol)(CONTAINER_REQUEST *me);
	const char *(*method)(CONTAINER_REQUEST *me);
	const char *(*request_uri_str)(CONTAINER_REQUEST *me);
	
	/* Low-level introspection methods */
	const char *(*getenv)(CONTAINER_REQUEST *me, const char *name);
	
	/* I/O */
	int (*header)(CONTAINER_REQUEST *me, const char *name, const char *value, int replace);
	int (*write)(CONTAINER_REQUEST *me, const char *buf, size_t buflen);
	int (*puts)(CONTAINER_REQUEST *me, const char *str);
	int (*close)(CONTAINER_REQUEST *me);
	int (*freadfile)(CONTAINER_REQUEST *me, FILE *fin);
	int (*readfile)(CONTAINER_REQUEST *me, const char *path);
};


/* A container host obtains requests so that they may be processed by
 * instances; implemented by ContainerWare
 */
struct container_instance_host_api_struct
{
	void *reserved1;
	void *reserved2;
	void *reserved3;
	/* Block waiting for the next request */
	int (*request)(CONTAINER_INSTANCE_HOST *me, CONTAINER_REQUEST **req);
};

# if defined(__cplusplus)
}
# endif

#endif /*!CONTAINERWARE_H_*/
