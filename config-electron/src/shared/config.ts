export interface DS5Config {
  hapticsGain: number;                // float [1.0–2.0]
  speakerVolume: number;              // float [-100–0] dB
  inactiveTime: number;               // uint8 [5–60] minutes
  disableInactiveDisconnect: boolean; // uint8
  disablePicoLed: boolean;            // uint8
  pollingRateMode: 0 | 1 | 2;        // uint8 — 250/500/1000 Hz
  audioBufferLength: number;          // uint8 [16–128]
  controllerMode: 0 | 1 | 2;         // uint8 — DS5/Edge/Auto
  autoHapticsEnable: 0 | 1 | 2;      // uint8 — Off/Mix/Replace
  autoHapticsGain: number;            // uint8 [0–200]
  autoHapticsLowpassHz: number;       // uint16 [20–400]
  enablePoweroffShortcut: boolean;    // uint8
  enableTouchpad: boolean;            // uint8
  poweroffButton: number;             // uint8 [0–8]
  touchpadButton: number;             // uint8 [0–8]
  batteryColorEnable: boolean;        // uint8
}

export const DEFAULTS: DS5Config = {
  hapticsGain: 1.0,
  speakerVolume: -100.0,
  inactiveTime: 30,
  disableInactiveDisconnect: false,
  disablePicoLed: false,
  pollingRateMode: 0,
  audioBufferLength: 64,
  controllerMode: 2,
  autoHapticsEnable: 2,
  autoHapticsGain: 100,
  autoHapticsLowpassHz: 80,
  enablePoweroffShortcut: true,
  enableTouchpad: true,
  poweroffButton: 3,  // Triangle
  touchpadButton: 2,  // Circle
  batteryColorEnable: true,
};

export interface FieldConstraint {
  min: number;
  max: number;
  step: number;
}

export const CONSTRAINTS: Partial<Record<keyof DS5Config, FieldConstraint>> = {
  hapticsGain:          { min: 1.0, max: 2.0,  step: 0.01 },
  speakerVolume:        { min: -100, max: 0,    step: 1 },
  inactiveTime:         { min: 5,   max: 60,    step: 1 },
  audioBufferLength:    { min: 16,  max: 128,   step: 1 },
  autoHapticsGain:      { min: 0,   max: 200,   step: 1 },
  autoHapticsLowpassHz: { min: 20,  max: 400,   step: 1 },
  poweroffButton:       { min: 0,   max: 8,     step: 1 },
  touchpadButton:       { min: 0,   max: 8,     step: 1 },
};

// These fields require USB reconnect after write (firmware resets the USB stack)
export const RECONNECT_FIELDS: ReadonlySet<keyof DS5Config> = new Set([
  'pollingRateMode',
  'controllerMode',
]);
