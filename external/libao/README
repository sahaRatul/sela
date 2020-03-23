libao - A Cross-platform Audio Library, Version 0.8.6

Originally Copyright (C) Aaron Holtzman - May 1999
Changes Copyright (C) Jack Moffitt - October 2000
Changes Copyright (C) Stan Seibert - July 2000-March 2004
libao-pulse Copyright (C) Lennart Poettering 2004-2006
Changes Copyright (C) 2004-2005 Xiph.org Foundation 
Changes Maintainer Benjamin Gerard

libao is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

libao is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

---------------------------------------------------------------------

OVERVIEW

Libao is a cross-platform audio library that allows programs to output
audio using a simple API on a wide variety of platforms.  It currently
supports:
   * Null output
   * WAV files
   * OSS (Open Sound System)
   * ESD (ESounD or Enlightened Sound Daemon)
   * ALSA (Advanced Linux Sound Architecture)
   * PulseAudio (next generation GNOME sound server)
   * AIX
   * Solaris (untested)
   * IRIX (untested)

HISTORY

Libao began life as cross-platform audio library inside of ac3dec, an
AC3 decoder by Aaron Holtzman that is part of the LiViD project.  When
ogg123 (part of the command line vorbis tools) needed a way to play
audio on multiple operating systems, someone on the vorbis-dev mailing
list suggested the libao library as a possible way to add cross-platform 
support to ogg123. Stan Seibert downloaded the libao library, severely 
hacked it up in order to make the build process simpler and support 
multiple live-playback devices. (The original code allowed one live 
playback driver, the wav driver, and a null driver to be compiled into 
the library.) Jack Moffitt got it supporting dynamically loaded plugins 
so that binary versions of libao could be provided. The API was revised 
for version 0.8.0.

This code is being maintained by Stan Seibert (volsung@xiph.org) 
and various other individuals.  Please DO NOT annoy Aaron Holtzman about 
bugs, features, comments, etc. regarding this code.

WORKAROUNDS

The OSS emulation in ALSA deviates from the OSS spec by not returning
immediately from an open() call if the OSS device is already in use.
Instead, it makes the application wait until the device is available.
This is not desirable during the autodetection phase of libao, so a
workaround has been included in the source.  Since the workaround
itself violates the OSS spec and causes other problems on some
platforms, it is only enabled when ALSA is detected.  The workaround
can be turned on or off by passing the --enable-broken-oss or
--disable-broken-oss flag to the configure script.
