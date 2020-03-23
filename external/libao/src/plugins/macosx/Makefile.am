## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

if HAVE_MACOSX

macosxltlibs = libmacosx.la
macosxsources = ao_macosx.c

else

macosxltlibs =
macosxsources =

endif

AM_CPPFLAGS = -I$(top_builddir)/include/ao -I$(top_srcdir)/include

libdir = $(plugindir)
lib_LTLIBRARIES = $(macosxltlibs)

libmacosx_la_LDFLAGS = @PLUGIN_LDFLAGS@ -framework AudioUnit -framework CoreAudio -framework CoreServices
libmacosx_la_SOURCES = $(macosxsources)

EXTRA_DIST = ao_macosx.c
