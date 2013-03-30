pkglib_LTLIBRARIES = info.la

info_la_SOURCES = p_info.h \
	container.c instance.c

info_la_CPPFLAGS = @LIBURI_CPPFLAGS@

info_la_LIBADD = @LIBURI_LDFLAGS@ @LIBURI_LOCAL_LIBS@ @LIBURI_LIBS@

info_la_LDFLAGS = -module -avoid-version -no-undefined
