%define theme sailfish-default

%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}

Name:       screenrecorder
Summary:    Sailfish screen recorder
Version:    0.2.1
Release:    1
Group:      System/GUI/Other
License:    GPLv2
URL:        https://github.com/coderus/screenrecorder
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  qt5-qtwayland-wayland_egl-devel
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  systemd
BuildRequires:  sailfish-svg2png

%description
Lipstick recorder client

%prep
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5 \
  "PROJECT_PACKAGE_VERSION=%{version}"
%qtc_make %{_smp_mflags}

%install
rm -rf %{buildroot}
%qmake5_install

%post
systemctl-user stop screenrecorder.service

%preun
systemctl-user stop screenrecorder.service

%postun
systemctl-user stop screenrecorder.service


%files
%defattr(-,root,root,-)
%attr(2755, root, privileged) %{_sbindir}/screenrecorder

%attr(755, root, root) %{_bindir}/screenrecorder-gui
%{_datadir}/screenrecorder-gui
%{_datadir}/applications/screenrecorder-gui.desktop
%{_datadir}/icons/hicolor/*/apps/screenrecorder-gui.png

%{_datadir}/themes/%{theme}/meegotouch/z1.0/icons/*.png
%{_datadir}/themes/%{theme}/meegotouch/z1.25/icons/*.png
%{_datadir}/themes/%{theme}/meegotouch/z1.5/icons/*.png
%{_datadir}/themes/%{theme}/meegotouch/z1.5-large/icons/*.png
%{_datadir}/themes/%{theme}/meegotouch/z1.75/icons/*.png
%{_datadir}/themes/%{theme}/meegotouch/z2.0/icons/*.png

%{_datadir}/dbus-1/services/org.coderus.screenrecorder.service
%{_libdir}/systemd/user/screenrecorder.service

%{_datadir}/jolla-settings/entries/screenrecorder.json
%{_datadir}/jolla-settings/pages/screenrecorder/ScreenrecorderToggle.qml
