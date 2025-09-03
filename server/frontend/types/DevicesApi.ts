import { AltusPacket, FullState } from "./Altus";

export interface ApiDeviceState {
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
      Devices: ApiDeviceState[] | null;
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
      CombinedState: FullState;
      Height: {
        Int32: number;
        Valid: boolean;
      };
      Speed: {
        Int32: number;
        Valid: boolean;
      };
      Accel: {
        Int32: number;
        Valid: boolean;
      };
      Altitude: {
        Int32: number;
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
