export enum FlightState {
  STARTUP = 0,
  IDLE,
  PAD,
  BOOST,
  FAST,
  COAST,
  DROGUE,
  MAIN,
  LANDED,
  INVALID,
  STATELESS,
}
export const FlightStateNames = {
  [FlightState.STARTUP]: 'Startup',
  [FlightState.IDLE]: 'Idle',
  [FlightState.PAD]: 'Pad',
  [FlightState.BOOST]: 'Boost',
  [FlightState.FAST]: 'Fast',
  [FlightState.COAST]: 'Coast',
  [FlightState.DROGUE]: 'Drogue',
  [FlightState.MAIN]: 'Main',
  [FlightState.LANDED]: 'Landed',
  [FlightState.INVALID]: 'INVALID',
  [FlightState.STATELESS]: 'STATELESS',
} as const;

export const DeviceTypes = {
  [0x0000]: 'UNKNOWN',
  [0x000a]: 'AltusMetrum',
  [0x000b]: 'TeleMetrum',
  [0x000c]: 'TeleDongle',
  [0x000d]: 'EasyTimer',
  [0x000e]: 'TeleBT',
  [0x000f]: 'TeleLaunch',
  [0x0010]: 'TeleLCO',
  [0x0011]: 'TeleScience',
  [0x0012]: 'TelePyro',
  [0x0023]: 'TeleMega',
  [0x0024]: 'MegaDongle',
  [0x0025]: 'TeleGPS',
  [0x0026]: 'EasyMini',
  [0x0027]: 'TeleMini',
  [0x0028]: 'EasyMega',
  [0x0029]: 'USBTRNG',
  [0x002a]: 'USBRELAY',
  [0x002b]: 'MPUSB',
  [0x002c]: 'EasyMotor',
} as const;

interface BasePacketData {
  Serial: number;
  Freq: number;
  RTime: number;
  Time: number;
  Raw: string;
}

interface AltosTelemetrySensor extends BasePacketData {
  Type: 1 | 2 | 3; // 0x01, 0x02, 0x03
  GroundAccel?: number;
  AccelPlusG?: number;
  AccelMinusG?: number;
  Accelerometer?: number;
  GrdPress: number;
  Press: number;
  Temp: number;
  ApogeeVolts?: number;
  MainVolts?: number;
  Height: number;
  Speed: number;
  Accel: number;
}

interface AltosTelemetryConfiguration extends BasePacketData {
  Type: 4; // 0x04
  Device: keyof typeof DeviceTypes;
  Flight: number;
  ConfMaj: number;
  ConfMin: number;
  ApoDelay?: number;
  MainAlt?: number;
  BattV?: number;
  MaxLog: number;
  Callsign: string;
  Version: string;
}

interface AltosTelemetryLocation extends BasePacketData {
  Type: 5; // 0x05
  NSat: number;
  Locked: boolean;
  Connected: boolean;
  Mode: number; // @TODO
  Altitude: number;
  Latitude: number;
  Longitude: number;
  Year: number;
  Month: number;
  Day: number;
  Hour: number;
  Minute: number;
  Second: number;
  PDop: number;
  HDop: number;
  VDop: number;
  GroundSpeed: number;
  ClimbRate: number;
  Course: number;
}

interface AltosTelemetrySatellite extends BasePacketData {
  Type: 6; // 0x06
  Sats: {
    SVID: number;
    C_N_1: number;
  }[];
  Channels: number;
}

interface AltosTelemetryCompanion extends BasePacketData {
  Type: 7; // 0x07
  BoardId: number;
  UpdatePeriod: number;
  Data: number[];
  Channels: number;
}

interface AltosTelemetryMegaSensor extends BasePacketData {
  Type: 8 | 18; // 0x08, 0x12
  AccelAcross: number;
  AccelAlong: number;
  AccelThrough: number;
  GyroRoll: number;
  GyroPitch: number;
  GyroYaw: number;
  MagAcross: number;
  MagAlong: number;
  MagThrough: number;
  Orient: number;
  Accel: number;
  Pres: number;
  Temp: number;
}

interface AltosTelemetryMegaData extends BasePacketData {
  Type: 9 | 21; // 0x09, 0x15
  Pyro: number[];
  State: FlightState;
  BattV: number;
  PyroV: number;
  GroundPres: number;
  GroundAccel: number;
  AccelPlusG: number;
  AccelMinusG: number;
  Accel: number;
  Speed: number;
  Height: number;
}

interface AltosTelemetryMetrumSensor extends BasePacketData {
  Type: 10; // 0x0A
  State: FlightState;
  Accelerometer: number;
  Pres: number;
  Temp: number;
  Accel: number;
  Speed: number;
  Height: number;
  BattV: number;
  ApogeeVolts: number;
  MainVolts: number;
}

interface AltosTelemetryMetrumData extends BasePacketData {
  Type: 11; // 0x0B
  GroundPres: number;
  GroundAccel: number;
  AccelPlusG: number;
  AccelMinusG: number;
}

interface AltosTelemetryMini extends BasePacketData {
  Type: 16 | 17; // 0x10, 0x11
  State: FlightState;
  BattV: number;
  ApogeeVolts: number;
  MainVolts: number;
  Pres: number;
  Temp: number;
  Accel: number;
  Speed: number;
  Height: number;
  GroundPres: number;
}

interface AltosTelemetryMegaNorm extends BasePacketData {
  Type: 19 | 20; // 0x13, 0x14
  Orient: number;
  Accel: number;
  Pres: number;
  Temp: number;
  AccelAlong: number;
  AccelAcross: number;
  AccelThrough: number;
  GyroRoll: number;
  GyroPitch: number;
  GyroYaw: number;
  MagAlong: number;
  MagAcross: number;
  MagThrough: number;
}

export interface FullState {
  '1'?: AltosTelemetrySensor;
  '2'?: AltosTelemetrySensor;
  '3'?: AltosTelemetrySensor;
  '4'?: AltosTelemetryConfiguration;
  '5'?: AltosTelemetryLocation;
  '6'?: AltosTelemetrySatellite;
  '7'?: AltosTelemetryCompanion;
  '8'?: AltosTelemetryMegaSensor;
  '9'?: AltosTelemetryMegaData;
  '10'?: AltosTelemetryMetrumSensor;
  '11'?: AltosTelemetryMetrumData;
  '16'?: AltosTelemetryMini;
  '17'?: AltosTelemetryMini;
  '18'?: AltosTelemetryMegaSensor;
  '19'?: AltosTelemetryMegaNorm;
  '20'?: AltosTelemetryMegaNorm;
  '21'?: AltosTelemetryMegaData;
}

export type AltusPacket = AltosTelemetrySensor | AltosTelemetryConfiguration | AltosTelemetryLocation
  | AltosTelemetrySatellite | AltosTelemetryCompanion | AltosTelemetryMegaSensor | AltosTelemetryMegaData
  | AltosTelemetryMetrumSensor | AltosTelemetryMetrumData | AltosTelemetryMini | AltosTelemetryMegaNorm;
