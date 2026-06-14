// Battery readout on Windows — stub, returns null until implemented
// Will read from HID input report or WMI in a future step

import type { BatteryProvider } from './BatteryProvider';

export class WindowsBattery implements BatteryProvider {
  async read(): Promise<number | null> {
    return null;
  }
}
