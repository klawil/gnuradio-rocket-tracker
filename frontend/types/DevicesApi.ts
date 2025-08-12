import { DeviceTypes, FlightState, FlightStateNames, FullState } from "./Altus";

export interface DeviceState {
  DeviceName: {
    String: string;
    Valid: boolean;
  };
  DeviceType: {
    Int16: number;
    Valid: boolean;
  };
  DeviceSerial: number;
  MaxCreatedAt: string;
  CombinedState: string;
}

export interface ParsedDeviceState {
  DeviceName: string | null;
  DeviceType: number | null;
  DeviceTypeName: string | null;
  DeviceState: null | string;
  DeviceStateNum: FlightState;
  DeviceSerial: number;
  MaxCreatedAt: Date;
  CombinedState: FullState;
}

export function parseDeviceState(s: DeviceState): ParsedDeviceState {
  const outState: ParsedDeviceState = {
    DeviceName: null,
    DeviceType: null,
    DeviceTypeName: null,
    DeviceState: null,
    DeviceSerial: s.DeviceSerial,
    MaxCreatedAt: new Date(s.MaxCreatedAt),
    DeviceStateNum: FlightState.STATELESS,
    CombinedState: {},
  };

  // Add the nullable values
  if (s.DeviceName.Valid) {
    outState.DeviceName = s.DeviceName.String;
  }
  if (s.DeviceType.Valid) {
    outState.DeviceType = s.DeviceType.Int16;
  }

  // Parse the combined state
  outState.CombinedState = JSON.parse(s.CombinedState);

  // Check for a device type in the state
  if (outState.DeviceType === null && outState.CombinedState[4]?.device) {
    outState.DeviceType = outState.CombinedState[4]?.device;
  }

  // Map to the device type name
  if (
    outState.DeviceType !== null &&
    typeof DeviceTypes[outState.DeviceType as keyof typeof DeviceTypes] !== 'undefined'
  ) {
    outState.DeviceTypeName = DeviceTypes[outState.DeviceType as keyof typeof DeviceTypes];
  }

  // Map the device state
  let deviceState: null | FlightState = null;
  const fullState = outState.CombinedState;
  if (fullState[9]) {
    deviceState = fullState[9].state;
  } else if (fullState[21]) {
    deviceState = fullState[21].state;
  } else if (fullState[10]) {
    deviceState = fullState[10].state;
  } else if (fullState[16]) {
    deviceState = fullState[16].state;
  } else if (fullState[17]) {
    deviceState = fullState[17].state;
  }
  if (deviceState !== null) {
    outState.DeviceState = `${FlightStateNames[deviceState] || 'UNKNOWN'} (${deviceState})`;
    outState.DeviceStateNum = deviceState;
  }

  return outState;
}

/**
 * Query DTR events
 * @summary Query DTR events
 * @tags Events
 */
export interface ListDevicesApi {
  path: '/api/devices/';
  method: 'GET';
  query: {
    all?: 'true' | 'false';
  };
  responses: {
    
    /**
     * @contentType application/json
     */
    200: {
      Success: boolean;
      Msg: string | null;
      Devices: DeviceState[];
      Count: number;
    };

    
    /**
     * @contentType application/json
     */
    500: {
      Success: boolean;
      Msg: string | null;
    };
  };
};
