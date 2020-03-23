Name:		libao
Version:	0.8.6
Release:	1
Summary:	Cross-Platform Audio Output Library

Group:		System Environment/Libraries
License:	GPL
URL:		http://www.xiph.org/
Vendor:		Xiph.org Foundation <team@xiph.org>
Source:		http://www.xiph.org/ao/src/%{name}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-root

# glibc-devel is needed for oss plug-in build
BuildRequires:  glibc-devel
%{!?_without_esd:BuildRequires: esound-devel >= 0.2.8}
%{!?_without_arts:BuildRequires: arts-devel}
%{?_with_alsa:BuildRequires: alsa-lib-devel >= 0.9.0}
# FIXME: perl is needed for the dirty configure flag trick, which should be
# solved differently
BuildRequires:  perl

%description
Libao is a cross-platform audio output library.  It currently supports
ESD, aRts, ALSA, OSS, *BSD and Solaris.

This package provides plug-ins for OSS, ESD, aRts, and ALSA (0.9).  You will
need to install the supporting libraries for any plug-ins you want to use
in order for them to work.

Available rpmbuild rebuild options :
--with : alsa
--without : esd arts

%package devel
Summary: Cross Platform Audio Output Library Development
Group: Development/Libraries
Requires: libao = %{version}

%description devel
The libao-devel package contains the header files, libraries and
documentation needed to develop applications with libao.

%prep
%setup -q -n %{name}-%{version}

perl -p -i -e "s/-O20/$RPM_OPT_FLAGS/" configure
perl -p -i -e "s/-ffast-math//" configure

%build

%configure \
    --disable-nas \
    --disable-alsa \
    %{?_with_alsa:--enable-alsa09} %{!?_with_alsa:--disable-alsa09} \
    %{?_without_esd:--disable-esd} \
    %{?_without_arts:--disable-arts}

make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

#FIXME: makeinstall breaks the plugin install location; they end up in /usr/lib
make DESTDIR=$RPM_BUILD_ROOT install

%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun
if [ "$1" -ge "1" ]; then
  /sbin/ldconfig
fi

%files
%defattr(-,root,root)
%doc AUTHORS CHANGES COPYING README
%{_libdir}/libao.so.*
%{_libdir}/ao/*/liboss.so
%{!?_without_esd:%{_libdir}/ao/*/libesd.so}
%{!?_without_arts:%{_libdir}/ao/*/libarts.so}
%{?_with_alsa:%{_libdir}/ao/*/libalsa09.so}
%{_mandir}/man5/*

%files devel
%defattr(-,root,root)
%doc doc/*
%{_includedir}/ao
%{_libdir}/libao.so
%{_libdir}/libao.la
%{_libdir}/ao/*/liboss.la
%{!?_without_esd:%{_libdir}/ao/*/libesd.la}
%{!?_without_arts:%{_libdir}/ao/*/libarts.la}
%{?_with_alsa:%{_libdir}/ao/*/libalsa09.la}
%{_datadir}/aclocal/ao.m4
%{_libdir}/pkgconfig/ao.pc

%changelog
* Mon Mar 25 2004 Gary Peck <gbpeck@sbcglobal.net> 0.8.5-3
- Set default user and permissions on the devel package

* Mon Mar 22 2004 Gary Peck <gbpeck@sbcglobal.net> 0.8.5-2
- Update source URL
- Add support for "--with alsa", "--without esd" and "--without arts"
- Make configure more explicit on what plugins to enable

* Fri Mar 11 2004 Stan Seibert <volsung@xiph.org> 0.8.5-1
- Version bump

* Fri Oct 5 2003 Stan Seibert <volsung@xiph.org> 0.8.4-1
- Remove alsa libraries from RPM since RedHat doesn't ship with ALSA
  ALSA users will need to recompile from source.
- Add ao.pc to -devel
- Make the devel libraries .la instead of .a

* Fri Jul 19 2002 Michael Smith <msmith@xiph.org> 0.8.3-2
- re-disable static libraries (they do not work - at all)

* Sun Jul 14 2002 Thomas Vander Stichele <thomas@apestaart.org> 0.8.3-1
- new release for vorbis 1.0
- small cleanups
- added better BuildRequires
- added alsa-lib-devel 0.9.0 buildrequires
- added static libraries to -devel
- added info about plug-ins to description
- listed plug-in so files explicitly to ensure package build fails when one
  is missing

* Mon Jan  7 2002 Peter Jones <pjones@redhat.com> 0.8.2-4
- minor cleanups, even closer to RH .spec 
- arts-devel needs a build dependancy to be sure the
  plugin will get built

* Wed Jan  2 2002 Peter Jones <pjones@redhat.com> 0.8.2-3
- fix libao.so's provide

* Wed Jan  2 2002 Peter Jones <pjones@redhat.com> 0.8.2-2
- merge RH and Xiphophorous packages

* Tue Dec 18 2001 Jack Moffitt <jack@xiph.org>
- Update for 0.8.2 release.

* Sun Oct 07 2001 Jack Moffitt <jack@xiph.org>
- supports configurable prefixes

* Sun Oct 07 2001 Stan Seibert <indigo@aztec.asu.edu>
- devel packages look for correct documentation files
- added ao/plugin.h include file to devel package
- updated package description

* Sun Sep 03 2000 Jack Moffitt <jack@icecast.org>
- initial spec file created
