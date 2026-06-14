// Preset CRUD on disk — ~/.config/ds5dongle/presets/ (Linux) or %APPDATA% (Windows)
// TODO: implement in step 6

import type { DS5Config } from '../../shared/config';
import type { PresetSummary } from '../../shared/presets';

export class PresetStore {
  list(): PresetSummary[] { return []; }
  load(_name: string): DS5Config { throw new Error('not implemented'); }
  save(_name: string, _config: DS5Config): void { throw new Error('not implemented'); }
  delete(_name: string): void { throw new Error('not implemented'); }
}

export const presetStore = new PresetStore();
