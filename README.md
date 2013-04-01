ContainerWare is an application server, similar in concept to Tomcat or
Jetty, except that it's written in C and the applications (containers)
are implemented as dynamically-loadable modules.

It listens on one or more endpoints (which might be TCP/IP ports, FastCGI
sockets, etc.) and passes requests to application instances for processing.
Depending upon the engine processing the request, the instance might be one
of a pool of threads, or child processes, or something else.
