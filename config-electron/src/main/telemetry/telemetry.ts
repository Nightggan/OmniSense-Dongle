// Opt-in anonymous telemetry — DS5Dongle Electron config app.
//
// Data sent (POST JSON):
//   { installation_id, platform, os, version, firmware }
//
// Privacy guarantees:
//   - Opt-in only (default: never asked → no data sent)
//   - installation_id is a salted SHA-256 hash of the machine-id — irreversible
//   - No IP address is stored server-side (Cloudflare Worker drops CF-Connecting-IP)
//   - Daily rate-limit (at most one ping per calendar day)
//   - All errors are silently swallowed — telemetry never crashes the app
//
// MAINTAINER: after deploying the Cloudflare Worker, replace TELEMETRY_ENDPOINT
// with the real workers.dev URL (e.g. https://ds5-telemetry.YOUR-NAME.workers.dev/collect)

import { app } from 'electron';
import { createHash, randomUUID } from 'crypto';
import { readFileSync } from 'fs';
import { execSync } from 'child_process';
import * as os from 'os';
import { loadSettings, saveSettings } from '../store/settings';

// ---------------------------------------------------------------------------
// Endpoint — replace this placeholder after deploying the Cloudflare Worker
// ---------------------------------------------------------------------------
export const TELEMETRY_ENDPOINT =
  'https://ds5-telemetry.PLACEHOLDER.workers.dev/collect';

// Salt used for the SHA-256 machine-id hash. Changing this would break
// continuity across installs; do not change without a migration plan.
const INSTALLATION_ID_SALT = 'ds5dongle-autohaptics';

// ---------------------------------------------------------------------------
// Consent helpers
// ---------------------------------------------------------------------------

/** Return true (opted in), false (declined), or null (never asked). */
export function getConsent(): boolean | null {
  const s = loadSettings();
  if (s.telemetryConsent === true)  return true;
  if (s.telemetryConsent === false) return false;
  return null; // undefined or null → never asked
}

/** Persist consent choice. */
export function setConsent(value: boolean): void {
  const s = loadSettings();
  s.telemetryConsent = value;
  saveSettings(s);
}

// ---------------------------------------------------------------------------
// Installation ID — salted SHA-256 of machine-id, fallback to random UUID
// ---------------------------------------------------------------------------

/** Read raw machine-id string for the current platform. Returns null on failure. */
function readRawMachineId(): string | null {
  const plat = process.platform;

  if (plat === 'linux') {
    // Primary: /etc/machine-id; fallback: /var/lib/dbus/machine-id
    for (const path of ['/etc/machine-id', '/var/lib/dbus/machine-id']) {
      try {
        const raw = readFileSync(path, 'utf8').trim();
        if (raw) return raw;
      } catch { /* try next */ }
    }
    return null;
  }

  if (plat === 'win32') {
    // Registry: HKLM\SOFTWARE\Microsoft\Cryptography → MachineGuid
    try {
      const out = execSync(
        'reg query "HKLM\\SOFTWARE\\Microsoft\\Cryptography" /v MachineGuid',
        { timeout: 3000, encoding: 'utf8' },
      );
      // Output line looks like:  "    MachineGuid    REG_SZ    <value>"
      const match = out.match(/MachineGuid\s+REG_SZ\s+(\S+)/i);
      if (match) return match[1];
    } catch { /* fall through */ }
    return null;
  }

  if (plat === 'darwin') {
    try {
      const out = execSync('ioreg -rd1 -c IOPlatformExpertDevice', {
        timeout: 3000,
        encoding: 'utf8',
      });
      const match = out.match(/"IOPlatformUUID"\s*=\s*"([^"]+)"/);
      if (match) return match[1];
    } catch { /* fall through */ }
    return null;
  }

  return null;
}

/** Return the salted SHA-256 hash of the machine-id (first 32 hex chars). */
function machineIdHash(): string | null {
  const raw = readRawMachineId();
  if (!raw) return null;
  return createHash('sha256')
    .update(`${INSTALLATION_ID_SALT}:${raw}`)
    .digest('hex')
    .slice(0, 32);
}

/** Stable anonymous installation ID. Prefers hashed machine-id; falls back to
 *  a random UUID persisted in settings. */
function getInstallationId(): string {
  const hashed = machineIdHash();
  if (hashed) return hashed;

  // Fallback: random UUID stored in settings so it survives app restarts
  const s = loadSettings();
  if (s.telemetryInstallId) return s.telemetryInstallId;

  const uuid = randomUUID();
  s.telemetryInstallId = uuid;
  saveSettings(s);
  return uuid;
}

// ---------------------------------------------------------------------------
// OS label helpers
// ---------------------------------------------------------------------------

/** Human-readable OS label for the current platform. */
function getOsLabel(): string {
  const plat = process.platform;

  if (plat === 'linux') {
    // Parse /etc/os-release — most distributions provide this
    try {
      const lines = readFileSync('/etc/os-release', 'utf8').split('\n');
      const fields: Record<string, string> = {};
      for (const line of lines) {
        const idx = line.indexOf('=');
        if (idx === -1) continue;
        const key = line.slice(0, idx).trim();
        const val = line.slice(idx + 1).trim().replace(/^"|"$/g, '');
        fields[key] = val;
      }
      if (fields['PRETTY_NAME']) return fields['PRETTY_NAME'];
      const name    = fields['NAME'] ?? '';
      const version = fields['VERSION'] ?? fields['VERSION_ID'] ?? '';
      if (name && version) return `${name} ${version}`;
      if (name) return name;
    } catch { /* fall through */ }
    return 'Unknown Linux';
  }

  if (plat === 'win32') {
    // os.version() → e.g. "Windows 11 Pro"
    // os.release() → build number string e.g. "10.0.22631"
    const ver   = os.version();    // "Windows 11 Pro" on Node ≥14
    const build = os.release();    // "10.0.22631"
    const buildNum = build.split('.').pop() ?? build;
    if (ver) return `${ver} (${buildNum})`;
    return `Windows (build ${buildNum})`;
  }

  if (plat === 'darwin') {
    return `macOS ${os.release()}`;
  }

  return `${process.platform} ${os.release()}`;
}

// ---------------------------------------------------------------------------
// Fire-and-forget send
// ---------------------------------------------------------------------------

export interface TelemetryPayload {
  installation_id: string;
  platform: string;
  os: string;
  version: string;
  firmware: string;
}

/** Blocking send — must be called in a non-blocking context (Promise). */
async function doSend(firmware: string): Promise<void> {
  const payload: TelemetryPayload = {
    installation_id: getInstallationId(),
    platform:        process.platform,
    os:              getOsLabel(),
    version:         app.getVersion(),
    firmware:        firmware || 'Unknown',
  };

  const body = JSON.stringify(payload);

  // Use global fetch (available in Electron 35 / Node 18+)
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 5000);
  try {
    await fetch(TELEMETRY_ENDPOINT, {
      method:  'POST',
      headers: { 'Content-Type': 'application/json' },
      body,
      signal:  controller.signal,
    });
    // Record today's date to enforce the daily rate-limit
    const s = loadSettings();
    s.telemetryLastSent = new Date().toISOString().slice(0, 10); // YYYY-MM-DD
    saveSettings(s);
  } finally {
    clearTimeout(timeout);
  }
}

/**
 * Fire-and-forget telemetry send.
 *
 * Silently skipped if:
 *  - consent is not exactly `true`
 *  - already sent today (daily rate-limit)
 *  - TELEMETRY_ENDPOINT is still the placeholder
 *  - network / endpoint unreachable
 */
export function maybeSend(opts: { firmware?: string } = {}): void {
  if (getConsent() !== true) return;

  // Do not send if the endpoint has not been configured by the maintainer
  if (TELEMETRY_ENDPOINT.includes('PLACEHOLDER')) return;

  // Daily rate-limit check
  const s = loadSettings();
  if (s.telemetryLastSent) {
    const today = new Date().toISOString().slice(0, 10);
    if (s.telemetryLastSent >= today) return;
  }

  const firmware = opts.firmware ?? 'Unknown';

  // Fire-and-forget: run in background, swallow all errors
  doSend(firmware).catch(() => { /* intentionally silent */ });
}
