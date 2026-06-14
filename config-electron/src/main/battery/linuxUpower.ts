// Battery readout via upower -d — port of hid_bridge.py read_battery_upower()

import { execFile } from 'child_process';
import type { BatteryProvider } from './BatteryProvider';

export class LinuxUpower implements BatteryProvider {
  async read(): Promise<number | null> {
    return new Promise((resolve) => {
      execFile('upower', ['-d'], { timeout: 3000 }, (err, stdout) => {
        if (err || !stdout) {
          resolve(null);
          return;
        }

        const idx = stdout.indexOf('DualSense');
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
