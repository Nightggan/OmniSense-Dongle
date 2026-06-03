# DS5Dongle — Auto Haptics Edition

> **Fork of [awalol/DS5Dongle](https://github.com/awalol/DS5Dongle)**  
> Adds **Audio Auto Haptics**: your DualSense vibrates in sync with the game's sounds — footsteps, gunshots, explosions — even when the game has no haptic support.

![Hardware](https://img.shields.io/badge/hardware-Raspberry%20Pi%20Pico%202%20W-c51a4a?logo=raspberrypi&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-blue?logo=linux&logoColor=white)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Table of Contents

- [What does this do?](#-what-does-this-do)
  - [The problem](#the-problem)
  - [The solution — a $20 bridge](#the-solution--a-20-bridge)
  - [What this fork adds](#what-this-fork-adds--audio-auto-haptics)
- [How it works internally](#-how-it-works-internally)
- [Hardware required](#-hardware-required)
- [Installation](#-installation)
  - [1. Download the firmware](#1-download-the-pre-built-firmware)
  - [2. Flash the Pico](#2-flash-the-pico)
  - [3. Route audio to the Pico](#3-route-audio-to-the-pico)
    - [Linux (PipeWire)](#linux-pipewire)
    - [Windows](#windows)
- [Configuration](#-configuration)
  - [Web app](#web-app--recommended)
  - [Desktop app (Flatpak)](#desktop-app-flatpak)
  - [Auto Haptics settings](#auto-haptics-settings)
  - [Python CLI](#python-script-cli--no-chrome)
- [All configuration parameters](#-all-configuration-parameters)
- [Building from source](#-building-from-source)
- [Technical notes](#-technical-notes)
- [Credits](#-credits)

---

## 🎮 What does this do?

### The problem

The Sony DualSense controller has advanced features that only work properly when connected **by USB cable** on PC: the HD haptics (the actuators that make you feel every texture), the internal speaker, and the adaptive triggers. Connect it via Bluetooth and most of these are lost — the operating system simply doesn't know how to send that data wirelessly.

On top of that, even with a cable, **most PC games never send haptic commands at all** — including many games that do vibrate on PS5. The game just doesn't bother implementing it for PC.

### The solution — a $20 bridge

DS5Dongle is a firmware for the **Raspberry Pi Pico 2 W** (~$7 board) that acts as a smart bridge between your PC and your DualSense:

```
Your PC
  │
  │  USB cable  (the Pico looks exactly like a DualSense plugged in by cable)
  ▼
Raspberry Pi Pico 2 W        ← this is the "dongle"
  │
  │  Bluetooth
  ▼
DualSense controller         ← wireless, on your couch
```

Your PC thinks it has a DualSense connected by USB. In reality it's talking to the Pico, which relays everything to the controller wirelessly. You get all the features, without the cable to the controller.

### What this fork adds — Audio Auto Haptics

This fork goes further: the Pico **listens to the game's audio** and **generates vibrations from the sound**, in real time, with no software needed on your PC.

| Sound | Haptic feel |
|-------|-------------|
| Horse galloping | Rhythmic hoofbeat pulses |
| Gunshot | Sharp impact in your hands |
| Explosion | Deep rumble that fades out |
| Car engine | Constant low-frequency buzz changing with RPM |

This works even if the game has **zero haptic support**. The Pico extracts the bass and impact sounds from the audio stream and converts them into motor signals, entirely by itself.

> **Compared to the PS5 experience**, this is not identical — the PS5 has access to precise per-object haptic data from the game engine. What this does is a smart approximation from audio alone, similar to what apps like DSX do on Windows. In practice it adds a lot of immersion to games that would otherwise have no feedback at all.

---

## ⚙️ How it works internally

The Pico receives the game audio over USB (it appears as a stereo USB sound card — 2 channels, 48 kHz). It then runs this signal through a small DSP chain entirely in firmware, with no CPU overhead on your PC:

```
Game audio (stereo, 48 kHz)
    │
    ├── Low-pass filter (selectable: 80 / 160 / 250 / 400 Hz)
    │       → isolates the bass frequencies the actuators can reproduce
    │
    ├── Envelope follower  (attack 1 ms / release 80 ms)
    │       → detects sudden impacts and gives them extra punch
    │
    ├── Waveform shaping
    │       → turns the filtered signal into a physically convincing rumble
    │
    └── Sent to the DualSense actuators via Bluetooth
```

Three modes let you control how the auto haptics interact with games that do send native haptic data:

| Mode | Behaviour |
|------|-----------|
| **0 — Off** | Original behaviour: only native haptics from the game pass through |
| **1 — Mix** | Native game haptics **+** audio-derived signal at the same time |
| **2 — Replace** *(default)* | Audio-derived signal only — best for games with no haptic support |

Classic rumble (games that do send vibration commands via DirectInput/SDL) works normally alongside the auto haptics — they go through a completely separate path and are unaffected.

---

## 🔧 Hardware required

| Item | Notes |
|------|-------|
| **Raspberry Pi Pico 2 W** | RP2350 + CYW43439 BT chip. Pico W (RP2040) compiles but has no audio. |
| USB-A to micro-USB cable | To connect the Pico to your PC |
| DualSense controller | Pair once following the [original pairing guide](https://github.com/awalol/DS5Dongle#pairing) |

---

## 🚀 Installation

### 1. Download the pre-built firmware

Get the latest `ds5-bridge-<version>.uf2` from the [Releases](https://github.com/loteran/DS5Dongle/releases) page.

> This fork requires a **Raspberry Pi Pico 2 W** (RP2350). Audio processing is mandatory for auto-haptics, so there is no Pico W (RP2040) build.

### 2. Flash the Pico

1. Hold **BOOTSEL** while plugging the Pico into USB → it mounts as **`RP2350`**
2. Copy the UF2 onto it:

```bash
cp ds5-bridge-picow-<version>.uf2 /run/media/$USER/RP2350/
```

The Pico reboots automatically. Done.

### 3. Route audio to the Pico

The Pico needs to receive the game audio to generate haptics. **Do not set it as your default output** — your headset or speakers should remain the default. Instead, create a loopback that sends a silent copy of your audio to the Pico in the background.

---

#### Linux (PipeWire)

The loopback taps the **monitor of your current default output** — whatever it is (headset, speakers, DAC, HDMI TV…). Your default output is never changed.

The loopback is tied to the dongle's presence: it is created **only while the dongle is plugged in** (started/stopped by udev), so its playback target always exists. This avoids an audio **feedback loop** — if a permanently-loaded loopback can't find the dongle it falls back to your speakers and pipes their monitor straight back into them.

**Step 1 — name the dongle sink (WirePlumber rule):**

This gives the Pico a stable name (`ds5_dongle_sink`) regardless of ALSA card index, and prevents the DualSense ACP audio profile from stealing your default output on reconnect.

```bash
mkdir -p ~/.config/wireplumber/wireplumber.conf.d
```

Create `~/.config/wireplumber/wireplumber.conf.d/51-ds5dongle.conf`:

```ini
monitor.alsa.rules = [
  {
    matches = [
      { alsa.components = "USB054c:0ce6"
        media.class     = "Audio/Sink" }
      { alsa.components = "USB054c:0df2"
        media.class     = "Audio/Sink" }
    ]
    actions = {
      update-props = {
        node.name        = "ds5_dongle_sink"
        priority.session = 0
      }
    }
  }
  {
    matches = [
      { alsa.components = "USB054c:0ce6"
        node.name       = "~alsa_output\\.usb-.*" }
      { alsa.components = "USB054c:0df2"
        node.name       = "~alsa_output\\.usb-.*" }
    ]
    actions = {
      update-props = {
        priority.session = 0
      }
    }
  }
]
```

> **Match by ALSA components + media class, not node name.** Recent PipeWire/WirePlumber name the dongle sink `alsa_output.usb-…pro-output-0` (or `…Direct__Direct__sink`), not the legacy `alsa_output.hw_Controller_0`. Matching on `alsa.components` + `media.class` renames it regardless of profile or PipeWire version.

> **Use the `pro-audio` card profile for the dongle.** The auto-haptics need the raw 4-channel sink with `AUX0…AUX3` positions. The `Direct` profile exposes `FL/FR/RL/RR` instead, which the `[AUX0,AUX1]` loopback mapping can't target. Set it once with:
> ```bash
> pactl set-card-profile alsa_card.usb-Sony_Interactive_Entertainment_DualSense_Wireless_Controller-00 pro-audio
> ```
> WirePlumber remembers the choice. (If you ever reset its state, re-apply it.)

**Step 2 — install the loopback service + udev rule:**

These files ship in [`config-app/`](config-app/). Install them (the package does this automatically):

```bash
# systemd user service that runs the loopback (pw-loopback) while active
install -Dm644 config-app/ds5-haptics-loopback.service ~/.config/systemd/user/ds5-haptics-loopback.service

# udev: start it on plug, stop it on unplug  (root)
sudo install -Dm644 config-app/70-ds5dongle.rules        /etc/udev/rules.d/70-ds5dongle.rules
sudo install -Dm755 config-app/ds5dongle-loopback-stop   /usr/lib/ds5dongle/ds5dongle-loopback-stop

systemctl --user daemon-reload
sudo udevadm control --reload-rules
```

The service captures the **monitor of your current default output** (`@DEFAULT_AUDIO_SINK@`, a read-only copy — your headset/speakers are unchanged) and pipes it to `ds5_dongle_sink`. Two safeguards prevent feedback:

- `ExecStartPre` refuses to start unless `ds5_dongle_sink` exists.
- `node.dont-reconnect=true` keeps the playback disconnected (instead of falling back to the speakers) if the dongle disappears at runtime.

**Step 3 — verify (with the dongle plugged in):**

```bash
systemctl --user is-active ds5-haptics-loopback.service   # -> active
pw-link -lo | grep -A2 ds5_haptics_playback               # -> linked to ds5_dongle_sink only
```

The playback must be linked **only** to `ds5_dongle_sink`, never to your speakers/HDMI.

**To remove:**

```bash
systemctl --user stop ds5-haptics-loopback.service
rm ~/.config/systemd/user/ds5-haptics-loopback.service
sudo rm -f /etc/udev/rules.d/70-ds5dongle.rules /usr/lib/ds5dongle/ds5dongle-loopback-stop
rm -f ~/.config/wireplumber/wireplumber.conf.d/51-ds5dongle.conf
systemctl --user daemon-reload && sudo udevadm control --reload-rules
systemctl --user restart wireplumber pipewire
```

**Per-game routing (Steam / Proton):**

To send only one game's audio to the Pico instead of your whole system output:

1. Open `pavucontrol` → **Playback** tab while the game is running
2. Find the game's audio stream
3. Change its output to **DS5 Bridge**

Everything else keeps playing through your normal output.

---

#### Windows

The Pico appears in Windows as a USB audio device. Windows has no native way to send audio to two outputs simultaneously — use **VoiceMeeter Banana** (free) to duplicate the stream to both your headset and the Pico.

**VoiceMeeter Banana setup:**

1. Download and install **[VoiceMeeter Banana](https://vb-audio.com/Voicemeeter/banana.htm)**, then **restart your PC**
2. In Windows Sound settings → set **VoiceMeeter Input** as the default playback device
3. Open VoiceMeeter Banana:
   - **Hardware Out A1** → your headset or speakers
   - **Hardware Out A2** → **DualSense Wireless Controller** (the Pico)
4. On the **VoiceMeeter VAIO** input strip → enable both **A1** and **A2** (both buttons lit)

All audio now goes to your headset **and** the Pico at the same time.

---

## 🎛️ Configuration

### Web app — recommended

Open **[DS5 Bridge Config](https://loteran.github.io/ds5dongle-config/)** in **Chrome or Edge** (WebHID — Firefox not supported).

1. Click **Connect** → select the DS5Dongle from the browser dialog
2. Config is read automatically from the device
3. Adjust settings
4. Click **Save to Device** — written to flash, persists across reboots

### Desktop app (Flatpak)

A native desktop app (`config-app/`) is also available — no browser required, works on
any Linux distribution. It exposes every setting from the web app plus **named presets**
(save/load/delete your favourite configurations).

**Install from the bundled `.flatpak`** (attached to each [release](https://github.com/loteran/DS5Dongle/releases)):

```bash
# 1. KDE runtime (one time — skip if already installed)
flatpak install flathub org.kde.Platform//6.8

# 2. Install the app
flatpak install ./DS5DongleConfig.flatpak

# 3. Launch
flatpak run com.github.loteran.DS5DongleConfig
```

It also appears in your application menu as **DS5Dongle Config**.

> **HID permissions:** the app needs read/write access to the Pico over USB HID. The
> Flatpak ships with the `--device=all` permission so this works out of the box. If you
> run the app *outside* Flatpak (e.g. `python3 config-app/main.py`), install the udev rule
> first so your user can reach `/dev/hidraw*`:
>
> ```bash
> sudo cp config-app/70-ds5dongle.rules /etc/udev/rules.d/
> sudo udevadm control --reload-rules && sudo udevadm trigger --subsystem-match=hidraw
> # then replug the Pico
> ```

**Presets** are stored as JSON in `~/.config/ds5dongle/presets/`. Use **Save As…** to
capture the current settings, **Load** to recall them into the form (then click **Save to
Device** to apply), and **Delete** to remove one.

### Controller shortcuts

These work at any time without opening the config page:

| Shortcut | Action |
|----------|--------|
| **PS + Triangle** | Power off the controller |
| **PS + Circle** | Toggle touchpad on/off (runtime only, not saved to flash) |

Both can be enabled/disabled from the config page.

### Auto Haptics settings

| Setting | Description | Default |
|---------|-------------|---------|
| **Mode** | 0=off · 1=mix · 2=replace | 2 |
| **Intensity** | Strength as % of Haptics Gain | 100% |
| **Low-pass cutoff** | 0=80Hz · 1=160Hz · 2=250Hz · 3=400Hz | 0 (80 Hz) |

**Tuning tips:**
- Start with **Replace + 80 Hz + 100%** (defaults)
- Increase intensity if vibrations feel too weak
- Use **80 Hz** for heavy bass feel (racing, explosions)
- Use **250–400 Hz** for more detail (FPS — footsteps, reloads)
- Use **Mix** if the game already sends haptics and you want both

### Python script (CLI / no Chrome)

```bash
pip install hidapi

# Read config
python3 scripts/set_ds5.py

# Enable auto haptics — replace mode, 160 Hz, 120% intensity
python3 scripts/set_ds5.py --auto-haptics-enable 2 --auto-haptics-gain 120 --auto-haptics-lowpass 1

# Disable auto haptics
python3 scripts/set_ds5.py --auto-haptics-enable 0

# All options
python3 scripts/set_ds5.py --help
```

---

## 📋 All configuration parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| `haptics_gain` | 1.0 – 2.0 | 1.0 | Global haptics amplitude multiplier |
| `speaker_volume` | -100 – 0 dB | -100 | DualSense internal speaker volume |
| `audio_buffer_length` | 16 – 128 | 64 | Haptics PCM packet size (lower = less latency) |
| `inactive_time` | 5 – 60 min | 30 | Auto-disconnect delay |
| `disable_inactive_disconnect` | 0 / 1 | 0 | 1 = always stay connected |
| `disable_pico_led` | 0 / 1 | 0 | 1 = turn off Pico LED |
| `polling_rate_mode` | 0 / 1 / 2 | 0 | 0=250Hz · 1=500Hz · 2=1000Hz |
| `controller_mode` | 0 / 1 / 2 | 2 | 0=DS5 · 1=DSE Edge · 2=Auto |
| `auto_haptics_enable` | 0 / 1 / 2 | 2 | Auto haptics mode |
| `auto_haptics_gain` | 0 – 200% | 100 | Auto haptics intensity |
| `auto_haptics_lowpass` | 0 / 1 / 2 / 3 | 0 | LP cutoff: 80/160/250/400 Hz |
| `enable_poweroff_shortcut` | 0 / 1 | 1 | 1 = PS+Triangle powers off the controller |
| `enable_touchpad` | 0 / 1 | 1 | Touchpad default state on connect (PS+Circle toggles runtime) |

---

## 🛠️ Building from source

```bash
# Arch / CachyOS
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib ninja cmake

# Clone with submodules
git clone --recurse-submodules https://github.com/loteran/DS5Dongle.git
cd DS5Dongle

# Pico SDK 2.2.0
git clone --depth 1 --branch 2.2.0 https://github.com/raspberrypi/pico-sdk.git /tmp/pico-sdk
git -C /tmp/pico-sdk submodule update --init --recursive

# TinyUSB must be exactly 0.20.0
git -C /tmp/pico-sdk/lib/tinyusb fetch --depth 1 origin refs/tags/0.20.0:refs/tags/0.20.0
git -C /tmp/pico-sdk/lib/tinyusb checkout --detach 0.20.0

# Configure & build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DPICO_SDK_PATH=/tmp/pico-sdk
ninja -C build ds5-bridge

# Output
ls build/ds5-bridge.uf2
```

---

## 📌 Technical notes

- DSP runs on **Core 0** inside `audio_loop()` — no new threads, no queues added
- All state is `static` (16 bytes: 2× LP + 2× envelope)
- Cost: ~6 multiply-adds per sample + 1 division — negligible at 320 MHz
- `x / (1 + |x|)` avoids `tanhf()` which is expensive on Cortex-M33
- Classic rumble goes through a separate path (`tud_hid_set_report_cb` → BT report `0x31`) — unaffected
- Config stored in last flash sector (4 kB), validated by magic header + CRC32
- **TinyUSB must be 0.20.0** — the version bundled in Pico SDK 2.2.0 is incompatible

---

## 🙏 Credits

- **[awalol](https://github.com/awalol/DS5Dongle)** — original DS5Dongle firmware
- **[egormanga/SAxense](https://github.com/egormanga/SAxense)** — DualSense BT haptics proof of concept
- **[nondebug/dualsense](https://github.com/nondebug/dualsense)** — DualSense protocol reverse engineering
- **Cockos WDL** — resampler · **xiph/opus** — audio codec

---

## License

MIT — see [LICENSE](LICENSE)
