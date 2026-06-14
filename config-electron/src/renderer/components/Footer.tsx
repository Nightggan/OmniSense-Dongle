import { useState } from 'react';
import { ds5 } from '../ipc/client';
import { useConfigStore, useIsDirty, useNeedsReconnect } from '../state/configStore';

interface FooterProps {
  connected: boolean;
  onRead: () => void;
}

export default function Footer({ connected, onRead }: FooterProps) {
  const { draft, setConfig } = useConfigStore();
  const isDirty       = useIsDirty();
  const needsReconnect = useNeedsReconnect();
  const [saving, setSaving] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);

  async function handleSave(): Promise<void> {
    if (!draft) return;
    setSaving(true);
    setSaveError(null);
    try {
      // Main process writes + saves to flash + reconnects USB if needed
      const result = await ds5.writeConfig(draft, needsReconnect);
      setConfig(result); // update origin → marks as clean
    } catch (err) {
      setSaveError(err instanceof Error ? err.message : 'Save failed');
    } finally {
      setSaving(false);
    }
  }

  return (
    <footer className="footer">
      <div className="footer-left">
        {needsReconnect && isDirty && (
          <span className="footer-warn" title="Polling rate or controller mode changed">
            ⚡ USB reconnect required
          </span>
        )}
        {saveError && (
          <span className="footer-error">{saveError}</span>
        )}
      </div>
      <div className="footer-right">
        <button
          className="btn-secondary"
          disabled={!connected || saving}
          onClick={onRead}
        >
          Read Config
        </button>
        <button
          className="btn-primary"
          disabled={!connected || !isDirty || saving}
          onClick={handleSave}
        >
          {saving ? (needsReconnect ? 'Reconnecting…' : 'Saving…') : 'Save to Device'}
        </button>
      </div>
    </footer>
  );
}
