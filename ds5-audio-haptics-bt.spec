%global app_name  ds5-audio-haptics-bt
%global gh_url    https://github.com/loteran/DS5Dongle
%global gh_raw    https://raw.githubusercontent.com/loteran/DS5Dongle/master

Name:           %{app_name}
Version:        0.3.0
Release:        1%{?dist}
Summary:        Configuration app for DS5Dongle audio haptics (DualSense BT dongle)
License:        MIT
URL:            %{gh_url}
# Sources are downloaded at build time (AppImage binary, no compile step).
# Keep COPR chroots to x86_64 — the AppImage only supports that arch.

Requires:       fuse-libs
Requires:       libusb1

BuildRequires:  curl

%description
Cross-platform configuration app for DS5Dongle, a Raspberry Pi Pico-based
DualSense (PS5 controller) audio-haptics dongle for Linux and Windows.

Configures haptics gain, auto-haptics audio-reactive intensity, LED
lightbar colour, Bluetooth power mode, and more via USB HID.

%prep
# Nothing to extract.

%build
# Nothing to compile.

%install
# Download AppImage and supporting files at RPM build time.
# Requires --enable-net on in COPR (already set for this project).
mkdir -p %{buildroot}%{_prefix}/lib/%{app_name}
curl -fsSL --retry 3 \
    "%{gh_url}/releases/download/app-v%{version}/%{app_name}-%{version}.AppImage" \
    -o %{buildroot}%{_prefix}/lib/%{app_name}/%{app_name}.AppImage
chmod 755 %{buildroot}%{_prefix}/lib/%{app_name}/%{app_name}.AppImage

install -d %{buildroot}%{_bindir}
printf '#!/bin/sh\nexec %{_prefix}/lib/%{app_name}/%{app_name}.AppImage "$@"\n' \
    > %{buildroot}%{_bindir}/%{app_name}
chmod 755 %{buildroot}%{_bindir}/%{app_name}

install -d %{buildroot}/usr/lib/udev/rules.d
curl -fsSL --retry 3 \
    "%{gh_raw}/config-app/70-ds5dongle.rules" \
    -o %{buildroot}/usr/lib/udev/rules.d/70-ds5dongle.rules
chmod 644 %{buildroot}/usr/lib/udev/rules.d/70-ds5dongle.rules

install -d %{buildroot}%{_datadir}/applications
curl -fsSL --retry 3 \
    "%{gh_raw}/config-app/%{app_name}.desktop" \
    -o %{buildroot}%{_datadir}/applications/%{app_name}.desktop
chmod 644 %{buildroot}%{_datadir}/applications/%{app_name}.desktop

%post
udevadm control --reload-rules 2>/dev/null || true
udevadm trigger --subsystem-match=usb --attr-match=idVendor=054c 2>/dev/null || true

%postun
udevadm control --reload-rules 2>/dev/null || true

%files
%{_prefix}/lib/%{app_name}/%{app_name}.AppImage
%{_bindir}/%{app_name}
/usr/lib/udev/rules.d/70-ds5dongle.rules
%{_datadir}/applications/%{app_name}.desktop

%changelog
* Thu Jun 19 2026 loteran <axel.valadon@gmail.com> - 0.3.0-1
- Initial Fedora/COPR package
- Wraps AppImage with udev rules and .desktop launcher entry
