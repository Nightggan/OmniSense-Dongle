%global app_name  ds5-audio-haptics-bt
%global gh_url    https://github.com/loteran/DS5Dongle
%global gh_raw    https://raw.githubusercontent.com/loteran/DS5Dongle/master

Name:           %{app_name}
Version:        0.3.0
Release:        1%{?dist}
Summary:        Configuration app for DS5Dongle audio haptics (DualSense BT dongle)
License:        MIT
URL:            %{gh_url}
# Sources are downloaded at build time (tar.gz binary, no compile step).
# Keep COPR chroots to x86_64 — the binary only supports that arch.

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
# Download and extract tar.gz at RPM build time.
# Requires --enable-net on in COPR (already set for this project).
mkdir -p %{buildroot}%{_prefix}/lib/%{app_name}
curl -fsSL --retry 3 \
    "%{gh_url}/releases/download/app-v%{version}/%{app_name}-%{version}-linux-x64.tar.gz" \
    | tar -xzf - --strip-components=1 \
    -C %{buildroot}%{_prefix}/lib/%{app_name}
chmod 755 %{buildroot}%{_prefix}/lib/%{app_name}/%{app_name}
chmod 4755 %{buildroot}%{_prefix}/lib/%{app_name}/chrome-sandbox

install -d %{buildroot}%{_bindir}
printf '#!/bin/sh\nexec %{_prefix}/lib/%{app_name}/%{app_name} "$@"\n' \
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
%{_prefix}/lib/%{app_name}/
%attr(4755,root,root) %{_prefix}/lib/%{app_name}/chrome-sandbox
%{_bindir}/%{app_name}
/usr/lib/udev/rules.d/70-ds5dongle.rules
%{_datadir}/applications/%{app_name}.desktop

%changelog
* Fri Jun 20 2026 loteran <axel.valadon@gmail.com> - 0.3.0-1
- Switch from AppImage to tar.gz distribution (no FUSE dependency)
- Install unpacked Electron binary to /usr/lib/ds5-audio-haptics-bt/
