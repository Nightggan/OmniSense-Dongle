import { app } from 'electron';
import { mkdirSync, readdirSync, readFileSync, writeFileSync, unlinkSync, existsSync } from 'fs';
import { join } from 'path';
import type { DS5Config } from '../../shared/config';
import type { PresetSummary, PresetFile } from '../../shared/presets';
import { PRESET_SCHEMA_VERSION, toSafeFilename, isValidPresetName } from '../../shared/presets';

export class PresetStore {
  private presetsDir(): string {
    const dir = join(app.getPath('userData'), 'presets');
    mkdirSync(dir, { recursive: true });
    return dir;
  }

  private filePath(name: string): string {
    return join(this.presetsDir(), `${toSafeFilename(name)}.json`);
  }

  list(): PresetSummary[] {
    const dir = this.presetsDir();
    const summaries: PresetSummary[] = [];

    for (const entry of readdirSync(dir)) {
      if (!entry.endsWith('.json')) continue;
      try {
        const raw = readFileSync(join(dir, entry), 'utf-8');
        const file = JSON.parse(raw) as PresetFile;
        summaries.push({ name: file.name, savedAt: file.savedAt });
      } catch { /* skip corrupt files */ }
    }

    return summaries.sort((a, b) => b.savedAt.localeCompare(a.savedAt));
  }

  load(name: string): DS5Config {
    const raw = readFileSync(this.filePath(name), 'utf-8');
    const file = JSON.parse(raw) as PresetFile;
    return file.config;
  }

  save(name: string, config: DS5Config): void {
    if (!isValidPresetName(name)) throw new Error(`Invalid preset name: "${name}"`);

    const file: PresetFile = {
      _schema: PRESET_SCHEMA_VERSION,
      name,
      savedAt: new Date().toISOString(),
      config,
    };
    writeFileSync(this.filePath(name), JSON.stringify(file, null, 2), 'utf-8');
  }

  delete(name: string): void {
    const path = this.filePath(name);
    if (existsSync(path)) unlinkSync(path);
  }
}

export const presetStore = new PresetStore();
