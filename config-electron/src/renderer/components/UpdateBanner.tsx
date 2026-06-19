import { useState } from 'react';
import { ds5 } from '../ipc/client';
import { useUpdateCheck } from '../hooks/useUpdateCheck';

export default function UpdateBanner() {
  const { latestVersion, releaseUrl } = useUpdateCheck();
  const [dismissed, setDismissed] = useState(false);

  if (!latestVersion || !releaseUrl || dismissed) return null;

  return (
    <div className="update-banner">
      <span>Update available — v{latestVersion}</span>
      <button className="btn-primary btn-update" onClick={() => ds5.openUrl(releaseUrl)}>
        Download
      </button>
      <button className="btn-secondary btn-dismiss" onClick={() => setDismissed(true)}>
        ×
      </button>
    </div>
  );
}
