% KMIDIMON(1) kmidimon 0.0.0 | Drumstick MIDI Monitor
% Pedro López-Cabanillas <plcl@users.sf.net>

# NAME

**kmidimon** - Drumstick MIDI monitor using ALSA sequencer and QT user interface

# SYNOPSIS

| **kmidimon** \[ options ] \[ file ]

# DESCRIPTION

Drumstick MIDI Monitor records events coming from a MIDI external port or application
via the ALSA sequencer, or stored as Standard MIDI Files (SMF), RIFF MIDI (RMI) 
or Cakewalk (WRK) files.
It is especially useful if you want to debug MIDI software or your MIDI setup.
It features a nice graphical user interface, customizable event filters
and sequencer parameters, support for all MIDI messages and some ALSA
messages, and saving the recorded event list to a text file or SMF.

## Generic options

`--help`

:   Show help about options

`--help-all`

:   Show all options

`-v`,`--version`

:   Show version information

## QT options

The following options apply to all Qt5 applications.

`-style=` style / `-style` style

:   Set the application GUI style. Possible values depend on the system
    configuration. If Qt is compiled with additional styles or has
    additional styles as plugins these will be available to the `-style`
    command line option.

`-stylesheet=` stylesheet / `-stylesheet` stylesheet

:   Set the application styleSheet. The value must be a path to a file
    that contains the Style Sheet.

`-widgetcount`

:   Print debug message at the end about number of widgets left
    undestroyed and maximum number of widgets existed at the same time.

`-reverse`

:   Set the application's layout direction to Qt::RightToLeft. This
    option is intended to aid debugging and should not be used in
    production. The default value is automatically detected from the
    user's locale (see also QLocale::textDirection()).

`-platform` platformName\[:options\]

:   Specify the Qt Platform Abstraction (QPA) plugin.

`-platformpluginpath` path

:   Specify the path to platform plugins.

`-platformtheme` platformTheme

:   Specify the platform theme.

`-plugin` plugin

:   Specify additional plugins to load. The argument may appear multiple
    times.

`-qwindowgeometry` geometry

:   Specify the window geometry for the main window using the
    X11-syntax. For example: -qwindowgeometry 100x100+50+50

`-qwindowicon` icon

:   Set the default window icon.

`-qwindowtitle` title

:   Set the title of the first window.

`-session` session

:   Restore the application from an earlier session.

`-display` hostname:screen\_number

:   Switch displays on X11. Overrides the DISPLAY environment variable.

`-geometry` geometry

:   Specify the window geometry for the main window on X11. For example:
    -geometry 100x100+50+50

`-fontengine=` freetype

:   Use the FreeType font engine.

## Arguments

`file`
:   Input SMF/RMI/KAR/WRK file name.

# BUGS

See Tickets at Sourceforge <https://sourceforge.net/p/kmidimon/>

# SEE ALSO

**qt5options (7)**

# LICENSE

This manual page was written by Pedro Lopez-Cabanillas
<plcl@users.sourceforge.net> Permission is granted to copy, distribute
and/or modify this document under the terms of the GNU General Public
License, Version 2 or any later version published by the Free Software
Foundation; considering as source code all the file that enable the
production of this manpage.
