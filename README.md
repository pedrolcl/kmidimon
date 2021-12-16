# Drumstick MIDI Monitor (a.k.a. kmidimon)

[Drumstick MIDI Monitor](https://kmidimon.sourceforge.io) logs MIDI events 
coming from MIDI external ports or applications via the ALSA sequencer, 
and from SMF (Standard MIDI files) or WRK (Cakewalk/Sonar) files. It is especially useful if you want to debug 
MIDI software or your MIDI setup. It features a nice graphical user interface, customizable event filters and 
sequencer parameters, support for MIDI and ALSA messages, and saving the recorded event list to a SMF or text file.

For brief building instructions, see INSTALL.

## Downloads

Sources: https://sourceforge.net/projects/kmidimon/files/

[![Download Drumstick MIDI Monitor](https://a.fsdn.com/con/app/sf-download-button)](https://sourceforge.net/projects/kmidimon/files/latest/download)

[<img width='240' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-en.png'/>](https://flathub.org/apps/details/net.sourceforge.kmidimon)

[![Packaging status](https://repology.org/badge/vertical-allrepos/kmidimon.svg)](https://repology.org/project/kmidimon/versions)

## Developers environment

You need the following software:

* CMake 3.14 or later
* Qt libraries 5.12 or later
* ALSA library
* Drumstick 2.5 or later

## Getting the development sources

Compiling and hacking the SVN sources is a bit different compared to the
distribution tarball. You can get the latest sources either using a sourceforge
user account, or the anonymous user (with read only rights). The SVN client 
documentation for SourceForge users is available at:
 
https://sourceforge.net/p/forge/documentation/svn/

### Check out the module Drumstick from the SVN repository.

Host: drumstick.svn.sourceforge.net

Path: /svnroot/drumstick/trunk

Module: drumstick

example:

~~~
$ svn co https://drumstick.svn.sourceforge.net/svnroot/drumstick/trunk drumstick 
~~~

There is also a Git mirror at [GitHub](https://github.com/pedrolcl/drumstick)

### Check out the module kmidimon from the SVN repository.

Host: kmidimon.svn.sourceforge.net

Path: /svnroot/kmidimon/trunk

Module: kmidimon

example:

~~~
$ svn co https://kmidimon.svn.sourceforge.net/svnroot/kmidimon/trunk kmidimon 
~~~

There is also a Git mirror at [GitHub](https://github.com/pedrolcl/kmidimon)

### Configure and compile

~~~
$ cmake . -DCMAKE_BUILD_TYPE=debug -DCMAKE_INSTALL_PREFIX=/usr/local
$ make VERBOSE=1
~~~
 
### Hack and enjoy!
