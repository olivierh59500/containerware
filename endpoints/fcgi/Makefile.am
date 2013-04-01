if WITH_FASTCGI

pkglib_LTLIBRARIES = fcgi.la

fcgi_la_SOURCES = p_fcgi.h \
	server.c endpoint.c request.c

fcgi_la_CPPFLAGS = @LIBCONTAINER_CPPFLAGS@ @fcgi_CFLAGS@

fcgi_la_LIBADD = @LIBCONTAINER_LOCAL_LIBS@ @LIBCONTAINER_LIBS@ \
	@fcgi_LDFLAGS@ @fcgi_LIBS@

fcgi_la_LDFLAGS = -module -avoid-version -no-undefined @LIBCONTAINER_LDFLAGS@

endif
