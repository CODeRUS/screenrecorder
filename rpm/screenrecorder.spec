%define theme sailfish-default
%define _binary_payload w2.xzdio

%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}

Name:       screenrecorder
Summary:    Sailfish screen recorder
Version:    0.5.4
Release:    2
Group:      System/GUI/Other
License:    GPLv2
URL:        https://github.com/coderus/screenrecorder
Source0:    %{name}-%{version}.tar.bz2
Requires:   sailfishsilica-qt5 >= 0.10.9a
Requires:   ffmpeg
Requires:   ffmpeg-tools

BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  desktop-file-utils
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Concurrent)
BuildRequires:  qt5-qtplatformsupport-devel
BuildRequires:  qt5-qtwayland-wayland_egl-devel
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  systemd
BuildRequires:  sailfish-svg2png


%description
Lipstick screenrecorder client

%if "%{?vendor}" == "chum"
PackageName: screenrecorder
Type: desktop-application
Categories:
 - Video
 - Graphics
PackagerName: Mark Washeim (poetaster)
Custom:
 - Repo: https://github.com/poetaster/screenrecorder
PackageIcon: https://raw.githubusercontent.com/poetaster/screenrecorder/master/icons/256x256/screenrecorder-gui.png
Url:
 - Bugtracker: https://github.com/poetaster/screenrecorder/issues
Links:
  Homepage: https://github.com/poetaster/screenrecorder
  Bugtracker: https://github.com/poetaster/screenrecorder/issues
  Donation: https://liberapay.com/poetaster
%endif

%prep
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5 \
  "PROJECT_PACKAGE_VERSION=%{version}" \
  SPEC_UNITDIR=%{_userunitdir}
%qtc_make %{_smp_mflags}

%install
rm -rf %{buildroot}
%qmake5_install

%post
systemctl-user start screenrecorder.service

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

%{_datadir}/dbus-1/services/org.coderus.screenrecorder.service
%{_userunitdir}/screenrecorder.service
%{_datadir}/dbus-1/system.d
%{_datadir}/jolla-settings/entries/screenrecorder.json
%{_datadir}/jolla-settings/pages/screenrecorder/ScreenrecorderToggle.qml
