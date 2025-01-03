% Help Index

# Introduction

[Drumstick MIDI Monitor](https://kmidimon.sourceforge.io) records events coming 
from a MIDI external port or application
via the ALSA sequencer, or stored as Standard MIDI Files. It is
especially useful if you want to debug MIDI software or your MIDI setup.
It features a nice graphical user interface, customizable event filters
and sequencer parameters, support for all MIDI messages and some ALSA
messages, and saving the recorded event list to a text file or SMF.

# Getting Started

## Main Window

The program starts in recording state, registering all incoming MIDI
events until you press the stop button. There are also buttons to play,
pause, rewind and forward, with the usual behavior of any other media
player.

Above the events list grid you can find a set of tabs, one for each
track defined in a SMF. You can add new tabs or close tabs without
losing the recorded events, because they are only views or event
filters.

You can control the ALSA sequencer MIDI connections to programs and
devices from inside Drumstick MIDI Monitor. To do so, use the options under the menu
"Connections" in the main menu. There are options to connect and
disconnect every available input port to Drumstick MIDI Monitor, and also a dialog box
where you can choose the ports to be monitored and the output one.

You can also use a MIDI connection tool like
[aconnect(1)](https://linux.die.net/man/1/aconnect)
or [QJackCtl](https://qjackctl.sourceforge.io) to connect the application
or MIDI port to Drumstick MIDI Monitor.

When a MIDI OUT port has been connected to the input port of Drumstick MIDI Monitor in
recording state, it will show incoming MIDI events if everything is
correct.

Each received MIDI event is shown in a single row. The columns have the
following meaning.

* **Ticks**: The musical time of the event arrival
* **Time**: The real time in seconds of the event arrival
* **Source**: the ALSA identifier of the MIDI device originating the
    event. You will be able to recognize what event belongs to which
    device if you have several connected simultaneously. There is a
    configuration option to show the ALSA client name instead of a
    number
* **Event Kind**: The event type: note on / off, control change, ALSA, and
    so on
* **Chan** The MIDI channel of the event (if it is a channel event). It
    is also used to show the Sysex channel.
* **Data 1**: It depends on the type of the received event. For a Control
    Change event or a Note, it is the control number, or the note number
* **Data 2**: It depends on the type of the received event. For a Control
    Change it will be the value, and for a Note event it will be the
    velocity
* **Data 3**: Text representation of System Exclusive or Meta Events.

You can hide or show any column using the contextual menu. To open this
menu, press the secondary mouse button over the event list. You can also
use the Configuration dialog to choose the visible columnns.

The event list always shows newer recorded events at the bottom of the
grid.

Drumstick MIDI Monitor can save the recorded events as a text file (in CSV format) or
a Standard MIDI File (SMF).

## Configuration Options 

To open the Configuration dialog go to Settings → Configure menu option
or click on the corresponding icon on the toolbar.

This is a list of the configuration options.

* **Sequencer tab**. The Queue Default Settings affect to the event's time
    precision.
* **Filters tab**. Here you can check several event families to be
    displayed in the event list.
* **Display tab**. The first group of checkboxes allows to show/hide the
    corresponding columns of the events list.
* **Misc. tab**. Miscellaneous options include:
    + Translate ALSA Client IDs into names. If checked, ALSA client
      names are used instead of ID numbers in the "Source" column for
      all king of events, and the data columns for the ALSA events.
    + Translate Universal System Exclusive messages. If checked,
      Universal System Exclusive messages (real time, non real time,
      MMC, MTC and a few other types) are interpreted and translated.
      Otherwise, the hexadecimal dump is shown.
    + Use fixed font. By default Drumstick MIDI Monitor uses the system font in the
      event list. If this option is checked, a fixed font is used
      instead.

# Credits and License

Program Copyright © 2005-2024 Pedro Lopez-Cabanillas

Documentation Copyright © 2005 Christoph Eckert

Documentation Copyright © 2008-2024 Pedro Lopez-Cabanillas

# Installation

## How to obtain Drumstick MIDI Monitor 

[Here](https://sourceforge.net/projects/kmidimon/files/)
you can find the last version. There is also a Git mirror at
[GitHub](https://github.com/pedrolcl/kmidimon)

## Requirements

In order to successfully use Drumstick MIDI Monitor, you need Qt 5, Drumstick 2
and ALSA drivers and library.

The build system requires [CMake](http://www.cmake.org) 3.14 or newer.

ALSA library, drivers and utilities can be found at
[ALSA home page](http://www.alsa-project.org)

You can find a list of changes at https://sourceforge.net/p/kmidimon

## Compilation and Installation

In order to compile and install Drumstick MIDI Monitor on your system, type the
following in the base directory of the Drumstick MIDI Monitor distribution:

    % cmake .
    % make
    % make install

Since Drumstick MIDI Monitor uses `cmake` and `make` you should have no trouble
compiling it. Should you run into problems please report them to the
author or the project's bug tracking system.
