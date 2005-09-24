%define name    kmidimon
%define version 0.4
%define release 1 

Name:           %{name}
Version:        %{version}
Release:        %mkrel %{release}
License:        GPL
Summary:        KDE MIDI Monitor for ALSA Sequencer
Group:          Sound
URL:            http://kmetronome.sourceforge.net/kmidimon/
Packager:       Pedro Lopez-Cabanillas
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-buildroot 
Source0:        %{name}-%{version}.tar.bz2

%description
KMidimon monitors events coming from a MIDI external port or application 
via the ALSA sequencer. It is especially useful if you want to debug MIDI 
software or your MIDI setup. It features a nice graphical user interface, 
customizable event filters and sequencer parameters, support for all MIDI 
messages and some ALSA messages, and saving the recorded event list to a 
text file.

%prep
%setup -q a 1

%build
%configure
%make

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall

install -d $RPM_BUILD_ROOT%{_menudir}
cat << EOF > $RPM_BUILD_ROOT%{_menudir}/%{name}
?package(%{name}):command="%{name}" \
icon="%{name}.png" needs="X11" section="Multimedia/Sound" \
title="KMidimon" longtitle="ALSA MIDI Monitor"
EOF

%clean
rm -rf $RPM_BUILD_ROOT

%post
%{update_menus}

%postun
%{clean_menus}

%files
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog NEWS README TODO
%doc %{_docdir}/HTML/en/%{name}/*
%{_bindir}/*
%{_datadir}/apps/%{name}/*
%{_datadir}/icons/*
%{_datadir}/applnk/*
%{_datadir}/locale/*
%{_menudir}/*

%changelog
* Sat Sep 24 2005 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net> 0.4
- Sources updated.

* Fri Sep 23 2005 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net> 0.3
- First package.
