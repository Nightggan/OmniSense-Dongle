import { useState, useEffect, useCallback } from 'react';
import { ds5 } from '../ipc/client';

export interface DeviceState {
  connected: boolean;
  model?: string;
  firmwareVersion?: string;
  battery?: number | null;
  rssi?: number | null;
  error?: string;
  connecting: boolean;
}

export function useDevice(): DeviceState & {
  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
} {
  const [state, setState] = useState<DeviceState>({
    connected: false,
    connecting: false,
  });

  // Subscribe to hotplug and telemetry events from main process
  useEffect(() => {
    const offChanged = ds5.onDeviceChanged((payload) => {
      setState((prev) => ({
        ...prev,
        connected: payload.connected,
        model: payload.connected ? (payload.model ?? prev.model) : undefined,
        ...(!payload.connected && { battery: null, rssi: null, firmwareVersion: undefined }),
      }));
    });

    const offTelemetry = ds5.onTelemetry((payload) => {
      setState((prev) => ({
        ...prev,
        battery: payload.battery !== undefined ? payload.battery : prev.battery,
        rssi:    payload.rssi    !== undefined ? payload.rssi    : prev.rssi,
      }));
    });

    // Sync with actual device state on first render (e.g. hot-reload during dev)
    ds5.getStatus()
      .then((status) => {
        setState((prev) => ({
          ...prev,
          connected: status.connected,
          model:     status.model,
          rssi:      status.rssi ?? null,
        }));
      })
      .catch(() => { /* main not ready yet */ });

    return () => {
      offChanged();
      offTelemetry();
    };
  }, []);

  const connect = useCallback(async () => {
    setState((prev) => ({ ...prev, connecting: true, error: undefined }));
    try {
      const result = await ds5.connect();
      const { percent: battery } = await ds5.getBattery().catch(() => ({ percent: null }));

      setState({
        connected: true,
        model: result.model,
        firmwareVersion: result.firmwareVersion,
        battery,
        rssi: null,
        connecting: false,
      });
    } catch (err) {
      setState((prev) => ({
        ...prev,
        connected: false,
        connecting: false,
        error: err instanceof Error ? err.message : 'Connection failed',
      }));
    }
  }, []);

  const disconnect = useCallback(async () => {
    await ds5.disconnect().catch(() => { /* ignore */ });
    setState({ connected: false, connecting: false });
  }, []);

  return { ...state, connect, disconnect };
}
