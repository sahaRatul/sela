## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

if HAVE_ARTS

artsltlibs = libarts.la
artssources = ao_arts.c

else

artsltlibs =
artssources =

endif

AM_CPPFLAGS = -I$(top_builddir)/include/ao -I$(top_srcdir)/include @ARTS_CFLAGS@

libdir = $(plugindir)
lib_LTLIBRARIES = $(artsltlibs)

libarts_la_LDFLAGS = @PLUGIN_LDFLAGS@
libarts_la_LIBADD = @ARTS_LIBS@
libarts_la_SOURCES = $(artssources)

EXTRA_DIST = ao_arts.c
