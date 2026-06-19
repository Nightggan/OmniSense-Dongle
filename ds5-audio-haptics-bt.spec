Name:           ds5-audio-haptics-bt
Version:        0.3.0
Release:        1%{?dist}
Summary:        Configuration app for DS5Dongle audio haptics (DualSense BT dongle)
License:        MIT
URL:            https://github.com/loteran/DS5Dongle
ExclusiveArch:  x86_64

# The Electron app is distributed as an AppImage; the RPM wraps it.
Source0:        https://github.com/loteran/DS5Dongle/releases/download/app-v%{version}/%{name}-%{version}.AppImage
Source1:        https://raw.githubusercontent.com/loteran/DS5Dongle/master/config-app/70-ds5dongle.rules
Source2:        https://raw.githubusercontent.com/loteran/DS5Dongle/master/config-app/%{name}.desktop

Requires:       fuse-libs
Requires:       libusb1

%description
Cross-platform configuration app for DS5Dongle, a Raspberry Pi Pico-based
DualSense (PS5 controller) audio-haptics dongle for Linux and Windows.

The app lets you configure haptics gain, auto-haptics audio-reactive
intensity, LED lightbar colour, Bluetooth power, and more via USB HID.

%prep
# Nothing to extract — binary AppImage, plain-text udev rules and .desktop.

%build
# Nothing to compile.

%install
install -Dm755 %{SOURCE0} %{buildroot}%{_prefix}/lib/%{name}/%{name}.AppImage

install -Dm644 %{SOURCE1} \
    %{buildroot}%{_udevrulesdir}/70-ds5dongle.rules

install -Dm644 %{SOURCE2} \
    %{buildroot}%{_datadir}/applications/%{name}.desktop

# Thin launcher wrapper so the app is on PATH
install -d %{buildroot}%{_bindir}
printf '#!/bin/sh\nexec %{_prefix}/lib/%{name}/%{name}.AppImage "$@"\n' \
    > %{buildroot}%{_bindir}/%{name}
chmod 755 %{buildroot}%{_bindir}/%{name}

%post
udevadm control --reload-rules 2>/dev/null || true
udevadm trigger --subsystem-match=usb --attr-match=idVendor=054c 2>/dev/null || true

%postun
udevadm control --reload-rules 2>/dev/null || true

%files
%{_prefix}/lib/%{name}/%{name}.AppImage
%{_bindir}/%{name}
%{_udevrulesdir}/70-ds5dongle.rules
%{_datadir}/applications/%{name}.desktop

%changelog
* Thu Jun 19 2026 loteran <axel.valadon@gmail.com> - 0.3.0-1
- Initial Fedora/COPR package
- Wraps AppImage with udev rules and .desktop launcher entry
