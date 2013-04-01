pkglib_LTLIBRARIES = info.la

info_la_SOURCES = p_info.h \
	container.c worker.c

info_la_CPPFLAGS = @LIBCONTAINER_CPPFLAGS@

info_la_LIBADD = @LIBCONTAINER_LOCAL_LIBS@ @LIBCONTAINER_LIBS@

info_la_LDFLAGS = -module -avoid-version -no-undefined @LIBCONTAINER_LDFLAGS@
