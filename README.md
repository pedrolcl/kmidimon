# Drumstick MIDI Monitor (a.k.a. kmidimon)

[![Build on Linux](https://github.com/pedrolcl/kmidimon/actions/workflows/linux-build.yml/badge.svg)](https://github.com/pedrolcl/kmidimon/actions/workflows/linux-build.yml)

[Drumstick MIDI Monitor](https://kmidimon.sourceforge.io) logs MIDI events
coming from MIDI external ports or applications via the ALSA sequencer,
and from SMF (Standard MIDI files) or WRK (Cakewalk/Sonar) files. It is especially useful if you want to debug
MIDI software or your MIDI setup. It features a nice graphical user interface, customizable event filters and
sequencer parameters, support for MIDI and ALSA messages, and saving the recorded event list to a SMF or text file.

For brief building instructions, see [INSTALL.md](INSTALL.md).

## Downloads

Sources: https://sourceforge.net/projects/kmidimon/files/

[![Download Drumstick MIDI Monitor](https://a.fsdn.com/con/app/sf-download-button)](https://sourceforge.net/projects/kmidimon/files/latest/download)

[<img width='240' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-en.png'/>](https://flathub.org/apps/details/net.sourceforge.kmidimon)

[![Packaging status](https://repology.org/badge/vertical-allrepos/kmidimon.svg)](https://repology.org/project/kmidimon/versions)

## Developers environment

You need the following software:

* CMake 3.16 or later
* Qt libraries 6.2 or later (Qt >= 5.12 using USE_QT5=On)
* ALSA library
* Drumstick 2.9 or later

## Getting the development sources

Compiling and hacking the Git sources is a bit different compared to the
distribution tarball. You can get the latest sources either using a sourceforge
user account, or the anonymous user (with read only rights). The Git client 
documentation for SourceForge users is available at:
https://sourceforge.net/p/forge/documentation/Git/

### Clone the [drumstick Git repository](https://sourceforge.net/p/drumstick/git/ci/master/tree/).

There is also a [Git mirror at GitHub](https://github.com/pedrolcl/drumstick)

example:

~~~sh
    git clone git://git.code.sf.net/p/drumstick/git drumstick-git
~~~

### Clone kmidimon from the [Git repository](https://sourceforge.net/p/kmidimon/git/ci/master/tree/)).

example:

~~~sh
    git clone git://git.code.sf.net/p/kmidimon/git kmidimon-git
~~~

There is also a [Git mirror at GitHub](https://github.com/pedrolcl/kmidimon)

### Configure and compile

~~~sh
    cd kmidimon-git
    mkdir build
    cmake -S . -B build -DCMAKE_BUILD_TYPE=debug \
               -DCMAKE_PREFIX_PATH=$HOME/Qt/6.6.1/gcc_64/ \
               -DCMAKE_INSTALL_PREFIX=/usr/local/
    cmake --build build --verbose
~~~
 
See also [INSTALL.md](INSTALL.md)
 
### Hack and enjoy!
