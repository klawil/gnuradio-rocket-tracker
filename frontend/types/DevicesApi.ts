import { AltusPacket, DeviceTypes, FlightState, FlightStateNames, FullState } from "./Altus";

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
  CombinedState: FullState;
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
    CombinedState: s.CombinedState,
  };

  // Add the nullable values
  if (s.DeviceName.Valid) {
    outState.DeviceName = s.DeviceName.String;
  }
  if (s.DeviceType.Valid) {
    outState.DeviceType = s.DeviceType.Int16;
  }

  // Check for a device type in the state
  if (outState.DeviceType === null && outState.CombinedState[4]?.Device) {
    outState.DeviceType = outState.CombinedState[4]?.Device;
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
    deviceState = fullState[9].State;
  } else if (fullState[21]) {
    deviceState = fullState[21].State;
  } else if (fullState[10]) {
    deviceState = fullState[10].State;
  } else if (fullState[16]) {
    deviceState = fullState[16].State;
  } else if (fullState[17]) {
    deviceState = fullState[17].State;
  }
  if (deviceState !== null) {
    outState.DeviceState = `${FlightStateNames[deviceState] || 'UNKNOWN'} (${deviceState})`;
    outState.DeviceStateNum = deviceState;
  }

  return outState;
}

/**
 * List
 * @summary List devices (by default in the past 12 hours)
 * @tags Devices
 * @contentType application/json
 */
export type ListDevicesApi = {
  path: '/api/devices/';
  method: 'GET';
  query: {
    all?: 'true' | 'false';
  };
  responses: {
    200: {
      Success: boolean;
      Msg: string | null;
      Devices: DeviceState[];
      Count: number;
    };
    500: {
      Success: boolean;
      Msg: string | null;
    };
  };
};

/**
 * Patch
 * @summary Update a device name or type
 * @tags Devices
 * @body.contentType application/json
 * @contentType application/json
 */
export type PatchDeviceApi = {
  path: '/api/devices/{id}/';
  params: {
    /**
     * The serial number of the device to update
     */
    id: number;
  };
  method: 'PATCH';
  body: {
    Name?: string;
    Type?: number;
  };
  responses: {
    200: {
      Success: boolean;
      Msg: string;
    };
    500: {
      Success: boolean;
      Msg: string;
    };
  };
};

/**
 * Delete
 * @summary Delete a device
 * @tags Devices
 * @body.contentType application/json
 * @contentType application/json
 */
export type DeleteDeviceApi = {
  path: '/api/devices/{id}/';
  params: {
    /**
     * The serial number of the device to update
     */
    id: number;
  };
  method: 'DELETE';
  responses: {
    200: {
      Success: boolean;
      Msg: string;
    };
    500: {
      Success: boolean;
      Msg: string;
    };
  };
};

/**
 * Get
 * @summary Get a device and the last 200 packets
 * @tags Devices
 * @body.contentType application/json
 * @contentType application/json
 */
export type GetDeviceApi = {
  path: '/api/devices/{id}/';
  params: {
    /**
     * The serial number of the device to update
     */
    id: number;
  };
  method: 'GET';
  responses: {
    200: {
      Success: boolean;
      Msg: string;
      Serial: number;
      CreatedAt: string;
      Name: {
        String: string;
        Valid: boolean;
      };
      DeviceType: {
        Int16: number;
        Valid: boolean;
      };
      Packets: AltusPacket[];
    };
    500: {
      Success: boolean;
      Msg: string;
    };
  };
};
