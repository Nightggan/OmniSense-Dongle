import { useConfigStore } from '../../state/configStore';
import ToggleRow from '../ToggleRow';

export default function LightbarSection() {
  const { draft, updateField } = useConfigStore();
  if (!draft) return <></>;

  return (
    <div className="section-card">
      <p className="section-title">Lightbar</p>
      <ToggleRow
        label="Battery level color"
        description="Lightbar color changes based on battery level (green → orange → red)"
        value={draft.batteryColorEnable}
        onChange={(v) => updateField('batteryColorEnable', v)}
      />
    </div>
  );
}
