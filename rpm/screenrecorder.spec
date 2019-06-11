%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}

Name:       screenrecorder
Summary:    Sailfish screen recorder
Version:    0.0.1
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

%files
%defattr(-,root,root,-)
%attr(755, root, privileged) %{_sbindir}/screenrecorder
