import { useConfigStore } from '../../state/configStore';
import SelectRow from '../SelectRow';
import { CONTROLLER_MODE_LABELS, POLLING_RATE_LABELS } from '../../../shared/enums';

export default function ControllerSection() {
  const { draft, updateField } = useConfigStore();
  if (!draft) return <></>;

  return (
    <div className="section-card">
      <p className="section-title">Controller</p>
      <SelectRow
        label="Controller mode"
        description="Which DualSense profile to emulate"
        value={draft.controllerMode}
        options={CONTROLLER_MODE_LABELS}
        warn
        onChange={(v) => updateField('controllerMode', v as 0 | 1 | 2)}
      />
      <SelectRow
        label="Polling rate"
        description="USB report rate (higher = lower latency)"
        value={draft.pollingRateMode}
        options={POLLING_RATE_LABELS}
        warn
        onChange={(v) => updateField('pollingRateMode', v as 0 | 1 | 2)}
      />
    </div>
  );
}
