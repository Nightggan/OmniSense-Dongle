# DS5Dongle — Auto Haptics Edition

> **Fork of [awalol/DS5Dongle](https://github.com/awalol/DS5Dongle)**  
> Adds **Audio Auto Haptics**: the Raspberry Pi Pico 2 W analyzes game audio in real time and generates DualSense rumble — even when the game sends no haptic commands.

---

## What is DS5Dongle?

DS5Dongle turns a **Raspberry Pi Pico 2 W** into a Bluetooth adapter for the **Sony DualSense (PS5) controller** on PC.

Without it, connecting the DualSense over Bluetooth on PC means losing:
- HD haptics (the voice-coil actuators)
- The internal speaker
- Adaptive triggers (partially)

The Pico bridges the gap: it presents itself to the PC as a **native DualSense USB device** (same VID/PID, same HID descriptors, 4-channel USB Audio), then forwards everything wirelessly to the controller over **Bluetooth Classic (L2CAP)**.

```
[PC / Linux / Windows]
        │
        ▼  USB  (HID gamepad + 4ch USB Audio)
[ Raspberry Pi Pico 2 W ]
        │
        ▼  Bluetooth Classic (L2CAP)
[ DualSense controller ]
```

- **ch 1/2** → Game audio → Opus 160 kbps → DualSense internal speaker
- **ch 3/4** → Native haptics stream → resampled 48k→3 kHz PCM → HD actuators
- **HID output 0x02** → Rumble motors + adaptive triggers → forwarded as BT report 0x31

---

## What this fork adds — Audio Auto Haptics

Most PC games (including Linux/Proton titles) never send haptic data, even for games that support rumble on PS5.  
This fork adds a **DSP pipeline inside the firmware** that derives haptic signals directly from the game audio:

```
Game audio (ch 1/2, 48 kHz stereo)
    │
    ├── 1-pole low-pass filter  (80 / 160 / 250 / 400 Hz selectable)
    │       → keeps only the bass felt by DualSense actuators
    │
    ├── Envelope follower  (attack ~1 ms / release ~80 ms)
    │       → fast attack captures gunshots, footsteps, impacts
    │       → slow release gives body to explosions
    │
    ├── Modulation: bass × (1 + 3 × envelope)
    │       → creates a "thumping" waveform
    │
    ├── Soft-clip: x / (1 + |x|)   ← tanh approximation, no tanhf cost
    │
    └── Injected into the haptics bus before 48k→3k resampling
```

Three modes available:

| Mode | Behaviour |
|------|-----------|
| **0 — Off** | Original behaviour: only native ch3/ch4 haptics pass through |
| **1 — Mix** | Native haptics **+** audio-derived signal blended together |
| **2 — Replace** *(default)* | Audio-derived only — works even when the game sends nothing |

Works with any game, any OS, without any software on the PC side.

---

## Hardware required

| Item | Notes |
|------|-------|
| **Raspberry Pi Pico 2 W** | RP2350 + CYW43439 BT chip. Pico W (RP2040) compiles but has no audio. |
| USB-A to micro-USB cable | To connect the Pico to your PC |
| DualSense controller | Pair once following the [original pairing guide](https://github.com/awalol/DS5Dongle#pairing) |

---

## Installation

### 1. Download the pre-built firmware

Get `ds5-bridge-autohaptics.uf2` from the [Releases](https://github.com/loteran/DS5Dongle/releases) page.

### 2. Flash the Pico

1. Hold **BOOTSEL** while plugging the Pico into USB → it mounts as **`RP2350`**
2. Copy the UF2 onto it:

```bash
cp ds5-bridge-autohaptics.uf2 /run/media/$USER/RP2350/
```

The Pico reboots automatically. Done.

### 3. Set your audio output

#### Linux
```bash
# The Pico appears as a 4-channel USB sound card — set it as default output
wpctl set-default $(wpctl status | grep "DS5 Bridge" | awk '{print $2}' | tr -d '.')
```
Or use `pavucontrol` / system sound settings to select **DS5 Bridge** as output.

#### Windows
Open **Sound settings** → set **DS5 Bridge** as the default playback device.

---

## Configuration

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
| **Low-pass cutoff** | 0=80Hz · 1=160Hz · 2=250Hz · 3=400Hz | 1 (160 Hz) |

**Tuning tips:**
- Start with **Replace + 160 Hz + 100%** (defaults)
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

## All configuration parameters

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
| `auto_haptics_lowpass` | 0 / 1 / 2 / 3 | 1 | LP cutoff: 80/160/250/400 Hz |

---

## Building from source

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

## Technical notes

- DSP runs on **Core 0** inside `audio_loop()` — no new threads, no queues added
- All state is `static` (16 bytes: 2× LP + 2× envelope)
- Cost: ~6 multiply-adds per sample + 1 division — negligible at 320 MHz
- `x / (1 + |x|)` avoids `tanhf()` which is expensive on Cortex-M33
- Classic rumble goes through a separate path (`tud_hid_set_report_cb` → BT report `0x31`) — unaffected
- Config stored in last flash sector (4 kB), validated by magic header + CRC32
- **TinyUSB must be 0.20.0** — the version bundled in Pico SDK 2.2.0 is incompatible

---

## Credits

- **[awalol](https://github.com/awalol/DS5Dongle)** — original DS5Dongle firmware
- **[egormanga/SAxense](https://github.com/egormanga/SAxense)** — DualSense BT haptics proof of concept
- **[nondebug/dualsense](https://github.com/nondebug/dualsense)** — DualSense protocol reverse engineering
- **Cockos WDL** — resampler · **xiph/opus** — audio codec

---

## License

MIT — see [LICENSE](LICENSE)
