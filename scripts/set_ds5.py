#!/usr/bin/env python3
"""
DS5Dongle configuration tool.
Requires: python-hidapi (pip install hidapi)  — NOT python-hid
"""

import struct
import sys
import argparse

try:
    import hid
except ImportError:
    print("[ERROR] Missing dependency: install with  pip install hidapi")
    sys.exit(1)

SONY_VID = 0x054C
DS5_PID  = 0x0CE6  # DualSense
DSE_PID  = 0x0DF2  # DualSense Edge

# Config_body layout (20 bytes, little-endian)
#   float  haptics_gain               [0:4]
#   float  speaker_volume             [4:8]
#   uint8  inactive_time              [8]
#   uint8  disable_inactive_disc.     [9]
#   uint8  disable_pico_led           [10]
#   uint8  polling_rate_mode          [11]
#   uint8  audio_buffer_length        [12]
#   uint8  controller_mode            [13]
#   uint8  auto_haptics_enable        [14]
#   uint8  auto_haptics_gain          [15]
#   uint16 auto_haptics_lowpass_hz    [16:18]  (free Hz value 20–400)
#   uint8  enable_poweroff_shortcut   [18]
#   uint8  enable_touchpad            [19]
CONFIG_FMT  = '<ffBBBBBBBBHBB'
CONFIG_SIZE = struct.calcsize(CONFIG_FMT)  # 20

POLLING_MODES     = {0: "250 Hz", 1: "500 Hz", 2: "Real-time (1000 Hz)"}
CONTROLLER_MODES  = {0: "DS5", 1: "DSE (Edge)", 2: "Auto"}
AUTO_HAP_MODES    = {
    0: "Off (pass-through ch3/ch4)",
    1: "Mix (native + audio-derived)",
    2: "Replace (audio-derived only)",
}


# ---------------------------------------------------------------------------
# Device helpers
# ---------------------------------------------------------------------------

def open_device():
    for pid, label in [(DS5_PID, "DualSense"), (DSE_PID, "DualSense Edge")]:
        try:
            device = hid.device()
            device.open(SONY_VID, pid)
            print(f"[INFO] Connected to {label}")
            return device
        except Exception:
            pass
    print("[ERROR] No DS5/DSE device found. Make sure the Pico is plugged in.")
    sys.exit(1)


# ---------------------------------------------------------------------------
# Config read / write
# ---------------------------------------------------------------------------

def get_config(device):
    raw = bytes(device.get_feature_report(0xF7, 64))
    body = raw[1:1 + CONFIG_SIZE]
    if len(body) < CONFIG_SIZE:
        print(f"[ERROR] Config too short ({len(body)} bytes, expected {CONFIG_SIZE}). "
              "Flash the latest firmware first.")
        sys.exit(1)
    (
        haptics_gain, speaker_volume,
        inactive_time, disable_inactive_disconnect, disable_pico_led,
        polling_rate_mode, audio_buffer_length, controller_mode,
        auto_haptics_enable, auto_haptics_gain, auto_haptics_lowpass_hz,
        enable_poweroff_shortcut, enable_touchpad,
    ) = struct.unpack(CONFIG_FMT, body)
    return {
        'haptics_gain':                haptics_gain,
        'speaker_volume':              speaker_volume,
        'inactive_time':               inactive_time,
        'disable_inactive_disconnect': disable_inactive_disconnect,
        'disable_pico_led':            disable_pico_led,
        'polling_rate_mode':           polling_rate_mode,
        'audio_buffer_length':         audio_buffer_length,
        'controller_mode':             controller_mode,
        'auto_haptics_enable':         auto_haptics_enable,
        'auto_haptics_gain':           auto_haptics_gain,
        'auto_haptics_lowpass_hz':     auto_haptics_lowpass_hz,
        'enable_poweroff_shortcut':    enable_poweroff_shortcut,
        'enable_touchpad':             enable_touchpad,
    }


def print_config(cfg):
    print("\n=== DS5Dongle Configuration ===")
    print(f"  haptics_gain                : {cfg['haptics_gain']:.2f}  [1.0 – 2.0]")
    print(f"  speaker_volume              : {cfg['speaker_volume']:.1f} dB  [-100 – 0]")
    print(f"  inactive_time               : {cfg['inactive_time']} min  [5 – 60]")
    print(f"  disable_inactive_disconnect : {cfg['disable_inactive_disconnect']}  (0=auto-off, 1=stay on)")
    print(f"  disable_pico_led            : {cfg['disable_pico_led']}  (0=LED on, 1=LED off)")
    pm = cfg['polling_rate_mode']
    print(f"  polling_rate_mode           : {pm}  ({POLLING_MODES.get(pm, '?')})")
    print(f"  audio_buffer_length         : {cfg['audio_buffer_length']}  [16 – 128]")
    cm = cfg['controller_mode']
    print(f"  controller_mode             : {cm}  ({CONTROLLER_MODES.get(cm, '?')})")
    print()
    print("--- Auto Haptics (audio → rumble) ---")
    ae = cfg['auto_haptics_enable']
    print(f"  auto_haptics_enable         : {ae}  ({AUTO_HAP_MODES.get(ae, '?')})")
    print(f"  auto_haptics_gain           : {cfg['auto_haptics_gain']}%  [0 – 200]")
    print(f"  auto_haptics_lowpass_hz     : {cfg['auto_haptics_lowpass_hz']} Hz  [20 – 400]")
    print()
    print("--- Input ---")
    print(f"  enable_poweroff_shortcut    : {cfg['enable_poweroff_shortcut']}  (1=PS+Triangle powers off)")
    print(f"  enable_touchpad             : {cfg['enable_touchpad']}  (1=on, 0=off; PS+Circle toggles runtime)")
    print("================================\n")


def set_config(device, **kwargs):
    cfg = get_config(device)

    for key, value in kwargs.items():
        if key not in cfg:
            print(f"[WARNING] Unknown parameter ignored: {key}")
            continue
        cfg[key] = value
        print(f"[INFO] {key} = {value}")

    packed = struct.pack(
        CONFIG_FMT,
        cfg['haptics_gain'],
        cfg['speaker_volume'],
        cfg['inactive_time'],
        cfg['disable_inactive_disconnect'],
        cfg['disable_pico_led'],
        cfg['polling_rate_mode'],
        cfg['audio_buffer_length'],
        cfg['controller_mode'],
        cfg['auto_haptics_enable'],
        cfg['auto_haptics_gain'],
        cfg['auto_haptics_lowpass_hz'],
        cfg['enable_poweroff_shortcut'],
        cfg['enable_touchpad'],
    )

    print("[INFO] Writing config to memory...")
    device.send_feature_report(bytes([0xF6, 0x01]) + packed)

    print("[INFO] Saving to flash...")
    device.send_feature_report(bytes([0xF6, 0x02]))

    print("[INFO] Reconnecting USB...")
    device.send_feature_report(bytes([0xF6, 0x03]))

    print("[OK] Configuration saved.")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser():
    p = argparse.ArgumentParser(
        description="Configure DS5Dongle firmware settings.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Show current config
  python set_ds5.py

  # Enable auto haptics (replace mode, 160 Hz LP, 120% gain)
  python set_ds5.py --auto-haptics-enable 2 --auto-haptics-gain 120 --auto-haptics-lowpass 160

  # Mix mode: native haptics + audio-derived
  python set_ds5.py --auto-haptics-enable 1

  # Disable auto haptics entirely
  python set_ds5.py --auto-haptics-enable 0

  # Adjust haptics intensity and speaker volume
  python set_ds5.py --haptics-gain 1.8 --speaker-volume -10

  # Switch to 500 Hz polling
  python set_ds5.py --polling-rate 1

Auto haptics modes:
  0 = Off            — use native ch3/ch4 haptics only (original behaviour)
  1 = Mix            — native ch3/ch4 + audio-derived signal blended together
  2 = Replace        — audio-derived signal only (works even when game sends no haptics)

Auto haptics lowpass cutoff (free Hz value, 20–400):
  20  Hz             — sub-bass only (very heavy, low-frequency rumble)
  80  Hz             — deep bass (default, good all-round feel)
  160 Hz             — balanced bass (more detail, good for most games)
  400 Hz             — wide bass (maximum detail)
""")

    p.add_argument('--haptics-gain', type=float,
                   metavar='GAIN', help='Haptics gain [1.0–2.0]')
    p.add_argument('--speaker-volume', type=float,
                   metavar='DB', help='Speaker volume in dB [-100–0]')
    p.add_argument('--inactive-time', type=int,
                   metavar='MIN', help='Auto-disconnect delay in minutes [5–60]')
    p.add_argument('--disable-inactive-disconnect', type=int, choices=[0, 1],
                   metavar='0|1', help='0=auto-off enabled, 1=always stay on')
    p.add_argument('--disable-pico-led', type=int, choices=[0, 1],
                   metavar='0|1', help='0=LED on, 1=LED off')
    p.add_argument('--polling-rate', type=int, choices=[0, 1, 2],
                   metavar='0|1|2', help='Polling rate: 0=250Hz, 1=500Hz, 2=1000Hz')
    p.add_argument('--audio-buffer-length', type=int,
                   metavar='N', help='Haptics audio buffer length [16–128]')
    p.add_argument('--controller-mode', type=int, choices=[0, 1, 2],
                   metavar='0|1|2', help='Controller mode: 0=DS5, 1=DSE, 2=Auto')

    g = p.add_argument_group('Auto haptics (audio → rumble)')
    g.add_argument('--auto-haptics-enable', type=int, choices=[0, 1, 2],
                   metavar='0|1|2',
                   help='0=off, 1=mix with native, 2=replace native (default: 2)')
    g.add_argument('--auto-haptics-gain', type=int,
                   metavar='PCT',
                   help='Auto haptics intensity 0–200%% of haptics_gain (default: 100)')
    g.add_argument('--auto-haptics-lowpass', type=int,
                   metavar='HZ',
                   help='LP cutoff in Hz [20–400], default 80')

    h = p.add_argument_group('Input')
    h.add_argument('--enable-poweroff-shortcut', type=int, choices=[0, 1],
                   metavar='0|1', help='1=PS+Triangle powers off controller (default: 1)')
    h.add_argument('--enable-touchpad', type=int, choices=[0, 1],
                   metavar='0|1', help='1=touchpad active, 0=disabled (PS+Circle toggles runtime)')
    return p


def validate(args):
    errs = []
    if args.haptics_gain is not None and not (1.0 <= args.haptics_gain <= 2.0):
        errs.append("--haptics-gain must be between 1.0 and 2.0")
    if args.speaker_volume is not None and not (-100.0 <= args.speaker_volume <= 0.0):
        errs.append("--speaker-volume must be between -100 and 0")
    if args.inactive_time is not None and not (5 <= args.inactive_time <= 60):
        errs.append("--inactive-time must be between 5 and 60")
    if args.audio_buffer_length is not None and not (16 <= args.audio_buffer_length <= 128):
        errs.append("--audio-buffer-length must be between 16 and 128")
    if args.auto_haptics_gain is not None and not (0 <= args.auto_haptics_gain <= 200):
        errs.append("--auto-haptics-gain must be between 0 and 200")
    if args.auto_haptics_lowpass is not None and not (20 <= args.auto_haptics_lowpass <= 400):
        errs.append("--auto-haptics-lowpass must be between 20 and 400")
    if errs:
        for e in errs:
            print(f"[ERROR] {e}")
        sys.exit(1)


def main():
    parser = build_parser()
    args = parser.parse_args()
    validate(args)

    # Build the dict of changes requested
    changes = {}
    mapping = {
        'haptics_gain':                args.haptics_gain,
        'speaker_volume':              args.speaker_volume,
        'inactive_time':               args.inactive_time,
        'disable_inactive_disconnect': args.disable_inactive_disconnect,
        'disable_pico_led':            args.disable_pico_led,
        'polling_rate_mode':           args.polling_rate,
        'audio_buffer_length':         args.audio_buffer_length,
        'controller_mode':             args.controller_mode,
        'auto_haptics_enable':         args.auto_haptics_enable,
        'auto_haptics_gain':           args.auto_haptics_gain,
        'auto_haptics_lowpass_hz':     args.auto_haptics_lowpass,
        'enable_poweroff_shortcut':    args.enable_poweroff_shortcut,
        'enable_touchpad':             args.enable_touchpad,
    }
    for key, val in mapping.items():
        if val is not None:
            changes[key] = val

    device = open_device()
    try:
        if changes:
            set_config(device, **changes)
            # Device reconnects after save — reopen to read back config
            device.close()
            import time; time.sleep(2)
            device = open_device()
        cfg = get_config(device)
        print_config(cfg)
    finally:
        device.close()


if __name__ == '__main__':
    main()
