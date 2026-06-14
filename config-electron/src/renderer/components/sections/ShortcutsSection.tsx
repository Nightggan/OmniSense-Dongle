import { useConfigStore } from '../../state/configStore';
import ToggleRow from '../ToggleRow';
import SelectRow from '../SelectRow';
import { BUTTON_NAMES } from '../../../shared/enums';

export default function ShortcutsSection() {
  const { draft, updateField } = useConfigStore();
  if (!draft) return <></>;

  return (
    <div className="section-card">
      <p className="section-title">Button Shortcuts</p>

      <ToggleRow
        label="Power off shortcut"
        description="Hold PS + button to turn off the controller"
        value={draft.enablePoweroffShortcut}
        onChange={(v) => updateField('enablePoweroffShortcut', v)}
      />
      <SelectRow
        label="Power off button"
        value={draft.poweroffButton}
        options={BUTTON_NAMES}
        disabled={!draft.enablePoweroffShortcut}
        onChange={(v) => updateField('poweroffButton', v)}
      />

      <div className="row-separator" />

      <ToggleRow
        label="Touchpad enabled"
        description="Enable the touchpad by default"
        value={draft.enableTouchpad}
        onChange={(v) => updateField('enableTouchpad', v)}
      />
      <SelectRow
        label="Touchpad toggle button"
        description="Hold PS + button to toggle touchpad on/off"
        value={draft.touchpadButton}
        options={BUTTON_NAMES}
        onChange={(v) => updateField('touchpadButton', v)}
      />
    </div>
  );
}
