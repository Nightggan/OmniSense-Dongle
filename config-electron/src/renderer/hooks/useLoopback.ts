import { useState, useEffect } from 'react';
import { ds5 } from '../ipc/client';
import type { LoopbackStatusPayload } from '../../shared/ipc';

export function useLoopbackStatus(): LoopbackStatusPayload | null {
  const [status, setStatus] = useState<LoopbackStatusPayload | null>(null);

  useEffect(() => {
    const off = ds5.onLoopbackStatus((payload) => setStatus(payload));
    return off;
  }, []);

  return status;
}
