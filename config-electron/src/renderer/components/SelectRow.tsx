interface SelectRowProps {
  label: string;
  description?: string;
  value: number;
  options: readonly string[];
  disabled?: boolean;
  warn?: boolean; // shows reconnect badge
  onChange: (v: number) => void;
}

export default function SelectRow({
  label, description, value, options, disabled, warn, onChange,
}: SelectRowProps) {
  return (
    <div className="row select-row">
      <div className="row-text">
        <span className="row-label">
          {label}
          {warn && <span className="reconnect-badge" title="Changing this requires USB reconnect">⚡</span>}
        </span>
        {description && <span className="row-desc">{description}</span>}
      </div>
      <select
        className="select-input"
        value={value}
        disabled={disabled}
        onChange={(e) => onChange(Number(e.target.value))}
      >
        {options.map((opt, i) => (
          <option key={i} value={i}>{opt}</option>
        ))}
      </select>
    </div>
  );
}
