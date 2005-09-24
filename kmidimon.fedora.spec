Summary: ALSA MIDI monitor
Name: kmidimon
Version: 0.4
Release: 1
License: GPL
Group: Applications/Multimedia
URL: http://kmetronome.sourceforge.net/kmidimon/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
BuildRequires: gcc-c++, kdelibs-devel, alsa-lib-devel

%description
MIDI monitor for Linux using ALSA sequencer and KDE user interface.

%prep
%setup -q

%build
%configure
%make

%install
%{__rm} -rf %{buildroot}
%makeinstall

%{__cat} << EOF > %{name}.desktop
[Desktop Entry]
Name=Kmidimon
Comment=ALSA MIDI monitor
Icon=kmidimon
Exec=%{_bindir}/%{name}
Terminal=false
Type=Application
EOF

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-,root,root,-)
%doc AUTHORS ChangeLog COPYING INSTALL NEWS README TODO
%{_bindir}/kmidimon
%exclude %{_datadir}/applnk/Utilities/kmidimon.desktop
%{_datadir}/apps/kmidimon/kmidimonui.rc
%{_datadir}/doc/HTML/en/kmidimon
%{_datadir}/icons/hicolor/*/actions/kmidimon_record.png
%{_datadir}/icons/hicolor/*/apps/kmidimon.png
%{_datadir}/applications/%{desktop_vendor}-%{name}.desktop

%changelog
* Wed May  4 2005 Fernando Lopez-Lezcano <nando@ccrma.stanford.edu> 0.1-1
- initial build.
