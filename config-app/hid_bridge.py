import struct
import subprocess
import re

try:
    import hid
    _HID_DEVICE_CLS = getattr(hid, 'Device', None) or getattr(hid, 'device', None)
except ImportError:
    hid = None
    _HID_DEVICE_CLS = None


class DS5BridgeError(Exception):
    pass


class DS5Bridge:
    SONY_VID = 0x054C
    DS5_PID  = 0x0CE6
    DSE_PID  = 0x0DF2

    # 23 bytes: ff BBBBBBBB H BBBBB
    CONFIG_FMT  = '<ffBBBBBBBBHBBBBB'
    CONFIG_SIZE = struct.calcsize(CONFIG_FMT)  # 23

    CONFIG_FIELDS = [
        'haptics_gain', 'speaker_volume',
        'inactive_time', 'disable_inactive_disconnect', 'disable_pico_led',
        'polling_rate_mode', 'audio_buffer_length', 'controller_mode',
        'auto_haptics_enable', 'auto_haptics_gain', 'auto_haptics_lowpass_hz',
        'enable_poweroff_shortcut', 'enable_touchpad',
        'poweroff_button', 'touchpad_button', 'battery_color_enable',
    ]

    DEFAULTS = {
        'haptics_gain': 1.0, 'speaker_volume': -100.0,
        'inactive_time': 30, 'disable_inactive_disconnect': 0,
        'disable_pico_led': 0, 'polling_rate_mode': 0,
        'audio_buffer_length': 64, 'controller_mode': 2,
        'auto_haptics_enable': 2, 'auto_haptics_gain': 100,
        'auto_haptics_lowpass_hz': 80, 'enable_poweroff_shortcut': 1,
        'enable_touchpad': 1, 'poweroff_button': 3,
        'touchpad_button': 2, 'battery_color_enable': 1,
    }

    def __init__(self):
        if hid is None or _HID_DEVICE_CLS is None:
            raise DS5BridgeError(
                "hidapi not installed.\nRun: pip install hid  or  pip install hidapi"
            )
        self._device = None
        self._model: str | None = None

    def _open_hid(self, vid: int, pid: int):
        """Open HID device — compatible with both 'hid' and 'hidapi' packages."""
        if hasattr(hid, 'Device'):
            # package 'hid' (ctypes wrapper, e.g. python-hid on Arch)
            return hid.Device(vid, pid)
        else:
            # package 'hidapi' (CFFI wrapper)
            dev = hid.device()
            dev.open(vid, pid)
            return dev

    def connect(self) -> str:
        for pid, label in [(self.DS5_PID, "DualSense"), (self.DSE_PID, "DualSense Edge")]:
            try:
                dev = self._open_hid(self.SONY_VID, pid)
                self._device = dev
                self._model = label
                return label
            except Exception:
                pass
        raise DS5BridgeError(
            "DS5Dongle not found.\nMake sure the Pico is plugged in via USB."
        )

    def disconnect(self):
        if self._device:
            try:
                self._device.close()
            except Exception:
                pass
            self._device = None
            self._model = None

    def is_connected(self) -> bool:
        return self._device is not None

    @property
    def model(self) -> str | None:
        return self._model

    def _assert_connected(self):
        if not self._device:
            raise DS5BridgeError("Not connected.")

    def read_config(self) -> dict:
        self._assert_connected()
        raw = bytes(self._device.get_feature_report(0xF7, 64))
        body = raw[1:1 + self.CONFIG_SIZE]
        if len(body) < self.CONFIG_SIZE:
            # Older firmware — pad with defaults for new fields
            body = body + bytes(self.CONFIG_SIZE - len(body))
        values = struct.unpack(self.CONFIG_FMT, body)
        cfg = dict(zip(self.CONFIG_FIELDS, values))
        cfg['haptics_gain'] = max(1.0, min(2.0, cfg['haptics_gain']))
        cfg['speaker_volume'] = max(-100.0, min(0.0, cfg['speaker_volume']))
        return cfg

    def write_config(self, cfg: dict):
        self._assert_connected()
        packed = struct.pack(
            self.CONFIG_FMT,
            cfg['haptics_gain'], cfg['speaker_volume'],
            cfg['inactive_time'], cfg['disable_inactive_disconnect'],
            cfg['disable_pico_led'], cfg['polling_rate_mode'],
            cfg['audio_buffer_length'], cfg['controller_mode'],
            cfg['auto_haptics_enable'], cfg['auto_haptics_gain'],
            cfg['auto_haptics_lowpass_hz'], cfg['enable_poweroff_shortcut'],
            cfg['enable_touchpad'], cfg['poweroff_button'],
            cfg['touchpad_button'], cfg['battery_color_enable'],
        )
        self._device.send_feature_report(bytes([0xF6, 0x01]) + packed)

    def save_config(self):
        self._assert_connected()
        self._device.send_feature_report(bytes([0xF6, 0x02]))

    def reconnect_usb(self):
        self._assert_connected()
        self._device.send_feature_report(bytes([0xF6, 0x03]))

    @staticmethod
    def read_battery_upower() -> int | None:
        try:
            result = subprocess.run(
                ['upower', '-d'], capture_output=True, text=True, timeout=3
            )
            text = result.stdout
            idx = text.find('DualSense')
            if idx == -1:
                return None
            m = re.search(r'percentage:\s+(\d+)%', text[idx:idx + 400])
            if m:
                return int(m.group(1))
        except Exception:
            pass
        return None
