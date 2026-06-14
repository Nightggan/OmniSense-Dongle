// USB hot-plug detection via node-usb.
// Fires callbacks when the DS5Dongle dongle is plugged/unplugged.
// TODO: implement in step 6

export type HotplugCallback = (event: 'attach' | 'detach') => void;

export function startHotplugWatcher(_cb: HotplugCallback): void {
  // TODO: step 6
}

export function stopHotplugWatcher(): void {
  // TODO: step 6
}
