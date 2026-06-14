import type { DeviceState } from '../hooks/useDevice';

interface HeaderProps extends DeviceState {
  onConnect: () => void;
  onDisconnect: () => void;
}

export default function Header({
  connected,
  connecting,
  model,
  firmwareVersion,
  battery,
  rssi,
  error,
  onConnect,
  onDisconnect,
}: HeaderProps) {
  return (
    <header className="header">
      <div className="header-left">
        <StatusDot connected={connected} connecting={connecting} />
        <span className="header-model">
          {connected ? model : connecting ? 'Connecting…' : 'Not connected'}
        </span>
        {firmwareVersion && (
          <span className="header-meta">fw {firmwareVersion}</span>
        )}
      </div>

      <div className="header-right">
        {connected && battery != null && (
          <BatteryBadge percent={battery} />
        )}
        {connected && rssi != null && (
          <span className="header-meta" title="Bluetooth signal (dBm)">
            📶 {rssi} dBm
          </span>
        )}
        {error && <span className="header-error">{error}</span>}
        <button
          className={`btn-connect ${connected ? 'btn-disconnect' : ''}`}
          onClick={connected ? onDisconnect : onConnect}
          disabled={connecting}
        >
          {connecting ? 'Connecting…' : connected ? 'Disconnect' : 'Connect'}
        </button>
      </div>
    </header>
  );
}

function StatusDot({ connected, connecting }: { connected: boolean; connecting: boolean }) {
  const cls = connecting ? 'dot dot--pulse' : connected ? 'dot dot--on' : 'dot dot--off';
  return <span className={cls} aria-label={connected ? 'Connected' : 'Disconnected'} />;
}

function BatteryBadge({ percent }: { percent: number }) {
  const color = percent > 50 ? 'var(--color-ok)' : percent > 20 ? 'var(--color-warn)' : 'var(--color-err)';
  return (
    <span className="battery-badge" style={{ color }} title={`Battery: ${percent}%`}>
      🔋 {percent}%
    </span>
  );
}
