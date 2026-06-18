// Sony USB VID and DS5Dongle firmware PIDs
export const SONY_VID  = 0x054C;
export const DS5_PID   = 0x0CE6; // DualSense
export const EDGE_PID  = 0x0DF2; // DualSense Edge

// HID feature report IDs (from src/cmd.cpp)
export const REPORT_READ_CONFIG      = 0xF7;
export const REPORT_CMD              = 0xF6;
export const REPORT_FIRMWARE_VERSION = 0xF8;
export const REPORT_RSSI             = 0xF9;

// Sub-commands for REPORT_CMD (0xF6)
export const CMD_WRITE_CONFIG  = 0x01;
export const CMD_SAVE_CONFIG   = 0x02;
export const CMD_RECONNECT_USB = 0x03;

// Total size of the packed config struct (little-endian: ff BBBBBBBB H BBBBBBBB)
// f(4) f(4) B B B B B B B B H(2) B B B B B B B B = 26 bytes
export const CONFIG_SIZE = 26;

// Byte offsets within the 23-byte config buffer.
// Must stay in sync with the firmware struct in src/config.h.
// node-hid getFeatureReport includes the report id at [0], so caller
// must slice raw[1..1+CONFIG_SIZE] before passing here.
export const FIELD_OFFSETS = {
  hapticsGain:                 0,  // float32LE [0..3]
  speakerVolume:               4,  // float32LE [4..7]
  inactiveTime:                8,  // uint8
  disableInactiveDisconnect:   9,  // uint8 (bool)
  disablePicoLed:              10, // uint8 (bool)
  pollingRateMode:             11, // uint8
  audioBufferLength:           12, // uint8
  controllerMode:              13, // uint8
  autoHapticsEnable:           14, // uint8
  autoHapticsGain:             15, // uint8
  autoHapticsLowpassHz:        16, // uint16LE [16..17]
  enablePoweroffShortcut:      18, // uint8 (bool)
  enableTouchpad:              19, // uint8 (bool)
  poweroffButton:              20, // uint8
  touchpadButton:              21, // uint8
  batteryColorEnable:          22, // uint8 (bool)
  wakeEnable:                  23, // uint8 (bool)
  autoHapticsMuteReplace:      24, // uint8 (bool)
  autoHapticsMuteMix:          25, // uint8 (bool)
} as const;

export type ControllerModel = 'DualSense' | 'DualSense Edge';

export interface DeviceStatus {
  connected: boolean;
  model?: ControllerModel;
  firmwareVersion?: string; // from 0xF8 report
  rssi?: number;            // dBm, from 0xF9 report
}
