import { useState, useEffect, useCallback } from 'react';
import { ds5 } from '../ipc/client';
import { useConfigStore } from '../state/configStore';
import type { PresetSummary } from '../../shared/presets';

export interface PresetsHook {
  presets: PresetSummary[];
  load:    (name: string) => Promise<void>;
  save:    (name: string) => Promise<void>;
  remove:  (name: string) => Promise<void>;
  refresh: () => Promise<void>;
  error:   string | null;
}

export function usePresets(): PresetsHook {
  const [presets, setPresets] = useState<PresetSummary[]>([]);
  const [error, setError] = useState<string | null>(null);
  const { draft, setConfig } = useConfigStore();

  const refresh = useCallback(async () => {
    try {
      setPresets(await ds5.presets.list());
      setError(null);
    } catch { /* device not ready yet */ }
  }, []);

  useEffect(() => { refresh(); }, [refresh]);

  const load = useCallback(async (name: string) => {
    try {
      const cfg = await ds5.presets.load(name);
      setConfig(cfg);
      setError(null);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to load preset');
    }
  }, [setConfig]);

  const save = useCallback(async (name: string) => {
    if (!draft) { setError('No config loaded'); return; }
    try {
      await ds5.presets.save(name, draft);
      await refresh();
      setError(null);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to save preset');
    }
  }, [draft, refresh]);

  const remove = useCallback(async (name: string) => {
    try {
      await ds5.presets.delete(name);
      await refresh();
      setError(null);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to delete preset');
    }
  }, [refresh]);

  return { presets, load, save, remove, refresh, error };
}
