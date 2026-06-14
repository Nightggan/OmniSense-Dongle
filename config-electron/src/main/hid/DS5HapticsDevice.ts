// HID communication layer — port of config-app/hid_bridge.py
// Communicates with the DS5Dongle firmware running on Raspberry Pi Pico 2W.

import HID from 'node-hid';
import type { DS5Config } from '../../shared/config';
import type { ControllerModel } from '../../shared/protocol';
import {
  SONY_VID, DS5_PID, EDGE_PID,
  REPORT_READ_CONFIG, REPORT_CMD,
  REPORT_FIRMWARE_VERSION, REPORT_RSSI,
  CMD_WRITE_CONFIG, CMD_SAVE_CONFIG, CMD_RECONNECT_USB,
  CONFIG_SIZE,
} from '../../shared/protocol';
import { DS5DeviceError } from './errors';
import { packConfig, unpackConfig } from './protocol';

export class DS5HapticsDevice {
  private device: HID.HID | null = null;
  private _model: ControllerModel | null = null;

  // --- Connection management ---

  connect(): ControllerModel {
    const candidates: Array<[number, ControllerModel]> = [
      [DS5_PID,  'DualSense'],
      [EDGE_PID, 'DualSense Edge'],
    ];

    for (const [pid, label] of candidates) {
      const infos = HID.devices(SONY_VID, pid);
      if (infos.length === 0) continue;

      // Open by path — more reliable than VID/PID when multiple Sony HID devices coexist
      const path = infos[0].path;
      if (!path) continue;

      try {
        this.device = new HID.HID(path);
        this._model = label;
        return label;
      } catch {
        // try next candidate
      }
    }

    throw new DS5DeviceError(
      'NOT_FOUND',
      'DS5Dongle not found. Make sure the Pico is plugged in via USB.',
    );
  }

  disconnect(): void {
    if (this.device) {
      try { this.device.close(); } catch { /* device may already be gone */ }
      this.device = null;
      this._model = null;
    }
  }

  isConnected(): boolean {
    return this.device !== null;
  }

  get model(): ControllerModel | null {
    return this._model;
  }

  private assertConnected(): HID.HID {
    if (!this.device) {
      throw new DS5DeviceError('NOT_CONNECTED', 'Not connected to DS5Dongle.');
    }
    return this.device;
  }

  // --- Config read / write ---

  readConfig(): DS5Config {
    const dev = this.assertConnected();

    // getFeatureReport returns number[] where [0] is the report id byte.
    // Firmware fills the buffer starting from haptics_gain (skips config_version).
    const raw = dev.getFeatureReport(REPORT_READ_CONFIG, 1 + CONFIG_SIZE);
    const body = Buffer.from(raw.slice(1, 1 + CONFIG_SIZE));

    return unpackConfig(body);
  }

  writeConfig(cfg: DS5Config): void {
    const dev = this.assertConnected();

    // Feature report layout: [0xF6, 0x01, ...23 config bytes]
    const payload = Buffer.alloc(2 + CONFIG_SIZE);
    payload[0] = REPORT_CMD;
    payload[1] = CMD_WRITE_CONFIG;
    packConfig(cfg).copy(payload, 2);

    dev.sendFeatureReport([...payload]);
  }

  saveConfig(): void {
    const dev = this.assertConnected();
    dev.sendFeatureReport([REPORT_CMD, CMD_SAVE_CONFIG]);
  }

  // Triggers a USB soft-reset on the Pico — the device disappears briefly then
  // reappears. Caller is responsible for waiting for re-attach (via hotplug.ts).
  reconnectUsb(): void {
    const dev = this.assertConnected();
    dev.sendFeatureReport([REPORT_CMD, CMD_RECONNECT_USB]);
    this.disconnect(); // handle is immediately invalid after this command
  }

  // --- Informational reads (firmware extensions) ---

  readFirmwareVersion(): string {
    const dev = this.assertConnected();

    // 0xF8: firmware copies PICO_PROGRAM_VERSION_STRING into the buffer (ASCII, null-terminated)
    const raw = dev.getFeatureReport(REPORT_FIRMWARE_VERSION, 32);
    const bytes = raw.slice(1); // strip report id
    const end = bytes.indexOf(0);
    return Buffer.from(end === -1 ? bytes : bytes.slice(0, end)).toString('ascii');
  }

  readRssi(): number | null {
    const dev = this.assertConnected();

    // 0xF9: 1 byte, signed int8 in range [-128, 0] dBm
    const raw = dev.getFeatureReport(REPORT_RSSI, 2);
    if (raw.length < 2) return null;

    const byte = raw[1]; // unsigned 0–255
    return byte > 127 ? byte - 256 : byte; // reinterpret as signed int8
  }
}

// Singleton — one device handle shared across all IPC handlers
export const hapticsDongle = new DS5HapticsDevice();
