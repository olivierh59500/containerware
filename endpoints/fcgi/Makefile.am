if WITH_FASTCGI

pkglib_LTLIBRARIES = fcgi.la

fcgi_la_SOURCES = p_fcgi.h \
	server.c endpoint.c request.c

fcgi_la_CPPFLAGS = @LIBURI_CPPFLAGS@ @fcgi_CFLAGS@

fcgi_la_LIBADD = @LIBURI_LDFLAGS@ @LIBURI_LOCAL_LIBS@ @LIBURI_LIBS@ \
	@fcgi_LDFLAGS@ @fcgi_LIBS@

fcgi_la_LDFLAGS = -module -avoid-version -no-undefined

endif
