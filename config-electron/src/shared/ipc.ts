import type { DS5Config } from './config';
import type { DeviceStatus } from './protocol';
import type { PresetSummary } from './presets';

// Invoke channels — renderer calls, main handles, returns a Promise
export const IPC = {
  DEVICE_CONNECT:    'device:connect',
  DEVICE_DISCONNECT: 'device:disconnect',
  DEVICE_STATUS:     'device:status',
  DEVICE_BATTERY:    'device:battery',
  CONFIG_READ:       'config:read',
  CONFIG_WRITE:      'config:write',
  CONFIG_SAVE:       'config:save',
  PRESETS_LIST:      'presets:list',
  PRESETS_LOAD:      'presets:load',
  PRESETS_SAVE:      'presets:save',
  PRESETS_DELETE:    'presets:delete',
} as const;

// Event channels — main pushes to renderer (fire-and-forget)
export const IPC_EVENTS = {
  DEVICE_CHANGED:   'device:changed',
  DEVICE_TELEMETRY: 'device:telemetry',
} as const;

// Typed contract for every invoke channel
export interface IpcContract {
  [IPC.DEVICE_CONNECT]:    { args: [];                      result: { model: string; firmwareVersion?: string } };
  [IPC.DEVICE_DISCONNECT]: { args: [];                      result: void };
  [IPC.DEVICE_STATUS]:     { args: [];                      result: DeviceStatus };
  [IPC.DEVICE_BATTERY]:    { args: [];                      result: { percent: number | null } };
  [IPC.CONFIG_READ]:       { args: [];                      result: DS5Config };
  [IPC.CONFIG_WRITE]:      { args: [DS5Config, boolean];    result: DS5Config };
  [IPC.CONFIG_SAVE]:       { args: [];                      result: void };
  [IPC.PRESETS_LIST]:      { args: [];                      result: PresetSummary[] };
  [IPC.PRESETS_LOAD]:      { args: [string];                result: DS5Config };
  [IPC.PRESETS_SAVE]:      { args: [string, DS5Config];     result: void };
  [IPC.PRESETS_DELETE]:    { args: [string];                result: void };
}

// Event payload types
export interface DeviceChangedPayload {
  connected: boolean;
  model?: string;
}

export interface DeviceTelemetryPayload {
  battery?: number | null; // percent
  rssi?: number | null;    // dBm
}
