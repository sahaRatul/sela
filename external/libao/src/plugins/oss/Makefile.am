## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

if HAVE_OSS

ossltlibs = liboss.la
osssources = ao_oss.c

else

ossltlibs =
osssources =

endif

AM_CPPFLAGS = -I$(top_builddir)/include/ao -I$(top_srcdir)/include

libdir = $(plugindir)
lib_LTLIBRARIES = $(ossltlibs)

liboss_la_LDFLAGS = @PLUGIN_LDFLAGS@
liboss_la_SOURCES = $(osssources)

EXTRA_DIST = ao_oss.c
