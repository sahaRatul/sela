## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

if HAVE_ESD

esdltlibs = libesd.la
esdsources = ao_esd.c

else

esdltlibs =
esdsources =

endif

AM_CPPFLAGS = -I$(top_builddir)/include/ao -I$(top_srcdir)/include @ESD_CFLAGS@

libdir = $(plugindir)
lib_LTLIBRARIES = $(esdltlibs)

libesd_la_LDFLAGS = @PLUGIN_LDFLAGS@
libesd_la_LIBADD = @ESD_LIBS@
libesd_la_SOURCES = $(esdsources)

EXTRA_DIST = ao_esd.c
