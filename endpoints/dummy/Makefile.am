pkglib_LTLIBRARIES = dummy.la

dummy_la_SOURCES = p_dummy.h \
	server.c endpoint.c request.c

dummy_la_CPPFLAGS = @LIBCONTAINER_CPPFLAGS@

dummy_la_LIBADD = @LIBCONTAINER_LOCAL_LIBS@ @LIBCONTAINER_LIBS@

dummy_la_LDFLAGS = -module -avoid-version -no-undefined @LIBCONTAINER_LDFLAGS@
