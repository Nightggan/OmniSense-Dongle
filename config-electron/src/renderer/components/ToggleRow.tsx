interface ToggleRowProps {
  label: string;
  description?: string;
  value: boolean;
  disabled?: boolean;
  onChange: (v: boolean) => void;
}

export default function ToggleRow({
  label, description, value, disabled, onChange,
}: ToggleRowProps) {
  return (
    <div
      className={`row toggle-row ${disabled ? 'row--disabled' : ''}`}
      onClick={() => !disabled && onChange(!value)}
    >
      <div className="row-text">
        <span className="row-label">{label}</span>
        {description && <span className="row-desc">{description}</span>}
      </div>
      <button
        role="switch"
        aria-checked={value}
        className={`toggle ${value ? 'toggle--on' : ''}`}
        disabled={disabled}
        onClick={(e) => { e.stopPropagation(); onChange(!value); }}
      />
    </div>
  );
}
