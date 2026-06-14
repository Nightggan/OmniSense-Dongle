import { create } from 'zustand';
import type { DS5Config } from '../../shared/config';
import { RECONNECT_FIELDS } from '../../shared/config';

interface ConfigStore {
  original: DS5Config | null; // authoritative state (last read from / saved to device)
  draft:    DS5Config | null; // current edits
  setConfig:   (cfg: DS5Config) => void; // sets both original and a fresh copy as draft
  clearConfig: () => void;
  updateField: <K extends keyof DS5Config>(key: K, value: DS5Config[K]) => void;
}

export const useConfigStore = create<ConfigStore>()((set) => ({
  original: null,
  draft:    null,

  setConfig: (cfg) => set({ original: cfg, draft: { ...cfg } }),
  clearConfig: () => set({ original: null, draft: null }),

  updateField: (key, value) =>
    set((s) => s.draft ? { draft: { ...s.draft, [key]: value } } : s),
}));

// Derived selectors — computed on every render, zero boilerplate
export function useIsDirty(): boolean {
  return useConfigStore((s) => {
    if (!s.original || !s.draft) return false;
    return (Object.keys(s.draft) as Array<keyof DS5Config>).some(
      (k) => (s.draft as DS5Config)[k] !== (s.original as DS5Config)[k],
    );
  });
}

export function useNeedsReconnect(): boolean {
  return useConfigStore((s) => {
    if (!s.original || !s.draft) return false;
    return ([...RECONNECT_FIELDS] as Array<keyof DS5Config>).some(
      (k) => (s.draft as DS5Config)[k] !== (s.original as DS5Config)[k],
    );
  });
}
