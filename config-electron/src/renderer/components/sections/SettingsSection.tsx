// Settings section — app-level preferences not tied to dongle config.
// Currently exposes the anonymous telemetry opt-in toggle.

import { useEffect, useState } from 'react';
import { ds5 } from '../../ipc/client';
import ToggleRow from '../ToggleRow';

export default function SettingsSection() {
  // null = never asked (consent dialog will handle it); treat as false for the toggle
  const [consent, setConsent] = useState<boolean | null>(null);
  const [loading, setLoading]  = useState(true);

  useEffect(() => {
    ds5.getTelemetryConsent()
      .then((v) => setConsent(v))
      .catch(() => setConsent(null))
      .finally(() => setLoading(false));
  }, []);

  async function handleToggle(value: boolean): Promise<void> {
    try {
      await ds5.setTelemetryConsent(value);
      setConsent(value);
    } catch { /* ignore — non-critical */ }
  }

  if (loading) return null;

  return (
    <div className="section-card">
      <p className="section-title">App Settings</p>
      <ToggleRow
        label="Anonymous usage statistics"
        description={
          'Share your OS and app version once a day — no personal data, ' +
          'no IP stored. Helps the maintainer understand which platforms to support.'
        }
        value={consent === true}
        onChange={handleToggle}
      />
    </div>
  );
}
