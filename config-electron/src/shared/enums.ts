export const BUTTON_NAMES = [
  'Square', 'Cross', 'Circle', 'Triangle',
  'L1', 'R1', 'Create', 'Options', 'Mute',
] as const;

export type ButtonIndex = 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8;

export const POLLING_RATE_LABELS = ['250 Hz', '500 Hz', '1000 Hz'] as const;

export const CONTROLLER_MODE_LABELS = ['DualSense', 'DualSense Edge', 'Auto'] as const;

export const AUTO_HAPTICS_LABELS = ['Off', 'Mix', 'Replace'] as const;
