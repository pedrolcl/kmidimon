KMidimon
kmidimon
1
kmidimon
MIDI monitor using ALSA sequencer and Qt5 user interface
kmidimon
Standard options...
Description
===========

KMidimon monitors events coming from a MIDI external port or application
via the ALSA sequencer, or stored as Standard MIDI Files. It is
especially useful if you want to debug MIDI software or your MIDI setup.
It features a nice graphical user interface, customizable event filters
and sequencer parameters, support for all MIDI messages and some ALSA
messages, and saving the recorded event list to a text file or SMF.

Options
=======

Generic options:

`--help`

:   Show help about options

`--help-qt`

:   Show QT specific options

`--help-all`

:   Show all options

`--author`

:   Show author information

`-v`,`--version`

:   Show version information

`--license`

:   Show license information

QT options:

`--display` `displayname`

:   Use the X-server display `displayname`.

`--session` `sessionId`

:   Restore the application for the given `sessionId`.

`--cmap`

:   Causes the application to install a private color map on an 8-bit
    display.

`--ncols` `count`

:   Limits the number of colors allocated in the color cube on an 8-bit
    display, if the application is using the QApplication::ManyColor
    color specification.

`--nograb`

:   Tells QT to never grab the mouse or the keyboard.

`--dograb`

:   Running under a debugger can cause an implicit `--nograb`, use
    `--dograb` to override.

`--sync`

:   Switches to synchronous mode for debugging.

`--fn`,`--font` `fontname`

:   Defines the application font.

`--bg`,`--background` `color`

:   Sets the default background color and an application palette (light
    and dark shades are calculated).

`--fg`,`--foreground` `color`

:   Sets the default foreground color

`--btn`,`--button` `color`

:   Sets the default button color.

`--name` `name`

:   Sets the application name.

`--title` `title`

:   Sets the application title (caption).

`--visual` `TrueColor`

:   Forces the application to use a `TrueColor` visual on an 8-bit
    display.

`--inputstyle` `inputstyle`

:   Sets XIM (X Input Method) input style. Possible values are
    `onthespot`, `overthespot`, `offthespot` and `root`.

`--im` `XIM server`

:   Set XIM server.

`--noxim`

:   Disable XIM

`--reverse`

:   mirrors the whole layout of widgets

`--stylesheet` file.qss

:   applies the QT stylesheet to the application widgets

License
=======

This manual page was written by Pedro Lopez-Cabanillas
<plcl@users.sourceforge.net> Permission is granted to copy, distribute
and/or modify this document under the terms of the GNU General Public
License, Version 2 or any later version published by the Free Software
Foundation; considering as source code all the file that enable the
production of this manpage.
