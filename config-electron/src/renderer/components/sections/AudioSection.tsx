import { useConfigStore } from '../../state/configStore';
import SliderRow from '../SliderRow';

export default function AudioSection() {
  const { draft, updateField } = useConfigStore();
  if (!draft) return <></>;

  return (
    <div className="section-card">
      <p className="section-title">Audio</p>
      <SliderRow
        label="Speaker volume"
        value={draft.speakerVolume}
        min={-100} max={0} step={1}
        format={(v) => `${v} dB`}
        onChange={(v) => updateField('speakerVolume', v)}
      />
      <SliderRow
        label="Audio buffer"
        value={draft.audioBufferLength}
        min={16} max={128} step={1}
        format={(v) => String(v)}
        onChange={(v) => updateField('audioBufferLength', v)}
      />
    </div>
  );
}
