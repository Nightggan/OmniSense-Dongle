export type DS5ErrorCode = 'NOT_FOUND' | 'NOT_CONNECTED' | 'IO' | 'TIMEOUT';

export class DS5DeviceError extends Error {
  constructor(
    public readonly code: DS5ErrorCode,
    message: string,
  ) {
    super(message);
    this.name = 'DS5DeviceError';
  }
}
