DIST_SUBDIRS = m4 liburi endpoints containers

SUBDIRS = @subdirs@ endpoints containers .

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = containerware
noinst_LTLIBRARIES = libiniparser.la

containerware_SOURCES = p_containerware.h ptrlist-impl.h \
	main.c log.c config.c module.c server.c route.c \
	listener.c listener-socket-thread.c listener-endpoint-thread.c \
	container.c host.c worker.c worker-thread.c \
	container-list.c listener-list.c server-list.c \
	worker-list.c host-list.c

containerware_LDADD = libiniparser.la \
	@LIBURI_LDFLAGS@ @LIBURI_LOCAL_LIBS@ @LIBURI_LIBS@ \
	@LIBJSONDATA_LDFLAGS@ @LIBJSONDATA_LOCAL_LIBS@ @LIBJSONDATA_LIBS@
	
containerware_CPPFLAGS = @LIBURI_CPPFLAGS@ @LIBJSONDATA_CPPFLAGS@

libiniparser_la_SOURCES = \
	iniparser/src/dictionary.h \
	iniparser/src/dictionary.c \
	iniparser/src/iniparser.h \
	iniparser/src/iniparser.c

libiniparser_la_LDFLAGS = -static
