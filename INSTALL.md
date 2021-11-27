# Drumstick MIDI Monitor (a.k.a. kmidimon)

## Requirements

You need the following software:

* CMake 3.14 or later
* Qt libraries 5.12 or later
* ALSA library
* Drumstick 2.5 or later

You will need CMake 3.14 or newer.  If your Linux distribution
doesn't provide CMake, or if it provides an older version, you can get
it here:

https://cmake.org/download/

There are ready to use binary packages available for Linux from
that page. If you must build it from source, please read the instructions
supplied here:

https://cmake.org/install/

Qt and ALSA are probably available from your Linux distribution repositories.
If not, you may find them here:

https://download.qt.io/official_releases/online_installers/

https://www.alsa-project.org/wiki/Main_Page

This program needs the Drumstick libraries. You should install the development
package before trying to compile kmidimon. Drumstick is available here:

https://drumstick.sourceforge.io

## Building

Unpack the tarball or check out SVN.  Assuming that you have the
source in ~/src/kmidimon, you need to change to that directory:

~~~
$ cd ~/src/kmidimon
~~~

Create a build directory, and change to it

~~~
$ mkdir build
$ cd build
~~~

Now run CMake to generate the build files.

~~~
$ cmake ..
~~~

Finally, run make, and then (sudo) make install, and you're done.

~~~
$ make
$ sudo make install
~~~

To uninstall, use:

~~~
$ sudo make uninstall
~~~

## Advanced Build Options

By default, make will output brief details of each build step.  If you
prefer to see full command lines, use:

~~~
$ make VERBOSE=1
~~~

Another option, useful for packagers, is setting `DESTDIR` at install
time. The `DESTDIR` directory will be prepended to the prefix when
copying the files:

~~~
$ make install DESTDIR=~/rpmroot
~~~

Some variables you may want to set:

* **CMAKE_INSTALL_PREFIX**

  `cmake .. -DCMAKE_INSTALL_PREFIX=/opt` is the equivalent to
  `./configure --prefix=/opt` for programs that use autotools
  
* **CMAKE_PREFIX_PATH** (location of alternative Qt5 libs):

  `cmake .. -DCMAKE_PREFIX_PATH=~/Qt/5.15.2/gcc_64`
  
* **EMBED_TRANSLATIONS** (instead of installing translations):

  `cmake .. -DEMBED_TRANSLATIONS=Yes`

* **BUILD_DOCS**  (default ON, to compile or use the pre-built documentation)

  `cmake .. -DBUILD_DOCS=OFF`

* **USE_QT**   

  `cmake .. -DUSE_QT=6` (to choose explicitly among Qt5 or Qt6)
  
* **Drumstick_DIR** (location of the custom Drumstick build):

  `cmake .. -DDrumstick_DIR=~/drumstick/build`

If you would prefer to avoid all this typing, you can use ccmake to
view and change these options using a friendly curses-based interface:

~~~
$ ccmake ..
~~~

or a GUI equivalent:

~~~
$ cmake-gui ..
~~~

## Dealing with Configuration Problems

First, look for an answer in CMake FAQ:

https://gitlab.kitware.com/cmake/community/-/wikis/FAQ

You may want to read the documentation at:

https://cmake.org/cmake/help/v3.14/

If you can't solve your problem, open a request for support at the project site:

https://sourceforge.net/p/kmidimon/support-requests/
