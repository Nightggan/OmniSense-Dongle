import { useConfigStore } from '../../state/configStore';
import SelectRow from '../SelectRow';
import SliderRow from '../SliderRow';
import ToggleRow from '../ToggleRow';
import { AUTO_HAPTICS_LABELS } from '../../../shared/enums';
import { useLoopbackStatus } from '../../hooks/useLoopback';
import { ds5 } from '../../ipc/client';

export default function AutoHapticsSection() {
  const { draft, updateField } = useConfigStore();
  const loopback = useLoopbackStatus();
  if (!draft) return <></>;

  const active = draft.autoHapticsEnable !== 0;
  const isWindows = ds5.platform === 'win32';

  return (
    <div className="section-card">
      <p className="section-title">Auto Haptics</p>
      <SelectRow
        label="Mode"
        description="Convert audio output into controller vibrations"
        value={draft.autoHapticsEnable}
        options={AUTO_HAPTICS_LABELS}
        onChange={(v) => updateField('autoHapticsEnable', v as 0 | 1 | 2)}
      />
      <SliderRow
        label="Intensity"
        value={draft.autoHapticsGain}
        min={0} max={200} step={1}
        format={(v) => `${v}%`}
        disabled={!active}
        onChange={(v) => updateField('autoHapticsGain', v)}
      />
      <SliderRow
        label="Low-pass cutoff"
        value={draft.autoHapticsLowpassHz}
        min={20} max={400} step={1}
        format={(v) => `${v} Hz`}
        disabled={!active}
        onChange={(v) => updateField('autoHapticsLowpassHz', v)}
      />
      <ToggleRow
        label="Auto-mute speaker (Replace)"
        description="Mute speaker output when Replace mode is active"
        value={draft.autoHapticsMuteReplace}
        disabled={draft.autoHapticsEnable !== 2}
        onChange={(v) => updateField('autoHapticsMuteReplace', v)}
      />
      <ToggleRow
        label="Auto-mute speaker (Mix)"
        description="Mute speaker output when Mix mode is active"
        value={draft.autoHapticsMuteMix}
        disabled={draft.autoHapticsEnable !== 1}
        onChange={(v) => updateField('autoHapticsMuteMix', v)}
      />
      {isWindows && <LoopbackBanner status={loopback} />}
    </div>
  );
}

function LoopbackBanner({ status }: { status: ReturnType<typeof useLoopbackStatus> }) {
  if (!status) return null;

  const dot = status.error
    ? 'loopback-dot loopback-dot--error'
    : status.running
      ? 'loopback-dot loopback-dot--ok'
      : 'loopback-dot loopback-dot--idle';

  const label = status.error
    ? `Error: ${status.error}`
    : status.running
      ? status.deviceName ?? 'Running'
      : 'Stopped';

  return (
    <div className="loopback-banner">
      <span className={dot} />
      <span className="loopback-label">Audio source: {label}</span>
    </div>
  );
}
