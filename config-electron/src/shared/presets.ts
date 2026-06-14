import type { DS5Config } from './config';

export const PRESET_SCHEMA_VERSION = 1 as const;

export interface PresetFile {
  _schema: typeof PRESET_SCHEMA_VERSION;
  name: string;
  savedAt: string; // ISO 8601
  config: DS5Config;
}

export interface PresetSummary {
  name: string;
  savedAt: string;
}

// Same validation rules as config-app/main.py
const PRESET_NAME_RE = /^[\w\s-]+$/;

export function isValidPresetName(name: string): boolean {
  return name.trim().length > 0 && PRESET_NAME_RE.test(name);
}

export function toSafeFilename(name: string): string {
  return name.trim().replace(/\s+/g, '_').toLowerCase();
}
