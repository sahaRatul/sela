## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

if HAVE_IRIX

irixltlibs = libirix.la
irixsources = ao_irix.c

else

irixltlibs =
irixsources =

endif

AM_CPPFLAGS = -I$(top_builddir)/include/ao -I$(top_srcdir)/include

libdir = $(plugindir)
lib_LTLIBRARIES = $(irixltlibs)

libirix_la_LDFLAGS = @PLUGIN_LDFLAGS@
libirix_la_LIBADD = -laudio
libirix_la_SOURCES = $(irixsources)

EXTRA_DIST = ao_irix.c
