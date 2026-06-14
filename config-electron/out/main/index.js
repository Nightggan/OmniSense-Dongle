"use strict";
const electron = require("electron");
const path = require("path");
const fs = require("fs");
const HID = require("node-hid");
const child_process = require("child_process");
function settingsPath() {
  return path.join(electron.app.getPath("userData"), "settings.json");
}
function loadSettings() {
  try {
    return JSON.parse(fs.readFileSync(settingsPath(), "utf8"));
  } catch {
    return {};
  }
}
function saveSettings(settings) {
  try {
    fs.writeFileSync(settingsPath(), JSON.stringify(settings, null, 2), "utf8");
  } catch {
  }
}
const IPC = {
  DEVICE_CONNECT: "device:connect",
  DEVICE_DISCONNECT: "device:disconnect",
  DEVICE_STATUS: "device:status",
  DEVICE_BATTERY: "device:battery",
  CONFIG_READ: "config:read",
  CONFIG_WRITE: "config:write",
  CONFIG_SAVE: "config:save",
  PRESETS_LIST: "presets:list",
  PRESETS_LOAD: "presets:load",
  PRESETS_SAVE: "presets:save",
  PRESETS_DELETE: "presets:delete"
};
const IPC_EVENTS = {
  DEVICE_TELEMETRY: "device:telemetry"
};
const SONY_VID = 1356;
const DS5_PID = 3302;
const EDGE_PID = 3570;
const REPORT_READ_CONFIG = 247;
const REPORT_CMD = 246;
const REPORT_FIRMWARE_VERSION = 248;
const REPORT_RSSI = 249;
const CMD_WRITE_CONFIG = 1;
const CMD_SAVE_CONFIG = 2;
const CMD_RECONNECT_USB = 3;
const CONFIG_SIZE = 23;
const FIELD_OFFSETS = {
  hapticsGain: 0,
  // float32LE [0..3]
  speakerVolume: 4,
  // float32LE [4..7]
  inactiveTime: 8,
  // uint8
  disableInactiveDisconnect: 9,
  // uint8 (bool)
  disablePicoLed: 10,
  // uint8 (bool)
  pollingRateMode: 11,
  // uint8
  audioBufferLength: 12,
  // uint8
  controllerMode: 13,
  // uint8
  autoHapticsEnable: 14,
  // uint8
  autoHapticsGain: 15,
  // uint8
  autoHapticsLowpassHz: 16,
  // uint16LE [16..17]
  enablePoweroffShortcut: 18,
  // uint8 (bool)
  enableTouchpad: 19,
  // uint8 (bool)
  poweroffButton: 20,
  // uint8
  touchpadButton: 21,
  // uint8
  batteryColorEnable: 22
  // uint8 (bool)
};
class DS5DeviceError extends Error {
  constructor(code, message) {
    super(message);
    this.code = code;
    this.name = "DS5DeviceError";
  }
}
function unpackConfig(raw) {
  if (raw.length < CONFIG_SIZE) {
    raw = Buffer.concat([raw, Buffer.alloc(CONFIG_SIZE - raw.length)]);
  }
  const o = FIELD_OFFSETS;
  const cfg = {
    hapticsGain: Math.min(2, Math.max(1, raw.readFloatLE(o.hapticsGain))),
    speakerVolume: Math.min(0, Math.max(-100, raw.readFloatLE(o.speakerVolume))),
    inactiveTime: raw.readUInt8(o.inactiveTime),
    disableInactiveDisconnect: raw.readUInt8(o.disableInactiveDisconnect) !== 0,
    disablePicoLed: raw.readUInt8(o.disablePicoLed) !== 0,
    pollingRateMode: raw.readUInt8(o.pollingRateMode),
    audioBufferLength: raw.readUInt8(o.audioBufferLength),
    controllerMode: raw.readUInt8(o.controllerMode),
    autoHapticsEnable: raw.readUInt8(o.autoHapticsEnable),
    autoHapticsGain: raw.readUInt8(o.autoHapticsGain),
    autoHapticsLowpassHz: raw.readUInt16LE(o.autoHapticsLowpassHz),
    enablePoweroffShortcut: raw.readUInt8(o.enablePoweroffShortcut) !== 0,
    enableTouchpad: raw.readUInt8(o.enableTouchpad) !== 0,
    poweroffButton: raw.readUInt8(o.poweroffButton),
    touchpadButton: raw.readUInt8(o.touchpadButton),
    batteryColorEnable: raw.readUInt8(o.batteryColorEnable) !== 0
  };
  return cfg;
}
function packConfig(cfg) {
  const buf = Buffer.alloc(CONFIG_SIZE);
  const o = FIELD_OFFSETS;
  buf.writeFloatLE(cfg.hapticsGain, o.hapticsGain);
  buf.writeFloatLE(cfg.speakerVolume, o.speakerVolume);
  buf.writeUInt8(cfg.inactiveTime, o.inactiveTime);
  buf.writeUInt8(cfg.disableInactiveDisconnect ? 1 : 0, o.disableInactiveDisconnect);
  buf.writeUInt8(cfg.disablePicoLed ? 1 : 0, o.disablePicoLed);
  buf.writeUInt8(cfg.pollingRateMode, o.pollingRateMode);
  buf.writeUInt8(cfg.audioBufferLength, o.audioBufferLength);
  buf.writeUInt8(cfg.controllerMode, o.controllerMode);
  buf.writeUInt8(cfg.autoHapticsEnable, o.autoHapticsEnable);
  buf.writeUInt8(cfg.autoHapticsGain, o.autoHapticsGain);
  buf.writeUInt16LE(cfg.autoHapticsLowpassHz, o.autoHapticsLowpassHz);
  buf.writeUInt8(cfg.enablePoweroffShortcut ? 1 : 0, o.enablePoweroffShortcut);
  buf.writeUInt8(cfg.enableTouchpad ? 1 : 0, o.enableTouchpad);
  buf.writeUInt8(cfg.poweroffButton, o.poweroffButton);
  buf.writeUInt8(cfg.touchpadButton, o.touchpadButton);
  buf.writeUInt8(cfg.batteryColorEnable ? 1 : 0, o.batteryColorEnable);
  return buf;
}
class DS5HapticsDevice {
  device = null;
  _model = null;
  // --- Connection management ---
  connect() {
    const candidates = [
      [DS5_PID, "DualSense"],
      [EDGE_PID, "DualSense Edge"]
    ];
    for (const [pid, label] of candidates) {
      const infos = HID.devices(SONY_VID, pid);
      if (infos.length === 0) continue;
      const path2 = infos[0].path;
      if (!path2) continue;
      try {
        this.device = new HID.HID(path2);
        this._model = label;
        return label;
      } catch {
      }
    }
    throw new DS5DeviceError(
      "NOT_FOUND",
      "DS5Dongle not found. Make sure the Pico is plugged in via USB."
    );
  }
  disconnect() {
    if (this.device) {
      try {
        this.device.close();
      } catch {
      }
      this.device = null;
      this._model = null;
    }
  }
  isConnected() {
    return this.device !== null;
  }
  get model() {
    return this._model;
  }
  assertConnected() {
    if (!this.device) {
      throw new DS5DeviceError("NOT_CONNECTED", "Not connected to DS5Dongle.");
    }
    return this.device;
  }
  // --- Config read / write ---
  readConfig() {
    const dev = this.assertConnected();
    const raw = dev.getFeatureReport(REPORT_READ_CONFIG, 1 + CONFIG_SIZE);
    const body = Buffer.from(raw.slice(1, 1 + CONFIG_SIZE));
    return unpackConfig(body);
  }
  writeConfig(cfg) {
    const dev = this.assertConnected();
    const payload = Buffer.alloc(2 + CONFIG_SIZE);
    payload[0] = REPORT_CMD;
    payload[1] = CMD_WRITE_CONFIG;
    packConfig(cfg).copy(payload, 2);
    dev.sendFeatureReport([...payload]);
  }
  saveConfig() {
    const dev = this.assertConnected();
    dev.sendFeatureReport([REPORT_CMD, CMD_SAVE_CONFIG]);
  }
  // Triggers a USB soft-reset on the Pico — the device disappears briefly then
  // reappears. Caller is responsible for waiting for re-attach (via hotplug.ts).
  reconnectUsb() {
    const dev = this.assertConnected();
    dev.sendFeatureReport([REPORT_CMD, CMD_RECONNECT_USB]);
    this.disconnect();
  }
  // --- Informational reads (firmware extensions) ---
  readFirmwareVersion() {
    const dev = this.assertConnected();
    const raw = dev.getFeatureReport(REPORT_FIRMWARE_VERSION, 32);
    const bytes = raw.slice(1);
    const end = bytes.indexOf(0);
    return Buffer.from(end === -1 ? bytes : bytes.slice(0, end)).toString("ascii");
  }
  readRssi() {
    const dev = this.assertConnected();
    const raw = dev.getFeatureReport(REPORT_RSSI, 2);
    if (raw.length < 2) return null;
    const byte = raw[1];
    return byte > 127 ? byte - 256 : byte;
  }
}
const hapticsDongle = new DS5HapticsDevice();
class LinuxUpower {
  async read() {
    return new Promise((resolve) => {
      child_process.execFile("upower", ["-d"], { timeout: 3e3 }, (err, stdout) => {
        if (err || !stdout) {
          resolve(null);
          return;
        }
        const idx = stdout.indexOf("DualSense");
        if (idx === -1) {
          resolve(null);
          return;
        }
        const window = stdout.slice(idx, idx + 400);
        const match = window.match(/percentage:\s+(\d+)%/);
        resolve(match ? parseInt(match[1], 10) : null);
      });
    });
  }
}
class WindowsBattery {
  async read() {
    return null;
  }
}
class PresetStore {
  list() {
    return [];
  }
  load(_name) {
    throw new Error("not implemented");
  }
  save(_name, _config) {
    throw new Error("not implemented");
  }
  delete(_name) {
    throw new Error("not implemented");
  }
}
const presetStore = new PresetStore();
const batteryProvider = process.platform === "linux" ? new LinuxUpower() : new WindowsBattery();
function registerHandlers() {
  electron.ipcMain.handle(IPC.DEVICE_CONNECT, () => {
    const model = hapticsDongle.connect();
    let firmwareVersion;
    try {
      firmwareVersion = hapticsDongle.readFirmwareVersion();
    } catch {
    }
    return { model, firmwareVersion };
  });
  electron.ipcMain.handle(IPC.DEVICE_DISCONNECT, () => {
    hapticsDongle.disconnect();
  });
  electron.ipcMain.handle(IPC.DEVICE_STATUS, () => {
    let rssi = null;
    try {
      if (hapticsDongle.isConnected()) rssi = hapticsDongle.readRssi();
    } catch {
    }
    return {
      connected: hapticsDongle.isConnected(),
      model: hapticsDongle.model ?? void 0,
      rssi: rssi ?? void 0
    };
  });
  electron.ipcMain.handle(IPC.DEVICE_BATTERY, async () => {
    const percent = await batteryProvider.read();
    return { percent };
  });
  electron.ipcMain.handle(IPC.CONFIG_READ, () => {
    return hapticsDongle.readConfig();
  });
  electron.ipcMain.handle(IPC.CONFIG_WRITE, async (_event, cfg, reconnect) => {
    hapticsDongle.writeConfig(cfg);
    hapticsDongle.saveConfig();
    if (reconnect) {
      hapticsDongle.reconnectUsb();
      await new Promise((r) => setTimeout(r, 2500));
      hapticsDongle.connect();
      return hapticsDongle.readConfig();
    }
    return cfg;
  });
  electron.ipcMain.handle(IPC.CONFIG_SAVE, () => {
    hapticsDongle.saveConfig();
  });
  electron.ipcMain.handle(IPC.PRESETS_LIST, () => presetStore.list());
  electron.ipcMain.handle(IPC.PRESETS_LOAD, (_e, name) => presetStore.load(name));
  electron.ipcMain.handle(IPC.PRESETS_SAVE, (_e, name, cfg) => presetStore.save(name, cfg));
  electron.ipcMain.handle(IPC.PRESETS_DELETE, (_e, name) => presetStore.delete(name));
}
const TELEMETRY_INTERVAL_MS = 3e4;
function startTelemetryTimer(win) {
  setInterval(async () => {
    if (win.isDestroyed() || !hapticsDongle.isConnected()) return;
    const battery = await batteryProvider.read().catch(() => null);
    let rssi = null;
    try {
      rssi = hapticsDongle.readRssi();
    } catch {
    }
    const payload = { battery, rssi };
    win.webContents.send(IPC_EVENTS.DEVICE_TELEMETRY, payload);
  }, TELEMETRY_INTERVAL_MS);
}
const DEFAULT_WIDTH = 780;
const DEFAULT_HEIGHT = 900;
const MIN_WIDTH = 600;
const MIN_HEIGHT = 700;
function createWindow() {
  const { windowBounds } = loadSettings();
  const win = new electron.BrowserWindow({
    width: windowBounds?.width ?? DEFAULT_WIDTH,
    height: windowBounds?.height ?? DEFAULT_HEIGHT,
    x: windowBounds?.x,
    y: windowBounds?.y,
    minWidth: MIN_WIDTH,
    minHeight: MIN_HEIGHT,
    title: "DS5 Audio Haptics BT",
    webPreferences: {
      preload: path.join(__dirname, "../preload/index.js"),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true
    }
  });
  win.on("close", () => {
    saveSettings({ windowBounds: win.getBounds() });
  });
  if (process.env["ELECTRON_RENDERER_URL"]) {
    win.loadURL(process.env["ELECTRON_RENDERER_URL"]);
  } else {
    win.loadFile(path.join(__dirname, "../renderer/index.html"));
  }
  return win;
}
electron.app.whenReady().then(() => {
  const win = createWindow();
  registerHandlers();
  startTelemetryTimer(win);
});
electron.app.on("window-all-closed", () => {
  if (process.platform !== "darwin") electron.app.quit();
});
