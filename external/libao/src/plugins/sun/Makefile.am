## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

if HAVE_SUN_AUDIO

sunltlibs = libsun.la
sunsources = ao_sun.c

else

sunltlibs =
sunsources =

endif

AM_CPPFLAGS = -I$(top_builddir)/include/ao -I$(top_srcdir)/include

libdir = $(plugindir)
lib_LTLIBRARIES = $(sunltlibs)

libsun_la_LDFLAGS = @PLUGIN_LDFLAGS@
libsun_la_SOURCES = $(sunsources)

EXTRA_DIST = ao_sun.c
