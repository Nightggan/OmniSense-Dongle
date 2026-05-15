# DS5Dongle — Auto Haptics Edition

> **Fork of [awalol/DS5Dongle](https://github.com/awalol/DS5Dongle)**  
> Adds **Audio Auto Haptics**: your DualSense vibrates in sync with the game's sounds — footsteps, gunshots, explosions — even when the game has no haptic support.

---

## What does this do, concretely?

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

- A horse galloping → hoofbeat vibrations
- A gunshot → sharp impact in your hands
- An explosion → deep rumble that fades out
- A car engine → constant low-frequency buzz that changes with RPM

This works even if the game has **zero haptic support**. The Pico extracts the bass and impact sounds from the audio stream and converts them into motor signals, entirely by itself.

**Compared to the PS5 experience**, this is not identical — the PS5 has access to precise per-object haptic data from the game engine. What this does is a smart approximation from audio alone, similar to what apps like DSX do on Windows. In practice it adds a lot of immersion to games that would otherwise have no feedback at all.

---

## How it works internally

The Pico receives the game audio over USB (it appears as a 4-channel sound card). It then runs this signal through a small DSP chain entirely in firmware, with no CPU overhead on your PC:

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

### 3. Route audio to the Pico

The Pico needs to receive the game audio to generate haptics. **Do not set it as your default output** — your headset or speakers should remain the default. Instead, create a PipeWire loopback that sends a silent copy of your audio to the Pico in the background.

#### Linux (PipeWire)

The loopback taps the **monitor of your current default output** — whatever it is (headset, speakers, DAC, HDMI TV…). Your default output is never changed.

**Option A — one-shot loopback (runs until you close the terminal):**

```bash
# Auto-detect your current default output and create the loopback
DEFAULT_SINK=$(pactl info | grep "Default Sink" | awk '{print $3}')
pw-loopback \
  --capture-props="media.class=Audio/Source node.target=${DEFAULT_SINK}.monitor" \
  --playback-props='node.target=DS5\ Bridge'
```

Run this after plugging in the Pico. Your default output is untouched — the Pico receives a silent copy.

> If you change your default output (e.g. switch from headset to speakers), re-run the command to update the loopback.

**Option B — persistent loopback that follows the default output (recommended):**

This version always mirrors whatever your current default output is, automatically.

Create `~/.config/pipewire/filter-chain.conf.d/ds5-haptics-loopback.conf`:

```conf
context.modules = [
  { name = libpipewire-module-loopback
    args = {
      capture.props = {
        audio.position = [ FL FR ]
        node.target = "@DEFAULT_AUDIO_SINK@.monitor"
        stream.dont-remix = true
        node.passive = true
      }
      playback.props = {
        audio.position = [ FL FR ]
        node.target = "DS5 Bridge"
        stream.dont-remix = true
      }
    }
  }
]
```

Apply without rebooting:

```bash
systemctl --user restart pipewire pipewire-pulse
```

`@DEFAULT_AUDIO_SINK@.monitor` is a PipeWire special value that always resolves to the monitor of whatever sink is currently set as default — no hardcoded device name needed.

**Per-game routing (Steam / Proton):**

To send only one game's audio to the Pico instead of your whole system output:

1. Open `pavucontrol` → **Playback** tab while the game is running
2. Find the game's audio stream
3. Change its output to **DS5 Bridge**

Everything else keeps playing through your normal output.

#### Windows

Open **Sound settings** → right-click **DS5 Bridge** → **Properties** → enable it as a secondary output, or use a virtual audio cable (e.g. VoiceMeeter) to duplicate the stream.

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
