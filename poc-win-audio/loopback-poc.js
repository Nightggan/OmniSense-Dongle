/*
 * DS5 dongle — Windows WASAPI loopback POC
 * ----------------------------------------
 * Goal: replace VoiceMeeter. Capture whatever the user's CURRENT default
 * output device is playing (headset, HDMI, speakers...) and mirror it to the
 * DS5 dongle's USB sound card, so the firmware turns it into haptics.
 * The default output keeps playing normally — we only *duplicate* to the dongle.
 *
 * How loopback works here: with RtAudio/WASAPI, opening an INPUT stream whose
 * deviceId points at an OUTPUT (render) device switches that stream to loopback
 * capture. So we capture the default render device, not a microphone.
 *
 * The dongle exposes a fixed format (see usb_descriptors.cpp): 48 kHz, 16-bit,
 * stereo (the 2ch alt interface). WASAPI shared mode auto-resamples the default
 * device's mix to 48 kHz for us, so any default rate (44.1k, 48k...) is fine.
 *
 * Usage (on Windows):
 *   npm install
 *   node loopback-poc.js                 # auto-detect dongle by name "dualsense"
 *   node loopback-poc.js "controller"    # override dongle name match (substring)
 *   FRAME=240 node loopback-poc.js       # tune frame size (samples/ch) for latency
 */

const { RtAudio, RtAudioFormat, RtAudioApi, RtAudioStreamFlags } = require('audify');

// --- Fixed dongle format ---
const SAMPLE_RATE = 48000;                 // dongle is locked to 48 kHz
const CHANNELS = 2;                        // use the stereo (2ch) streaming interface
const FORMAT = RtAudioFormat.RTAUDIO_SINT16; // 16-bit, matches bBitResolution=0x10
const BYTES_PER_FRAME = CHANNELS * 2;      // 2 ch * 2 bytes (S16)

// Frame size = samples per channel per callback. Smaller = lower latency but
// higher CPU and more risk of dropouts/underruns. 480 = 10 ms @ 48 kHz.
const FRAME_SIZE = Number(process.env.FRAME || 480);

// Dongle is matched by name substring; it enumerates as "DualSense Wireless Controller".
const DONGLE_MATCH = (process.argv[2] || process.env.DONGLE || 'dualsense').toLowerCase();

function pickApi() {
  if (process.platform === 'win32') return RtAudioApi.WINDOWS_WASAPI;
  // On Linux this POC won't do real loopback (different mechanism); used only to
  // sanity-check device enumeration / script wiring during development.
  return undefined;
}

const api = pickApi();
// audify rejects an explicit `undefined`; use the no-arg default API instead.
const makeRt = () => (api !== undefined ? new RtAudio(api) : new RtAudio());
const rtIn = makeRt();
const rtOut = makeRt();

// --- Enumerate devices ---
const devices = rtIn.getDevices();
console.log('\n=== Audio devices ===');
for (const d of devices) {
  const tags = [];
  if (d.isDefaultOutput) tags.push('DEFAULT-OUT');
  if (d.isDefaultInput) tags.push('DEFAULT-IN');
  console.log(
    `#${String(d.id).padStart(2)}  out:${d.outputChannels} in:${d.inputChannels}  ` +
    `${tags.join(' ').padEnd(22)} ${d.name}`
  );
}

const defOutId = rtIn.getDefaultOutputDevice();
const defOut = devices.find((d) => d.id === defOutId);
const dongle = devices.find(
  (d) => d.outputChannels > 0 && d.name.toLowerCase().includes(DONGLE_MATCH)
);

if (!defOut) {
  console.error('\n[!] No default output device found.');
  process.exit(1);
}
if (!dongle) {
  console.error(`\n[!] Dongle not found (name match "${DONGLE_MATCH}").`);
  console.error('    Pass a substring of its name as the first argument.');
  process.exit(1);
}
if (dongle.id === defOut.id) {
  console.error('\n[!] The dongle IS the default output. Set your headset/HDMI as default,');
  console.error('    otherwise we would capture the dongle itself (feedback loop).');
  process.exit(1);
}

console.log(`\nCapturing LOOPBACK of default output : #${defOut.id} ${defOut.name}`);
console.log(`Mirroring to dongle                  : #${dongle.id} ${dongle.name}`);
console.log(
  `Format: ${SAMPLE_RATE} Hz / ${CHANNELS}ch / S16 | ` +
  `frame=${FRAME_SIZE} (${((FRAME_SIZE / SAMPLE_RATE) * 1000).toFixed(1)} ms)\n`
);

// --- Stats ---
let framesIn = 0;
let underruns = 0;
const t0 = Date.now();

const errCb = (tag) => (err) => console.error(`[${tag}] stream error:`, err);

// --- OUTPUT stream: to the dongle. audify drives output via write() into an
//     internal queue, so we just push captured frames in the input callback. ---
rtOut.openStream(
  { deviceId: dongle.id, nChannels: CHANNELS, firstChannel: 0 },
  null,
  FORMAT,
  SAMPLE_RATE,
  FRAME_SIZE,
  'ds5-dongle-out',
  null,
  null,
  RtAudioStreamFlags.RTAUDIO_MINIMIZE_LATENCY,
  errCb('OUT')
);
rtOut.start();

// --- INPUT stream: loopback capture of the default OUTPUT device. ---
rtIn.openStream(
  null,
  { deviceId: defOut.id, nChannels: CHANNELS, firstChannel: 0 },
  FORMAT,
  SAMPLE_RATE,
  FRAME_SIZE,
  'ds5-loopback-in',
  (inputBuf) => {
    // Copy: RtAudio reuses the underlying buffer between callbacks.
    rtOut.write(Buffer.from(inputBuf));
    framesIn += FRAME_SIZE;
  },
  null,
  RtAudioStreamFlags.RTAUDIO_MINIMIZE_LATENCY,
  errCb('IN')
);
rtIn.start();

// --- Latency reporting ---
function latMs(rt) {
  try {
    const f = rt.getStreamLatency(); // frames
    if (typeof f === 'number' && f > 0) return (f / SAMPLE_RATE) * 1000;
  } catch (_) { /* not exposed on this build */ }
  return null;
}

setInterval(() => {
  const inL = latMs(rtIn);
  const outL = latMs(rtOut);
  const reported = [inL, outL].filter((x) => x != null);
  const sw = reported.length ? reported.reduce((a, b) => a + b, 0) : null;
  const secs = (Date.now() - t0) / 1000;
  const fps = (framesIn / SAMPLE_RATE / secs) * 100; // % of realtime captured

  let line = `[+${secs.toFixed(0)}s] captured=${(fps).toFixed(0)}% rt`;
  if (inL != null) line += ` | in=${inL.toFixed(1)}ms`;
  if (outL != null) line += ` | out=${outL.toFixed(1)}ms`;
  if (sw != null) line += ` | SW pipeline ~${sw.toFixed(1)}ms`;
  else line += ` | (getStreamLatency unavailable — use the physical test below)`;
  console.log(line);
}, 1000);

console.log('Running. Play audio on your default output and feel the dongle vibrate.');
console.log('Physical latency = the delay you actually care about (audio heard -> vibration felt).');
console.log('Measure it by recording phone-mic + a felt tap, or just judge by feel. Ctrl+C to stop.\n');

process.on('SIGINT', () => {
  console.log(`\nStopping. underruns=${underruns}`);
  try { rtIn.closeStream(); } catch (_) {}
  try { rtOut.closeStream(); } catch (_) {}
  process.exit(0);
});
