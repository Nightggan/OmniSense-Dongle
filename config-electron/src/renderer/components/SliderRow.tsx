interface SliderRowProps {
  label: string;
  value: number;
  min: number;
  max: number;
  step?: number;
  format?: (v: number) => string;
  disabled?: boolean;
  onChange: (v: number) => void;
}

export default function SliderRow({
  label, value, min, max, step = 1, format, disabled, onChange,
}: SliderRowProps) {
  const display = format ? format(value) : String(value);
  return (
    <div className="row slider-row">
      <span className="row-label">{label}</span>
      <input
        className="slider"
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        disabled={disabled}
        onChange={(e) => onChange(Number(e.target.value))}
      />
      <span className="row-value">{display}</span>
    </div>
  );
}
