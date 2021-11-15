# Drumstick MIDI Monitor (a.k.a. kmidimon)

Drumstick MIDI Monitor logs MIDI events coming from MIDI external ports or applications via the ALSA sequencer, 
and from SMF (Standard MIDI files) or WRK (Cakewalk/Sonar) files. It is especially useful if you want to debug 
MIDI software or your MIDI setup. It features a nice graphical user interface, customizable event filters and 
sequencer parameters, support for MIDI and ALSA messages, and saving the recorded event list to a SMF or text file.

For brief building instructions, see INSTALL.

## Developers environment

You need the following software:

* CMake 3.14
* Qt5 libraries
* ALSA library
* Drumstick 2.0

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

### Check out the module kmidimon from the SVN repository.

Host: kmidimon.svn.sourceforge.net

Path: /svnroot/kmidimon/trunk

Module: kmidimon

example:

~~~
$ svn co https://kmidimon.svn.sourceforge.net/svnroot/kmidimon/trunk kmidimon 
~~~

### Configure and compile

~~~
$ cmake . -DCMAKE_BUILD_TYPE=debug -DCMAKE_INSTALL_PREFIX=/usr/local
$ make VERBOSE=1
~~~
 
### Hack and enjoy!
