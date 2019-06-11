%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}

Name:       screenrecorder
Summary:    Sailfish screen recorder
Version:    0.1.0
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
dbus-send --system --type=method_call --dest=org.freedesktop.DBus / org.freedesktop.DBus.ReloadConfig
systemctl stop screenrecorder.service

%preun
systemctl stop screenrecorder.service

%postun
systemctl stop screenrecorder.service


%files
%defattr(-,root,root,-)
%attr(755, root, privileged) %{_sbindir}/screenrecorder

%attr(755, root, root) %{_bindir}/screenrecorder-gui
%{_datadir}/screenrecorder-gui
%{_datadir}/applications/screenrecorder-gui.desktop
%{_datadir}/icons/hicolor/*/apps/screenrecorder-gui.png

%{_sysconfdir}/dbus-1/system.d/org.coderus.screenrecorder.conf
%{_datadir}/dbus-1/system-services/org.coderus.screenrecorder.service
/lib/systemd/system/screenrecorder.service
