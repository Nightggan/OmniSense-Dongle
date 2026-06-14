import { useConfigStore } from '../../state/configStore';
import SliderRow from '../SliderRow';
import ToggleRow from '../ToggleRow';

export default function PowerSection() {
  const { draft, updateField } = useConfigStore();
  if (!draft) return <></>;

  return (
    <div className="section-card">
      <p className="section-title">Power</p>
      <SliderRow
        label="Auto power off"
        value={draft.inactiveTime}
        min={5} max={60} step={1}
        format={(v) => `${v} min`}
        disabled={draft.disableInactiveDisconnect}
        onChange={(v) => updateField('inactiveTime', v)}
      />
      <ToggleRow
        label="Stay connected"
        description="Disable auto power-off when idle"
        value={draft.disableInactiveDisconnect}
        onChange={(v) => updateField('disableInactiveDisconnect', v)}
      />
      <ToggleRow
        label="Disable Pico LED"
        description="Turn off the onboard LED on the Raspberry Pi Pico"
        value={draft.disablePicoLed}
        onChange={(v) => updateField('disablePicoLed', v)}
      />
    </div>
  );
}
