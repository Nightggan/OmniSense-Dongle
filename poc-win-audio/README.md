# DS5 dongle — Windows loopback POC (throwaway)

Goal: validate that we can replace **VoiceMeeter** on Windows by mirroring the
**current default output** (headset / HDMI / speakers — whatever the user picked)
to the DS5 dongle's USB sound card directly from Node, and measure the latency.

The default output keeps playing normally; we only *duplicate* the stream to the
dongle. No reconfiguring the default device (unlike VoiceMeeter).

## Run (on Windows)

```powershell
cd poc-win-audio
npm install
node loopback-poc.js
```

- Auto-detects the **default output** (loopback source) and the **dongle**
  (matched by name `dualsense`).
- Override the dongle match: `node loopback-poc.js "controller"`
- Tune latency vs stability: `set FRAME=240 && node loopback-poc.js` (240 = 5 ms).

Then play any audio and feel the controller. Press `Ctrl+C` to stop.

## Reading the result

The script prints a per-second line:

- `in/out` = software stream latency reported by RtAudio (capture + render buffers).
- `SW pipeline` = sum = the part *we* control in the app.
- If `getStreamLatency` isn't exposed on the build, only the physical test counts.

**The number that matters** is the *physical* latency: time between hearing the
sound and feeling the vibration. The USB + Bluetooth + firmware path is fixed and
not measurable in software — judge it by feel, or record phone-mic audio + a felt
tap and diff the timestamps. If felt latency is comparable to VoiceMeeter, the
approach is viable for the Electron app.

## Notes / gotchas

- **Windows only** for real loopback. On Linux the script only enumerates devices
  (different loopback mechanism — Linux already uses `pw-loopback`, untouched).
- Don't set the dongle as the default output, or we'd capture the dongle itself
  (feedback loop) — the script guards against this.
- If you hear dropouts/underruns, raise `FRAME` (e.g. 960 = 20 ms).
