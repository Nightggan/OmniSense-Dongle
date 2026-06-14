import { useConfigStore } from '../../state/configStore';
import SliderRow from '../SliderRow';

export default function HapticsSection() {
  const { draft, updateField } = useConfigStore();
  if (!draft) return <></>;

  return (
    <div className="section-card">
      <p className="section-title">Haptics</p>
      <SliderRow
        label="Rumble gain"
        value={draft.hapticsGain}
        min={1.0} max={2.0} step={0.01}
        format={(v) => `${v.toFixed(2)}×`}
        onChange={(v) => updateField('hapticsGain', v)}
      />
    </div>
  );
}
