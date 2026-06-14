export interface BatteryProvider {
  read(): Promise<number | null>;
}
