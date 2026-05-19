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

Get the latest `.uf2` from the [Releases](https://github.com/loteran/DS5Dongle/releases) page:

| Board | File |
|-------|------|
| Raspberry Pi Pico 2 W | `ds5-bridge-picow-<version>.uf2` |
| Raspberry Pi Pico 2 (no W) | `ds5-bridge-<version>.uf2` |

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

**Persistent loopback (recommended — starts automatically with PipeWire):**

**Step 1 — PipeWire loopback config:**

```bash
mkdir -p ~/.config/pipewire/pipewire.conf.d
```

Create `~/.config/pipewire/pipewire.conf.d/20-ds5-haptics-loopback.conf`:

```ini
context.modules = [
  {
    name  = libpipewire-module-loopback
    flags = [ nofail ]
    args  = {
      node.description = "DS5 Haptics Capture"
      capture.props    = {
        node.name           = "ds5_haptics_capture"
        media.class         = "Stream/Input/Audio"
        stream.capture.sink = true
        target.object       = "@DEFAULT_AUDIO_SINK@"
        audio.position      = [ FL FR ]
        stream.dont-remix   = true
        node.passive        = true
        node.linger         = true
      }
      playback.props   = {
        node.name           = "ds5_haptics_playback"
        target.object       = "alsa_output.hw_Controller_0"
        audio.position      = [ AUX0 AUX1 ]
        stream.dont-remix   = true
        node.passive        = true
        node.linger         = true
        latency.msec        = 20
      }
    }
  }
]
```

`stream.capture.sink = true` taps the **monitor** of the default output (a read-only copy of whatever is playing) — your headset or speakers remain unchanged. `@DEFAULT_AUDIO_SINK@` follows dynamically when you switch output devices.

**Step 2 — apply:**

```bash
systemctl --user restart pipewire
```

**Step 3 — verify:**

```bash
pw-dump Node 2>/dev/null | python3 -c "
import sys,json
for n in json.load(sys.stdin):
    name = n.get('info',{}).get('props',{}).get('node.name','')
    if 'ds5' in name or 'Controller' in name: print(name)
"
```

You should see `alsa_output.hw_Controller_0`, `ds5_haptics_capture`, and `ds5_haptics_playback`.

---

**Optional — WirePlumber rule for extra stability (WirePlumber 0.5+ only):**

> **Check your version:** `wireplumber --version`  
> Skip this step if you are on WirePlumber 0.4.x (Ubuntu 24.04 LTS, Debian stable, etc.).

This step assigns the Pico a stable fixed name regardless of ALSA card index, and prevents the DualSense ACP audio profile from stealing your default output on reconnect. Without it everything still works, but on systems with multiple USB audio devices the card index in `hw_Controller_0` could occasionally differ.

```bash
mkdir -p ~/.config/wireplumber/wireplumber.conf.d
```

Create `~/.config/wireplumber/wireplumber.conf.d/51-ds5dongle.conf`:

```ini
monitor.alsa.rules = [
  {
    matches = [
      { alsa.components = "USB054c:0ce6"
        node.name       = "~alsa_output\\.hw_.*" }
      { alsa.components = "USB054c:0df2"
        node.name       = "~alsa_output\\.hw_.*" }
    ]
    actions = {
      update-props = {
        node.name = "ds5_dongle_sink"
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

Then update the loopback config: change `target.object = "alsa_output.hw_Controller_0"` to `target.object = "ds5_dongle_sink"` and apply:

```bash
systemctl --user restart wireplumber pipewire
```

**To remove the loopback:**

```bash
rm ~/.config/pipewire/pipewire.conf.d/20-ds5-haptics-loopback.conf
rm -f ~/.config/wireplumber/wireplumber.conf.d/51-ds5dongle.conf
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
