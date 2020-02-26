Introduction
============

There's no exhaustive documentation yet. The provisional homepage can
currently be found at <http://kmidimon.sourceforge.net>.

KMIDIMON monitors events coming from a MIDI external port or application
via the ALSA sequencer, or stored as Standard MIDI Files. It is
especially useful if you want to debug MIDI software or your MIDI setup.
It features a nice graphical user interface, customizable event filters
and sequencer parameters, support for all MIDI messages and some ALSA
messages, and saving the recorded event list to a text file or SMF.

Getting Started
===============

Main Window {#kmidimon-mainwin}
-----------

The program starts in recording state, registering all incoming MIDI
events until you press the stop button. There are also buttons to play,
pause, rewind and forward, with the usual behavior of any other media
player.

Above the events list grid you can find a set of tabs, one for each
track defined in a SMF. You can add new tabs or close tabs without
losing the recorded events, because they are only views or event
filters.

You can control the ALSA sequencer MIDI connections to programs and
devices from inside KMIDIMON. To do so, use the options under the menu
"Connections" in the main menu. There are options to connect and
disconnect every available input port to KMIDIMON, and also a dialog box
where you can choose the ports to be monitored and the output one.

You can also use a MIDI connection tool like
[QJackCtl](http://qjackctl.sourceforge.net/) to connect the application
or MIDI port to monitor with KMIDIMON

Here's a screenshot of the MIDI Connections window in `qjackctl` MIDI
Connections in qjackctl

When a MIDI OUT port has been connected to the input port of KMIDIMON in
recording state, it will show incoming MIDI events if everything is
correct.

Here's a screenshot of the main window of KMIDIMON with some MIDI events
in it MIDI events in the main window of KMIDIMON

Each received MIDI event is shown in a single row. The columns have the
following meaning.

-   Ticks: The musical time of the event arrival

-   Time: The real time in seconds of the event arrival

-   Source: the ALSA identifier of the MIDI device originating the
    event. You will be able to recognize what event belongs to which
    device if you have several connected simultaneously. There is a
    configuration option to show the ALSA client name instead of a
    number

-   Event Kind: The event type: note on / off, control change, ALSA, and
    so on

-   Chan: The MIDI channel of the event (if it is a channel event). It
    is also used to show the Sysex channel.

-   Data 1: It depends on the type of the received event. For a Control
    Change event or a Note, it is the control number, or the note number

-   Data 2: It depends on the type of the received event. For a Control
    Change it will be the value, and for a Note event it will be the
    velocity

You can hide or show any column using the contextual menu. To open this
menu, press the secondary mouse button over the event list. You can also
use the Configuration dialog to choose the visible columnns.

The event list always shows newer recorded events at the bottom of the
grid.

KMIDIMON can save the recorded events as a text file (in CSV format) or
a Standard MIDI File (SMF).

Configuration Options {#kmidimon-configuration}
---------------------

Here's a screenshot of the Configuration dialog of KMIDIMON Display
settings tab Here's another screenshot of the Configuration dialog of
KMIDIMON Filter settings tab

To open the Configuration dialog go to Settings -&gt; Configure KMIDIMON
or click on the corresponding icon on the toolbar.

This is a list of the configuration options.

-   Sequencer tab. The Queue Default Settings affect to the event's time
    precision.

-   Filters tab. Here you can check several event families to be
    displayed in the event list.

-   Display tab. The first group of checkboxes allows to show/hide the
    corresponding columns of the events list.

-   Misc. tab. Miscellaneous options include:

    -   Translate ALSA Client IDs into names. If checked, ALSA client
        names are used instead of ID numbers in the "Source" column for
        all king of events, and the data columns for the ALSA events.

    -   Translate Universal System Exclusive messages. If checked,
        Universal System Exclusive messages (real time, non real time,
        MMC, MTC and a few other types) are interpreted and translated.
        Otherwise, the hexadecimal dump is shown.

    -   Use fixed font. By default KMIDIMON uses the system font in the
        event list. If this option is checked, a fixed font is used
        instead.

Credits and License {#credits}
===================

Program copyright 2005-2009 Pedro Lopez-Cabanillas
<plcl@users.sourceforge.net>

Documentation copyright 2005 Christoph Eckert
<christeck@users.sourceforge.net>

Documentation copyright 2008-2009 Pedro Lopez-Cabanillas
<plcl@users.sourceforge.net>

UNDERFDL UNDERGPL
Installation
============

How to obtain KMIDIMON {#getting-kmidimon}
----------------------

Here you can find the last version: [Project home
page](http://sourceforge.net/projects/kmidimon)

Requirements
------------

In order to successfully use KMIDIMON, you need KDE 4.X. and ALSA
drivers and library.

The build system requires [CMake](http://www.cmake.org) 2.4.4 or newer.

ALSA library, drivers and utilities can be found at [ALSA home
page](http://www.alsa-project.org).

You can find a list of changes at <http://kmidimon.sourceforge.net>

Compilation and Installation {#compilation}
----------------------------

In order to compile and install KMIDIMON on your system, type the
following in the base directory of the KMIDIMON distribution:

    % cmake .
    % make
    % make install

Since KMIDIMON uses `cmake` and `make` you should have no trouble
compiling it. Should you run into problems please report them to the
author or the project's bug tracking system.

DOCUMENTATION.INDEX
