## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

if HAVE_NAS

nasltlibs = libnas.la
nasldflags = -export-dynamic -avoid-version
nassources = ao_nas.c

else

nasltlibs =
nasldflags =
nassources =

endif

AM_CPPFLAGS = -I$(top_builddir)/include/ao -I$(top_srcdir)/include @NAS_CFLAGS@

libdir = $(plugindir)
lib_LTLIBRARIES = $(nasltlibs)

libnas_la_LDFLAGS = $(nasldflags)
libnas_la_LIBADD = @NAS_LIBS@
libnas_la_SOURCES = $(nassources)

EXTRA_DIST = ao_nas.c
