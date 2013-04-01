lib_LTLIBRARIES = libcontainer.la

libcontainer_la_SOURCES = p_cw.h containerware.h \
	refcount.c endpoint.c

libcontainer_la_CPPFLAGS = @LIBURI_CPPFLAGS@ @LIBJSONDATA_CPPFLAGS@

libcontainer_la_LDFLAGS = -avoid-version @LIBURI_LDFLAGS@ @LIBJSONDATA_LDFLAGS@

libcontainer_la_LIBADD = \
	@LIBURI_LOCAL_LIBS@ @LIBURI_LIBS@ \
	@LIBJSONDATA_LOCAL_LIBS@ @LIBJSONDATA_LIBS@
