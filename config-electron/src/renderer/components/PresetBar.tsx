import { useState } from 'react';
import { usePresets } from '../hooks/usePresets';
import { useConfigStore } from '../state/configStore';
import { isValidPresetName } from '../../shared/presets';

interface PresetBarProps {
  connected: boolean;
}

export default function PresetBar({ connected }: PresetBarProps) {
  const { presets, load, save, remove, error } = usePresets();
  const { draft } = useConfigStore();
  const [selected, setSelected] = useState('');
  const [newName, setNewName] = useState('');
  const [busy, setBusy] = useState(false);

  async function handleLoad() {
    if (!selected) return;
    setBusy(true);
    await load(selected);
    setBusy(false);
  }

  async function handleDelete() {
    if (!selected) return;
    if (!confirm(`Delete preset "${selected}"?`)) return;
    setBusy(true);
    await remove(selected);
    if (selected === selected) setSelected('');
    setBusy(false);
  }

  async function handleSave() {
    const name = newName.trim();
    if (!name || !isValidPresetName(name)) return;
    setBusy(true);
    await save(name);
    setNewName('');
    setBusy(false);
  }

  const canLoad   = connected && !!selected && !!draft;
  const canDelete = !!selected;
  const canSave   = connected && !!draft && isValidPresetName(newName.trim());

  return (
    <div className="preset-bar">
      {/* Load/Delete group */}
      <select
        className="select-input preset-select"
        value={selected}
        onChange={(e) => setSelected(e.target.value)}
        disabled={presets.length === 0}
      >
        <option value="">— Presets —</option>
        {presets.map((p) => (
          <option key={p.name} value={p.name}>{p.name}</option>
        ))}
      </select>

      <button className="btn-secondary" disabled={!canLoad || busy} onClick={handleLoad}>
        Load
      </button>
      <button className="btn-secondary preset-del" disabled={!canDelete || busy} onClick={handleDelete}>
        ×
      </button>

      <div className="preset-divider" />

      {/* Save group */}
      <input
        className="preset-name-input"
        type="text"
        placeholder="Preset name…"
        maxLength={40}
        value={newName}
        onChange={(e) => setNewName(e.target.value)}
        onKeyDown={(e) => e.key === 'Enter' && handleSave()}
      />
      <button className="btn-primary" disabled={!canSave || busy} onClick={handleSave}>
        Save
      </button>

      {error && <span className="preset-error" title={error}>⚠</span>}
    </div>
  );
}
