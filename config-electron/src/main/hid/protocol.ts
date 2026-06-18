// Pack/unpack between DS5Config and the 23-byte HID buffer.
// Implements the same logic as hid_bridge.py pack/unpack, in TypeScript.
// TODO: implement in step 2

import type { DS5Config } from '../../shared/config';
import { DEFAULTS } from '../../shared/config';
import { CONFIG_SIZE, FIELD_OFFSETS } from '../../shared/protocol';

export function unpackConfig(raw: Buffer): DS5Config {
  // raw must be exactly CONFIG_SIZE bytes (report id already stripped by caller)
  if (raw.length < CONFIG_SIZE) {
    raw = Buffer.concat([raw, Buffer.alloc(CONFIG_SIZE - raw.length)]);
  }

  const o = FIELD_OFFSETS;
  const cfg: DS5Config = {
    hapticsGain:               Math.min(2.0, Math.max(1.0, raw.readFloatLE(o.hapticsGain))),
    speakerVolume:             Math.min(0, Math.max(-100, raw.readFloatLE(o.speakerVolume))),
    inactiveTime:              raw.readUInt8(o.inactiveTime),
    disableInactiveDisconnect: raw.readUInt8(o.disableInactiveDisconnect) !== 0,
    disablePicoLed:            raw.readUInt8(o.disablePicoLed) !== 0,
    pollingRateMode:           raw.readUInt8(o.pollingRateMode) as 0 | 1 | 2,
    audioBufferLength:         raw.readUInt8(o.audioBufferLength),
    controllerMode:            raw.readUInt8(o.controllerMode) as 0 | 1 | 2,
    autoHapticsEnable:         raw.readUInt8(o.autoHapticsEnable) as 0 | 1 | 2,
    autoHapticsGain:           raw.readUInt8(o.autoHapticsGain),
    autoHapticsLowpassHz:      raw.readUInt16LE(o.autoHapticsLowpassHz),
    enablePoweroffShortcut:    raw.readUInt8(o.enablePoweroffShortcut) !== 0,
    enableTouchpad:            raw.readUInt8(o.enableTouchpad) !== 0,
    poweroffButton:            raw.readUInt8(o.poweroffButton),
    touchpadButton:            raw.readUInt8(o.touchpadButton),
    batteryColorEnable:        raw.readUInt8(o.batteryColorEnable) !== 0,
    wakeEnable:                raw.readUInt8(o.wakeEnable) !== 0,
    autoHapticsMuteReplace:    raw.readUInt8(o.autoHapticsMuteReplace) !== 0,
    autoHapticsMuteMix:        raw.readUInt8(o.autoHapticsMuteMix) !== 0,
  };

  return cfg;
}

export function packConfig(cfg: DS5Config): Buffer {
  const buf = Buffer.alloc(CONFIG_SIZE);
  const o = FIELD_OFFSETS;

  buf.writeFloatLE(cfg.hapticsGain,           o.hapticsGain);
  buf.writeFloatLE(cfg.speakerVolume,          o.speakerVolume);
  buf.writeUInt8(cfg.inactiveTime,             o.inactiveTime);
  buf.writeUInt8(cfg.disableInactiveDisconnect ? 1 : 0, o.disableInactiveDisconnect);
  buf.writeUInt8(cfg.disablePicoLed ? 1 : 0,  o.disablePicoLed);
  buf.writeUInt8(cfg.pollingRateMode,          o.pollingRateMode);
  buf.writeUInt8(cfg.audioBufferLength,        o.audioBufferLength);
  buf.writeUInt8(cfg.controllerMode,           o.controllerMode);
  buf.writeUInt8(cfg.autoHapticsEnable,        o.autoHapticsEnable);
  buf.writeUInt8(cfg.autoHapticsGain,          o.autoHapticsGain);
  buf.writeUInt16LE(cfg.autoHapticsLowpassHz,  o.autoHapticsLowpassHz);
  buf.writeUInt8(cfg.enablePoweroffShortcut ? 1 : 0, o.enablePoweroffShortcut);
  buf.writeUInt8(cfg.enableTouchpad ? 1 : 0,  o.enableTouchpad);
  buf.writeUInt8(cfg.poweroffButton,           o.poweroffButton);
  buf.writeUInt8(cfg.touchpadButton,           o.touchpadButton);
  buf.writeUInt8(cfg.batteryColorEnable ? 1 : 0,      o.batteryColorEnable);
  buf.writeUInt8(cfg.wakeEnable ? 1 : 0,              o.wakeEnable);
  buf.writeUInt8(cfg.autoHapticsMuteReplace ? 1 : 0,  o.autoHapticsMuteReplace);
  buf.writeUInt8(cfg.autoHapticsMuteMix ? 1 : 0,      o.autoHapticsMuteMix);

  return buf;
}

export { DEFAULTS };
