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
  serial: number;
  freq: number;
  type: number;
  rtime: number;
  time: number;
  raw: string;
}

interface AltosTelemetrySensor extends BasePacketData {
  ground_accel?: number;
  accel_plus_g?: number;
  accel_minus_g?: number;
  accel?: number;
  grd_press: number;
  press: number;
  temp: number;
  apogee?: number;
  main?: number;
  height: number;
  speed: number;
  accel2: number;
}

interface AltosTelemetryConfiguration extends BasePacketData {
  device: keyof typeof DeviceTypes;
  flight: number;
  conf_maj: number;
  conf_min: number;
  apo_delay: number;
  main_deploy: number;
  v_batt: number;
  max_log: number;
  callsign: string;
  version: string;
}

interface AltosTelemetryLocation extends BasePacketData {
  nsat: number;
  locked: boolean;
  connected: boolean;
  mode: number; // @TODO
  altitude: number;
  latitude: number;
  longitude: number;
  year: number;
  month: number;
  day: number;
  hour: number;
  minute: number;
  second: number;
  pdop: number;
  hdop: number;
  vdop: number;
  ground_speed: number;
  climb_rate: number;
  course: number;
}

interface AltosTelemetrySatellite extends BasePacketData {
  stats: {
    svid: number;
    c_n_1: number;
  }[];
  channels: number;
}

interface AltosTelemetryCompanion extends BasePacketData {
  board_id: number; // @TODO
  update_period: number;
  data: number[];
  channels: number;
}

interface AltosTelemetryMegaSensor extends BasePacketData {
  accel_across: number;
  accel_along: number;
  accel_through: number;
  gyro_roll: number;
  gyro_pitch: number;
  gyro_yaw: number;
  mag_across: number;
  mag_along: number;
  mag_through: number;
  orient: number;
  accel: number;
  pres: number;
  temp: number;
}

interface AltosTelemetryMegaData extends BasePacketData {
  pyro: number[];
  state: FlightState;
  v_batt: number;
  v_pyro: number;
  ground_pres: number;
  ground_accel: number;
  accel_plus_g: number;
  accel_minus_g: number;
  accel: number;
  speed: number;
  height: number;
}

interface AltosTelemetryMetrumSensor extends BasePacketData {
  state: FlightState;
  accel: number;
  pres: number;
  temp: number;
  acceleration: number;
  speed: number;
  height: number;
  v_batt: number;
  sense_a: number;
  sense_m: number;
}

interface AltosTelemetryMetrumData extends BasePacketData {
  ground_pres: number;
  ground_accel: number;
  accel_plus_g: number;
  accel_minus_g: number;
}

interface AltosTelemetryMini extends BasePacketData {
  state: FlightState;
  v_batt: number;
  sense_a: number;
  sense_m: number;
  pres: number;
  temp: number;
  acceleration: number;
  speed: number;
  height: number;
  ground_pres: number;
}

interface AltosTelemetryMegaNorm extends BasePacketData {
  orient: number;
  accel: number;
  pres: number;
  temp: number;
  accel_along: number;
  accel_across: number;
  accel_through: number;
  gyro_roll: number;
  gyro_pitch: number;
  gyro_yaw: number;
  mag_along: number;
  mag_across: number;
  mag_through: number;
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
