lib_LTLIBRARIES = libship.la

libship_la_SOURCES = p_ship.h ship.h \
	ship.c

libship_la_CPPFLAGS = @LIBCONTAINER_CPPFLAGS@

libship_la_LDFLAGS = -avoid-version @LIBURI_LDFLAGS@ @LIBJSONDATA_LDFLAGS@

libship_la_LIBADD = \
	@LIBCONTAINER_LOCAL_LIBS@ @LIBCONTAINER_LIBS@
