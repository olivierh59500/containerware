pkglib_LTLIBRARIES = dummy.la

dummy_la_SOURCES = p_dummy.h \
	server.c endpoint.c request.c

dummy_la_CPPFLAGS = @LIBURI_CPPFLAGS@

dummy_la_LIBADD = @LIBURI_LDFLAGS@ @LIBURI_LOCAL_LIBS@ @LIBURI_LIBS@

dummy_la_LDFLAGS = -module -avoid-version -no-undefined
