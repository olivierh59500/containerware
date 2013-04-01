DIST_SUBDIRS = m4 liburi jsondata libcontainer libship endpoints containers

SUBDIRS = @subdirs@ libcontainer . libship endpoints containers

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = containerware
noinst_LTLIBRARIES = libiniparser.la

containerware_SOURCES = p_containerware.h ptrlist-impl.h \
	main.c log.c config.c module.c server.c route.c \
	listener.c listener-socket-thread.c listener-endpoint-thread.c \
	container.c host.c worker.c worker-thread.c \
	container-list.c listener-list.c server-list.c \
	worker-list.c host-list.c

containerware_CPPFLAGS = @LIBCONTAINER_CPPFLAGS@

containerware_LDADD = libiniparser.la \
	@LIBCONTAINER_LOCAL_LIBS@ @LIBCONTAINER_LIBS@

containerware_LDFLAGS = @LIBCONTAINER_LDFLAGS@

libiniparser_la_SOURCES = \
	iniparser/src/dictionary.h \
	iniparser/src/dictionary.c \
	iniparser/src/iniparser.h \
	iniparser/src/iniparser.c

libiniparser_la_LDFLAGS = -static
