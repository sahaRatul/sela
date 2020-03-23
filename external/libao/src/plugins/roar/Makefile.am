## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

if HAVE_ROAR_AUDIO

roarltlibs = libroar.la
roarsources = ao_roar.c

else

roarltlibs =
roarsources =

endif

AM_CPPFLAGS = -I$(top_builddir)/include/ao -I$(top_srcdir)/include

libdir = $(plugindir)
lib_LTLIBRARIES = $(roarltlibs)

libroar_la_CFLAGS = $(CFLAGS) @SLP_DEF@
libroar_la_LDFLAGS = @PLUGIN_LDFLAGS@
libroar_la_LIBADD = -lroar
libroar_la_SOURCES = $(roarsources)

EXTRA_DIST = ao_roar.c
