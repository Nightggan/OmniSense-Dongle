// Registers all ipcMain.handle channels and the periodic telemetry push.

import { app, ipcMain, BrowserWindow } from 'electron';
import type { DS5Config } from '../../shared/config';
import { RECONNECT_FIELDS } from '../../shared/config';
import { IPC, IPC_EVENTS } from '../../shared/ipc';
import type { DeviceTelemetryPayload } from '../../shared/ipc';
import { hapticsDongle } from '../hid/DS5HapticsDevice';
import { LinuxUpower } from '../battery/linuxUpower';
import { WindowsBattery } from '../battery/windowsBattery';
import type { BatteryProvider } from '../battery/BatteryProvider';
import { presetStore } from '../presets/PresetStore';
import { startHotplugWatcher, stopHotplugWatcher, waitForReattach } from '../hid/hotplug';

const batteryProvider: BatteryProvider = process.platform === 'linux'
  ? new LinuxUpower()
  : new WindowsBattery();

// --- IPC handlers ---

export function registerHandlers(): void {

  // Device connection
  ipcMain.handle(IPC.DEVICE_CONNECT, () => {
    const model = hapticsDongle.connect();
    let firmwareVersion: string | undefined;
    try { firmwareVersion = hapticsDongle.readFirmwareVersion(); } catch { /* optional */ }
    return { model, firmwareVersion };
  });

  ipcMain.handle(IPC.DEVICE_DISCONNECT, () => {
    hapticsDongle.disconnect();
  });

  ipcMain.handle(IPC.DEVICE_STATUS, () => {
    let rssi: number | null = null;
    try { if (hapticsDongle.isConnected()) rssi = hapticsDongle.readRssi(); } catch { /* ignore */ }
    return {
      connected: hapticsDongle.isConnected(),
      model: hapticsDongle.model ?? undefined,
      rssi: rssi ?? undefined,
    };
  });

  ipcMain.handle(IPC.DEVICE_BATTERY, async () => {
    const percent = await batteryProvider.read();
    return { percent };
  });

  // Config
  ipcMain.handle(IPC.CONFIG_READ, () => {
    return hapticsDongle.readConfig();
  });

  ipcMain.handle(IPC.CONFIG_WRITE, async (_event, cfg: DS5Config, reconnect: boolean) => {
    hapticsDongle.writeConfig(cfg);
    hapticsDongle.saveConfig();

    if (reconnect) {
      // reconnectUsb() triggers USB soft-reset; the handle is invalid immediately.
      hapticsDongle.reconnectUsb();
      // Wait for the device to re-enumerate instead of a blind sleep.
      await waitForReattach(5000);
      hapticsDongle.connect();
      return hapticsDongle.readConfig();
    }

    return cfg;
  });

  ipcMain.handle(IPC.CONFIG_SAVE, () => {
    hapticsDongle.saveConfig();
  });

  // Presets
  ipcMain.handle(IPC.PRESETS_LIST,   () => presetStore.list());
  ipcMain.handle(IPC.PRESETS_LOAD,   (_e, name: string) => presetStore.load(name));
  ipcMain.handle(IPC.PRESETS_SAVE,   (_e, name: string, cfg: DS5Config) => presetStore.save(name, cfg));
  ipcMain.handle(IPC.PRESETS_DELETE, (_e, name: string) => presetStore.delete(name));
}

// --- Telemetry push — 30 s interval, mirrors Python QTimer ---

const TELEMETRY_INTERVAL_MS = 30_000;

export function startTelemetryTimer(win: BrowserWindow): void {
  setInterval(async () => {
    if (win.isDestroyed() || !hapticsDongle.isConnected()) return;

    const battery = await batteryProvider.read().catch(() => null);

    let rssi: number | null = null;
    try { rssi = hapticsDongle.readRssi(); } catch { /* ignore */ }

    const payload: DeviceTelemetryPayload = { battery, rssi };
    win.webContents.send(IPC_EVENTS.DEVICE_TELEMETRY, payload);
  }, TELEMETRY_INTERVAL_MS);
}

// --- Hot-plug push — auto-connect/disconnect forwarded to renderer ---

export function setupHotplugEvents(win: BrowserWindow): void {
  startHotplugWatcher((event) => {
    if (win.isDestroyed()) return;

    if (event === 'attach') {
      try {
        const model = hapticsDongle.connect();
        let firmwareVersion: string | undefined;
        try { firmwareVersion = hapticsDongle.readFirmwareVersion(); } catch { /* optional */ }
        win.webContents.send(IPC_EVENTS.DEVICE_CHANGED, { connected: true, model, firmwareVersion });
      } catch {
        // Device attached but HID open failed (e.g. permission issue) — still notify as disconnected
        win.webContents.send(IPC_EVENTS.DEVICE_CHANGED, { connected: false });
      }
    } else {
      hapticsDongle.disconnect();
      win.webContents.send(IPC_EVENTS.DEVICE_CHANGED, { connected: false });
    }
  });

  app.on('quit', stopHotplugWatcher);
}

// Helper: detect if a config write requires USB reconnect
export function needsReconnect(prev: DS5Config, next: DS5Config): boolean {
  return ([...RECONNECT_FIELDS] as Array<keyof DS5Config>).some(
    (field) => prev[field] !== next[field],
  );
}
