import { app, BrowserWindow } from 'electron';
import { join } from 'path';
import { loadSettings, saveSettings } from './store/settings';
import { registerHandlers, startTelemetryTimer, setupHotplugEvents } from './ipc/registerHandlers';

const DEFAULT_WIDTH  = 780;
const DEFAULT_HEIGHT = 900;
const MIN_WIDTH      = 600;
const MIN_HEIGHT     = 700;

function createWindow(): BrowserWindow {
  const { windowBounds } = loadSettings();

  const win = new BrowserWindow({
    width:     windowBounds?.width  ?? DEFAULT_WIDTH,
    height:    windowBounds?.height ?? DEFAULT_HEIGHT,
    x:         windowBounds?.x,
    y:         windowBounds?.y,
    minWidth:  MIN_WIDTH,
    minHeight: MIN_HEIGHT,
    title:     'DS5 Audio Haptics BT',
    webPreferences: {
      preload:          join(__dirname, '../preload/index.js'),
      contextIsolation: true,
      nodeIntegration:  false,
      sandbox:          true,
    },
  });

  // Persist window geometry on close
  win.on('close', () => {
    saveSettings({ windowBounds: win.getBounds() });
  });

  // Load renderer
  if (process.env['ELECTRON_RENDERER_URL']) {
    win.loadURL(process.env['ELECTRON_RENDERER_URL']);
  } else {
    win.loadFile(join(__dirname, '../renderer/index.html'));
  }

  return win;
}

app.whenReady().then(() => {
  const win = createWindow();

  registerHandlers();
  startTelemetryTimer(win);
  setupHotplugEvents(win);
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});
