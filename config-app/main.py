#!/usr/bin/env python3
import sys
import os
import time
import json
import glob

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QGroupBox, QLabel, QSlider, QComboBox, QCheckBox, QPushButton,
    QScrollArea, QFrame, QRadioButton, QButtonGroup,
    QSizePolicy, QMessageBox, QInputDialog,
)
from PyQt6.QtCore import Qt, QTimer, QThread, pyqtSignal, pyqtSlot
from PyQt6.QtGui import QFont

from hid_bridge import DS5Bridge, DS5BridgeError


BUTTON_NAMES     = ["Square", "Cross", "Circle", "Triangle", "L1", "R1", "Create", "Options", "Mute"]
POLLING_LABELS   = ["250 Hz", "500 Hz", "1000 Hz (Real-time)"]
CTRL_LABELS      = ["DualSense", "DualSense Edge", "Auto-detect"]


# ---------------------------------------------------------------------------
# Worker thread
# ---------------------------------------------------------------------------

class HidWorker(QThread):
    connected    = pyqtSignal(str)
    config_ready = pyqtSignal(dict)
    saved        = pyqtSignal()
    error        = pyqtSignal(str)

    def __init__(self, bridge: DS5Bridge, op: str, cfg: dict = None, needs_reconnect: bool = False):
        super().__init__()
        self._bridge          = bridge
        self._op              = op
        self._cfg             = cfg
        self._needs_reconnect = needs_reconnect

    def run(self):
        try:
            if self._op == 'connect':
                model = self._bridge.connect()
                self.connected.emit(model)
                cfg = self._bridge.read_config()
                self.config_ready.emit(cfg)

            elif self._op == 'read':
                cfg = self._bridge.read_config()
                self.config_ready.emit(cfg)

            elif self._op == 'save':
                self._bridge.write_config(self._cfg)
                self._bridge.save_config()
                if self._needs_reconnect:
                    self._bridge.reconnect_usb()
                    time.sleep(2.5)
                    self._bridge.connect()
                    cfg = self._bridge.read_config()
                    self.config_ready.emit(cfg)
                self.saved.emit()

        except DS5BridgeError as e:
            self.error.emit(str(e))
        except Exception as e:
            self.error.emit(str(e))


# ---------------------------------------------------------------------------
# Reusable slider widget
# ---------------------------------------------------------------------------

class SliderRow(QWidget):
    def __init__(self, label: str, min_val: int, max_val: int,
                 fmt: str = "{}", transform=None, parent=None):
        super().__init__(parent)
        self._fmt       = fmt
        self._transform = transform or (lambda v: v)

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 2, 0, 2)

        lbl = QLabel(label)
        lbl.setFixedWidth(190)

        self.slider = QSlider(Qt.Orientation.Horizontal)
        self.slider.setRange(min_val, max_val)
        self.slider.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)

        self.value_label = QLabel()
        self.value_label.setFixedWidth(100)
        self.value_label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)

        layout.addWidget(lbl)
        layout.addWidget(self.slider)
        layout.addWidget(self.value_label)

        self.slider.valueChanged.connect(self._on_change)
        self._on_change(min_val)

    def _on_change(self, raw: int):
        self.value_label.setText(self._fmt.format(self._transform(raw)))

    def value(self) -> int:
        return self.slider.value()

    def setValue(self, val: int):
        self.slider.setValue(int(val))


# ---------------------------------------------------------------------------
# Main window
# ---------------------------------------------------------------------------

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self._bridge   = DS5Bridge()
        self._worker   = None
        self._last_cfg = None
        self._workers  = []  # keep alive

        self.setWindowTitle("DS5Dongle Config")
        self.setMinimumWidth(700)
        self.resize(740, 880)

        self._build_ui()
        self._set_controls_enabled(False)

        self._battery_timer = QTimer(self)
        self._battery_timer.setInterval(30_000)
        self._battery_timer.timeout.connect(self._refresh_battery)

    # ------------------------------------------------------------------
    # UI construction
    # ------------------------------------------------------------------

    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        root = QVBoxLayout(central)
        root.setSpacing(0)
        root.setContentsMargins(0, 0, 0, 0)

        root.addWidget(self._make_header())

        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QFrame.Shape.NoFrame)

        content = QWidget()
        cl = QVBoxLayout(content)
        cl.setSpacing(10)
        cl.setContentsMargins(14, 14, 14, 14)
        for section in [
            self._make_haptics_section(),
            self._make_auto_haptics_section(),
            self._make_audio_section(),
            self._make_controller_section(),
            self._make_power_section(),
            self._make_shortcuts_section(),
            self._make_lightbar_section(),
        ]:
            cl.addWidget(section)
        cl.addStretch()
        scroll.setWidget(content)
        root.addWidget(scroll, 1)

        root.addWidget(self._make_preset_bar())
        root.addWidget(self._make_footer())

    def _make_header(self) -> QFrame:
        frame = QFrame()
        frame.setFrameShape(QFrame.Shape.StyledPanel)
        layout = QHBoxLayout(frame)
        layout.setContentsMargins(14, 8, 14, 8)

        self._status_dot   = QLabel("●")
        self._status_dot.setStyleSheet("color: #777;")
        self._status_label = QLabel("Not connected")
        self._battery_label = QLabel("")

        self._connect_btn = QPushButton("Connect")
        self._connect_btn.setFixedWidth(100)
        self._connect_btn.clicked.connect(self._on_connect)

        layout.addWidget(self._status_dot)
        layout.addWidget(self._status_label)
        layout.addStretch()
        layout.addWidget(self._battery_label)
        layout.addWidget(self._connect_btn)
        return frame

    def _make_preset_bar(self) -> QFrame:
        frame = QFrame()
        frame.setFrameShape(QFrame.Shape.StyledPanel)
        layout = QHBoxLayout(frame)
        layout.setContentsMargins(14, 6, 14, 6)

        layout.addWidget(QLabel("Preset:"))

        self._preset_combo = QComboBox()
        self._preset_combo.setMinimumWidth(180)
        self._preset_combo.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        layout.addWidget(self._preset_combo)

        self._preset_load_btn = QPushButton("Load")
        self._preset_load_btn.setFixedWidth(70)
        self._preset_load_btn.clicked.connect(self._on_preset_load)

        self._preset_save_btn = QPushButton("Save As…")
        self._preset_save_btn.setFixedWidth(80)
        self._preset_save_btn.clicked.connect(self._on_preset_save)

        self._preset_del_btn = QPushButton("Delete")
        self._preset_del_btn.setFixedWidth(70)
        self._preset_del_btn.clicked.connect(self._on_preset_delete)

        layout.addWidget(self._preset_load_btn)
        layout.addWidget(self._preset_save_btn)
        layout.addWidget(self._preset_del_btn)

        self._refresh_preset_list()
        return frame

    def _make_footer(self) -> QFrame:
        frame = QFrame()
        frame.setFrameShape(QFrame.Shape.StyledPanel)
        layout = QHBoxLayout(frame)
        layout.setContentsMargins(14, 8, 14, 8)

        self._read_btn = QPushButton("Read Config")
        self._read_btn.clicked.connect(self._on_read)

        self._save_btn = QPushButton("Save to Device")
        self._save_btn.setDefault(True)
        self._save_btn.clicked.connect(self._on_save)

        layout.addStretch()
        layout.addWidget(self._read_btn)
        layout.addWidget(self._save_btn)
        return frame

    def _make_haptics_section(self) -> QGroupBox:
        box = QGroupBox("Haptics")
        layout = QVBoxLayout(box)
        self._haptics_gain = SliderRow(
            "Global gain", 100, 200, "{:.2f}×",
            transform=lambda v: v / 100
        )
        layout.addWidget(self._haptics_gain)
        return box

    def _make_auto_haptics_section(self) -> QGroupBox:
        box = QGroupBox("Auto Haptics  (audio → rumble)")
        layout = QVBoxLayout(box)

        mode_row = QHBoxLayout()
        mode_lbl = QLabel("Mode")
        mode_lbl.setFixedWidth(190)
        self._ah_off     = QRadioButton("Off")
        self._ah_mix     = QRadioButton("Mix")
        self._ah_replace = QRadioButton("Replace")
        self._ah_group   = QButtonGroup(self)
        self._ah_group.addButton(self._ah_off,     0)
        self._ah_group.addButton(self._ah_mix,     1)
        self._ah_group.addButton(self._ah_replace, 2)
        mode_row.addWidget(mode_lbl)
        mode_row.addWidget(self._ah_off)
        mode_row.addWidget(self._ah_mix)
        mode_row.addWidget(self._ah_replace)
        mode_row.addStretch()
        layout.addLayout(mode_row)

        self._ah_gain    = SliderRow("Intensity",       0,   200, "{}%")
        self._ah_lowpass = SliderRow("Low-pass cutoff", 20,  400, "{} Hz")
        layout.addWidget(self._ah_gain)
        layout.addWidget(self._ah_lowpass)
        return box

    def _make_audio_section(self) -> QGroupBox:
        box = QGroupBox("Audio")
        layout = QVBoxLayout(box)
        self._speaker_vol = SliderRow("Speaker volume", -100,  0,  "{} dB")
        self._buffer_len  = SliderRow("Haptics buffer",   16, 128, "{} samples")
        layout.addWidget(self._speaker_vol)
        layout.addWidget(self._buffer_len)
        return box

    def _make_controller_section(self) -> QGroupBox:
        box = QGroupBox("Controller")
        layout = QVBoxLayout(box)

        self._ctrl_mode    = self._combo_row(layout, "Controller mode", CTRL_LABELS)
        self._polling_rate = self._combo_row(layout, "Polling rate",    POLLING_LABELS)

        note = QLabel("Polling rate and controller mode changes require saving and reconnecting the Pico USB.")
        note.setWordWrap(True)
        note.setStyleSheet("color: gray; font-size: 11px;")
        layout.addWidget(note)
        return box

    def _make_power_section(self) -> QGroupBox:
        box = QGroupBox("Power")
        layout = QVBoxLayout(box)
        self._inactive_time = SliderRow("Auto-off after", 5, 60, "{} min")
        layout.addWidget(self._inactive_time)
        self._disable_auto_off = QCheckBox("Always stay connected (disable auto-off)")
        self._disable_led      = QCheckBox("Disable Pico LED")
        layout.addWidget(self._disable_auto_off)
        layout.addWidget(self._disable_led)
        return box

    def _make_shortcuts_section(self) -> QGroupBox:
        box = QGroupBox("Button Shortcuts")
        layout = QVBoxLayout(box)

        r1 = QHBoxLayout()
        self._poweroff_enabled = QCheckBox("Power-off shortcut  (PS +)")
        self._poweroff_enabled.setFixedWidth(250)
        self._poweroff_btn = QComboBox()
        self._poweroff_btn.addItems(BUTTON_NAMES)
        self._poweroff_btn.setFixedWidth(120)
        r1.addWidget(self._poweroff_enabled)
        r1.addWidget(self._poweroff_btn)
        r1.addStretch()
        layout.addLayout(r1)

        r2 = QHBoxLayout()
        self._touchpad_enabled = QCheckBox("Touchpad toggle  (PS +)")
        self._touchpad_enabled.setFixedWidth(250)
        self._touchpad_btn = QComboBox()
        self._touchpad_btn.addItems(BUTTON_NAMES)
        self._touchpad_btn.setFixedWidth(120)
        r2.addWidget(self._touchpad_enabled)
        r2.addWidget(self._touchpad_btn)
        r2.addStretch()
        layout.addLayout(r2)
        return box

    def _make_lightbar_section(self) -> QGroupBox:
        box = QGroupBox("Lightbar")
        layout = QVBoxLayout(box)
        self._battery_color = QCheckBox("Change color based on battery level")
        layout.addWidget(self._battery_color)
        return box

    def _combo_row(self, parent_layout: QVBoxLayout, label: str, items: list) -> QComboBox:
        row = QHBoxLayout()
        lbl = QLabel(label)
        lbl.setFixedWidth(190)
        combo = QComboBox()
        combo.addItems(items)
        combo.setFixedWidth(220)
        row.addWidget(lbl)
        row.addWidget(combo)
        row.addStretch()
        parent_layout.addLayout(row)
        return combo

    # ------------------------------------------------------------------
    # Preset management
    # ------------------------------------------------------------------

    @property
    def _presets_dir(self) -> str:
        d = os.path.join(os.path.expanduser("~"), ".config", "ds5dongle", "presets")
        os.makedirs(d, exist_ok=True)
        return d

    def _refresh_preset_list(self):
        self._preset_combo.clear()
        files = sorted(glob.glob(os.path.join(self._presets_dir, "*.json")))
        for f in files:
            self._preset_combo.addItem(os.path.splitext(os.path.basename(f))[0], userData=f)
        has = self._preset_combo.count() > 0
        self._preset_load_btn.setEnabled(has)
        self._preset_del_btn.setEnabled(has)

    def _on_preset_save(self):
        name, ok = QInputDialog.getText(self, "Save Preset", "Preset name:")
        if not ok or not name.strip():
            return
        name = name.strip()
        # Sanitize: keep only alphanumeric, spaces, dashes, underscores
        safe = "".join(c for c in name if c.isalnum() or c in " -_").strip()
        if not safe:
            QMessageBox.warning(self, "DS5Dongle Config", "Invalid preset name.")
            return
        path = os.path.join(self._presets_dir, f"{safe}.json")
        cfg = self._collect_config()
        cfg['_preset_name'] = name
        with open(path, 'w', encoding='utf-8') as f:
            json.dump(cfg, f, indent=2)
        self._refresh_preset_list()
        # Select the newly saved preset
        idx = self._preset_combo.findText(safe)
        if idx >= 0:
            self._preset_combo.setCurrentIndex(idx)
        self.statusBar().showMessage(f"Preset \"{safe}\" saved.")

    def _on_preset_load(self):
        path = self._preset_combo.currentData()
        if not path or not os.path.exists(path):
            return
        with open(path, 'r', encoding='utf-8') as f:
            cfg = json.load(f)
        cfg.pop('_preset_name', None)
        self._apply_config(cfg)
        self.statusBar().showMessage(
            f"Preset \"{self._preset_combo.currentText()}\" loaded — click Save to Device to apply."
        )

    def _on_preset_delete(self):
        name = self._preset_combo.currentText()
        path = self._preset_combo.currentData()
        if not path:
            return
        ans = QMessageBox.question(
            self, "Delete Preset",
            f"Delete preset \"{name}\"?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        )
        if ans == QMessageBox.StandardButton.Yes:
            try:
                os.remove(path)
            except OSError:
                pass
            self._refresh_preset_list()
            self.statusBar().showMessage(f"Preset \"{name}\" deleted.")

    # ------------------------------------------------------------------
    # Enable / disable controls
    # ------------------------------------------------------------------

    def _set_controls_enabled(self, enabled: bool):
        controls = [
            self._read_btn, self._save_btn,
            self._haptics_gain.slider,
            self._ah_off, self._ah_mix, self._ah_replace,
            self._ah_gain.slider, self._ah_lowpass.slider,
            self._speaker_vol.slider, self._buffer_len.slider,
            self._ctrl_mode, self._polling_rate,
            self._inactive_time.slider,
            self._disable_auto_off, self._disable_led,
            self._poweroff_enabled, self._poweroff_btn,
            self._touchpad_enabled, self._touchpad_btn,
            self._battery_color,
        ]
        for w in controls:
            w.setEnabled(enabled)

    # ------------------------------------------------------------------
    # Button handlers
    # ------------------------------------------------------------------

    def _on_connect(self):
        if self._bridge.is_connected():
            self._bridge.disconnect()
            self._status_dot.setStyleSheet("color: #777;")
            self._status_label.setText("Not connected")
            self._battery_label.setText("")
            self._connect_btn.setText("Connect")
            self._set_controls_enabled(False)
            self._battery_timer.stop()
            return

        self._connect_btn.setEnabled(False)
        self.statusBar().showMessage("Connecting…")
        self._run_worker('connect')

    def _on_read(self):
        self.statusBar().showMessage("Reading config…")
        self._run_worker('read')

    def _on_save(self):
        cfg = self._collect_config()
        needs_reconnect = bool(self._last_cfg) and (
            cfg['polling_rate_mode'] != self._last_cfg.get('polling_rate_mode') or
            cfg['controller_mode']   != self._last_cfg.get('controller_mode')
        )
        msg = "Saving… Reconnecting USB…" if needs_reconnect else "Saving…"
        self.statusBar().showMessage(msg)
        self._save_btn.setEnabled(False)
        self._read_btn.setEnabled(False)
        self._run_worker('save', cfg=cfg, needs_reconnect=needs_reconnect)

    # ------------------------------------------------------------------
    # Worker
    # ------------------------------------------------------------------

    def _run_worker(self, op: str, cfg: dict = None, needs_reconnect: bool = False):
        w = HidWorker(self._bridge, op, cfg, needs_reconnect)
        w.connected.connect(self._on_connected)
        w.config_ready.connect(self._apply_config)
        w.saved.connect(self._on_saved)
        w.error.connect(self._on_error)
        w.finished.connect(lambda: self._connect_btn.setEnabled(True))
        self._worker = w
        self._workers.append(w)
        w.finished.connect(lambda: self._workers.remove(w) if w in self._workers else None)
        w.start()

    # ------------------------------------------------------------------
    # Slots
    # ------------------------------------------------------------------

    @pyqtSlot(str)
    def _on_connected(self, model: str):
        self._status_dot.setStyleSheet("color: #4caf50;")
        self._status_label.setText(f"Connected: {model}")
        self._connect_btn.setText("Disconnect")
        self._connect_btn.setEnabled(True)
        self._set_controls_enabled(True)
        self._battery_timer.start()
        self._refresh_battery()

    @pyqtSlot(dict)
    def _apply_config(self, cfg: dict):
        self._last_cfg = cfg.copy()

        self._haptics_gain.setValue(int(cfg['haptics_gain'] * 100))

        btn = self._ah_group.button(cfg['auto_haptics_enable'])
        if btn:
            btn.setChecked(True)
        self._ah_gain.setValue(cfg['auto_haptics_gain'])
        self._ah_lowpass.setValue(cfg['auto_haptics_lowpass_hz'])

        self._speaker_vol.setValue(int(cfg['speaker_volume']))
        self._buffer_len.setValue(cfg['audio_buffer_length'])

        self._ctrl_mode.setCurrentIndex(cfg['controller_mode'])
        self._polling_rate.setCurrentIndex(cfg['polling_rate_mode'])

        self._inactive_time.setValue(cfg['inactive_time'])
        self._disable_auto_off.setChecked(bool(cfg['disable_inactive_disconnect']))
        self._disable_led.setChecked(bool(cfg['disable_pico_led']))

        self._poweroff_enabled.setChecked(bool(cfg['enable_poweroff_shortcut']))
        self._poweroff_btn.setCurrentIndex(cfg['poweroff_button'])
        self._touchpad_enabled.setChecked(bool(cfg['enable_touchpad']))
        self._touchpad_btn.setCurrentIndex(cfg['touchpad_button'])

        self._battery_color.setChecked(bool(cfg['battery_color_enable']))

        self.statusBar().showMessage("Config loaded.")

    @pyqtSlot()
    def _on_saved(self):
        self._save_btn.setEnabled(True)
        self._read_btn.setEnabled(True)
        self.statusBar().showMessage("Saved to device.")

    @pyqtSlot(str)
    def _on_error(self, msg: str):
        self._connect_btn.setEnabled(True)
        self._save_btn.setEnabled(True)
        self._read_btn.setEnabled(True)
        self.statusBar().showMessage(f"Error: {msg}")
        QMessageBox.critical(self, "DS5Dongle Config", msg)

    # ------------------------------------------------------------------
    # Config collect / battery
    # ------------------------------------------------------------------

    def _collect_config(self) -> dict:
        mode = self._ah_group.checkedId()
        return {
            'haptics_gain':                self._haptics_gain.value() / 100.0,
            'speaker_volume':              float(self._speaker_vol.value()),
            'inactive_time':               self._inactive_time.value(),
            'disable_inactive_disconnect': int(self._disable_auto_off.isChecked()),
            'disable_pico_led':            int(self._disable_led.isChecked()),
            'polling_rate_mode':           self._polling_rate.currentIndex(),
            'audio_buffer_length':         self._buffer_len.value(),
            'controller_mode':             self._ctrl_mode.currentIndex(),
            'auto_haptics_enable':         max(0, mode),
            'auto_haptics_gain':           self._ah_gain.value(),
            'auto_haptics_lowpass_hz':     self._ah_lowpass.value(),
            'enable_poweroff_shortcut':    int(self._poweroff_enabled.isChecked()),
            'enable_touchpad':             int(self._touchpad_enabled.isChecked()),
            'poweroff_button':             self._poweroff_btn.currentIndex(),
            'touchpad_button':             self._touchpad_btn.currentIndex(),
            'battery_color_enable':        int(self._battery_color.isChecked()),
        }

    def _refresh_battery(self):
        pct = DS5Bridge.read_battery_upower()
        self._battery_label.setText(f"🔋 {pct}%" if pct is not None else "")

    def closeEvent(self, event):
        self._bridge.disconnect()
        super().closeEvent(event)


# ---------------------------------------------------------------------------

def main():
    app = QApplication(sys.argv)
    app.setApplicationName("DS5Dongle Config")
    app.setOrganizationName("loteran")

    try:
        window = MainWindow()
    except DS5BridgeError as e:
        from PyQt6.QtWidgets import QMessageBox
        QMessageBox.critical(None, "DS5Dongle Config", str(e))
        sys.exit(1)

    window.show()
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
