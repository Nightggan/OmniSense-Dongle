// Secure bridge between renderer and main process.
// contextBridge ensures the renderer never sees raw ipcRenderer.

import { contextBridge, ipcRenderer } from 'electron';
import { IPC, IPC_EVENTS } from '../shared/ipc';
import type { DS5Config } from '../shared/config';
import type { DeviceChangedPayload, DeviceTelemetryPayload } from '../shared/ipc';

contextBridge.exposeInMainWorld('ds5', {
  // Device
  connect:    () => ipcRenderer.invoke(IPC.DEVICE_CONNECT),
  disconnect: () => ipcRenderer.invoke(IPC.DEVICE_DISCONNECT),
  getStatus:  () => ipcRenderer.invoke(IPC.DEVICE_STATUS),
  getBattery: () => ipcRenderer.invoke(IPC.DEVICE_BATTERY),

  // Config
  readConfig:  () => ipcRenderer.invoke(IPC.CONFIG_READ),
  writeConfig: (cfg: DS5Config, reconnect: boolean) =>
    ipcRenderer.invoke(IPC.CONFIG_WRITE, cfg, reconnect),
  saveConfig:  () => ipcRenderer.invoke(IPC.CONFIG_SAVE),

  // Presets
  presets: {
    list:   ()                             => ipcRenderer.invoke(IPC.PRESETS_LIST),
    load:   (name: string)                 => ipcRenderer.invoke(IPC.PRESETS_LOAD, name),
    save:   (name: string, cfg: DS5Config) => ipcRenderer.invoke(IPC.PRESETS_SAVE, name, cfg),
    delete: (name: string)                 => ipcRenderer.invoke(IPC.PRESETS_DELETE, name),
  },

  // Push events — return a cleanup function so useEffect can unsubscribe on unmount
  onDeviceChanged: (cb: (payload: DeviceChangedPayload) => void): (() => void) => {
    const listener = (_e: Electron.IpcRendererEvent, p: DeviceChangedPayload) => cb(p);
    ipcRenderer.on(IPC_EVENTS.DEVICE_CHANGED, listener);
    return () => ipcRenderer.removeListener(IPC_EVENTS.DEVICE_CHANGED, listener);
  },
  onTelemetry: (cb: (payload: DeviceTelemetryPayload) => void): (() => void) => {
    const listener = (_e: Electron.IpcRendererEvent, p: DeviceTelemetryPayload) => cb(p);
    ipcRenderer.on(IPC_EVENTS.DEVICE_TELEMETRY, listener);
    return () => ipcRenderer.removeListener(IPC_EVENTS.DEVICE_TELEMETRY, listener);
  },
});
