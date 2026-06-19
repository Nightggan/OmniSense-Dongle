import { useState, useEffect } from 'react';
import { ds5 } from '../ipc/client';

const RELEASES_URL = 'https://api.github.com/repos/loteran/DS5Dongle/releases';

interface UpdateInfo {
  version: string;
  url: string;
}

export function useUpdateCheck(): { latestVersion: string | null; releaseUrl: string | null } {
  const [update, setUpdate] = useState<UpdateInfo | null>(null);

  useEffect(() => {
    let cancelled = false;

    async function check(): Promise<void> {
      const current = await ds5.getVersion();
      const res = await fetch(RELEASES_URL, { signal: AbortSignal.timeout(10_000) });
      if (!res.ok) return;

      const releases = await res.json() as Array<{
        tag_name: string;
        html_url: string;
        draft: boolean;
        prerelease: boolean;
      }>;

      const appRelease = releases.find(
        r => !r.draft && !r.prerelease && r.tag_name.startsWith('app-v'),
      );
      if (!appRelease || cancelled) return;

      const latest = appRelease.tag_name.replace(/^app-v/, '');
      if (isNewer(latest, current)) {
        setUpdate({ version: latest, url: appRelease.html_url });
      }
    }

    check().catch(() => { /* network unavailable — silently skip */ });
    return () => { cancelled = true; };
  }, []);

  return update
    ? { latestVersion: update.version, releaseUrl: update.url }
    : { latestVersion: null, releaseUrl: null };
}

function isNewer(a: string, b: string): boolean {
  const parse = (v: string): number[] => v.split('.').map(n => parseInt(n, 10));
  const [am, an, ap] = parse(a);
  const [bm, bn, bp] = parse(b);
  return am > bm || (am === bm && an > bn) || (am === bm && an === bn && ap > bp);
}
