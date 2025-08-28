import { AltusPacket, DeviceTypes, FlightState, FlightStateNames, FullState } from "@/types/Altus";

export interface DeviceState {
  Name: string | null;
  Serial: number;
  Channel: number;
  LastSeen: Date;
  Maximums: {
    Speed: number;
    Altitude: number;
    Height: number;
    Accel: number;
  };
  State: {
    Num: FlightState;
    Name: string;
    Height: number;
    Speed: number;
    Accel: number;
    LastUpdate: Date | null;
  };
  Location: {
    Locked: boolean;
    NumSats: number;
    Latitude: number;
    Longitude: number;
    Altitude: number;
    LastUpdate: Date | null;
  };
  Battery: {
    Voltage: number;
    LastUpdate: Date | null;
  };
  Igniters: {
    Apogee: number;
    Main: number;
    LastUpdate: Date | null;
  };
  Config: {
    ApogeeDelay: number;
    MainDeploy: number;
    Flight: number;
    CallSign: string;
    Device: keyof typeof DeviceTypes;
    DeviceName: string;
    LastUpdate: Date | null;
  };
}

export const defaultDeviceState: DeviceState = {
  Name: null,
  Serial: 0,
  Channel: 0,
  LastSeen: new Date(0),
  Maximums: {
    Speed: -1,
    Altitude: -1,
    Height: -1,
    Accel: -1,
  },
  State: {
    Num: FlightState.STATELESS,
    Name: FlightStateNames[FlightState.STATELESS],
    Height: 0,
    Speed: 0,
    Accel: 0,
    LastUpdate: null,
  },
  Location: {
    Locked: false,
    NumSats: 0,
    Latitude: 0,
    Longitude: 0,
    Altitude: 0,
    LastUpdate: null,
  },
  Battery: {
    Voltage: 0,
    LastUpdate: null,
  },
  Igniters: {
    Apogee: 0,
    Main: 0,
    LastUpdate: null,
  },
  Config: {
    ApogeeDelay: 0,
    MainDeploy: 0,
    Flight: 0,
    CallSign: '',
    Device: 0,
    DeviceName: '',
    LastUpdate: null,
  },
};

interface InitAction {
  Type: 0;
  DeviceName: {
    String: string;
    Valid: boolean;
  };
  Serial: number;
  CombinedState?: FullState;
  StartPackets?: AltusPacket[];
}

function addPacketToState(state: DeviceState, action: AltusPacket) {
  const newState = {
    ...state,
  };

  // Update serial and channel
  const LastUpdate = new Date(action.Time);
  newState.Channel = action.Freq;
  newState.Serial = action.Serial;

  // Update the last seen
  if (
    newState.LastSeen === null ||
    LastUpdate > newState.LastSeen
  ) {
    newState.LastSeen = LastUpdate;
  }

  // Update the height, speed, accel, and flight state
  if (
    'State' in action &&
    (
      newState.State.LastUpdate === null ||
      LastUpdate > newState.State.LastUpdate
    )
  ) {
    newState.State = {
      Num: action.State,
      Name: FlightStateNames[action.State],
      Height: action.Height,
      Speed: action.Speed,
      Accel: action.Accel,
      LastUpdate,
    };
  } else if (
    'Height' in action &&
    (
      newState.State.LastUpdate === null ||
      LastUpdate > newState.State.LastUpdate
    )
  ) {
    newState.State = {
      Num: FlightState.STATELESS,
      Name: FlightStateNames[FlightState.STATELESS],
      Height: action.Height,
      Speed: action.Speed,
      Accel: action.Accel,
      LastUpdate,
    };
  }

  // Update the battery voltage
  if (
    'BattV' in action &&
    typeof action.BattV !== 'undefined' &&
    (
      newState.Battery.LastUpdate === null ||
      LastUpdate > newState.Battery.LastUpdate
    )
  ) {
    newState.Battery = {
      Voltage: action.BattV,
      LastUpdate,
    };
  }

  // Update the igniters
  if (
    'MainVolts' in action &&
    typeof action.MainVolts !== 'undefined' &&
    (
      newState.Igniters.LastUpdate === null ||
      LastUpdate > newState.Igniters.LastUpdate
    )
  ) {
    newState.Igniters = {
      Main: action.MainVolts,
      Apogee: action.ApogeeVolts || 0,
      LastUpdate,
    };
  } else if (
    'Pyro' in action &&
    (
      newState.Igniters.LastUpdate === null ||
      LastUpdate > newState.Igniters.LastUpdate
    )
  ) {
    newState.Igniters = {
      Main: action.Pyro[0],
      Apogee: action.Pyro[1],
      LastUpdate,
    };
  }

  // Update the GPS location
  if (
    action.Type === 5 &&
    (
      newState.Location.LastUpdate === null ||
      LastUpdate > newState.Location.LastUpdate
    )
  ) {
    newState.Location = {
      Locked: action.Locked,
      NumSats: action.NSat,
      Latitude: action.Latitude,
      Longitude: action.Longitude,
      Altitude: action.Altitude,
      LastUpdate,
    };
  }

  // Update the device config
  if (
    action.Type === 4 &&
    (
      newState.Config.LastUpdate === null ||
      LastUpdate > newState.Config.LastUpdate
    )
  ) {
    newState.Config = {
      Device: action.Device,
      DeviceName: DeviceTypes[action.Device] || 'UNKNOWN',
      Flight: action.Flight,
      CallSign: action.Callsign,
      ApogeeDelay: action.ApoDelay || 0,
      MainDeploy: action.MainAlt || 0,
      LastUpdate,
    }
  }

  // Check for new maximums
  if (newState.State.Height > newState.Maximums.Height) {
    newState.Maximums.Height = newState.State.Height;
  }
  if (newState.State.Speed > newState.Maximums.Speed) {
    newState.Maximums.Speed = newState.State.Speed;
  }
  if (newState.State.Accel > newState.Maximums.Accel) {
    newState.Maximums.Accel = newState.State.Accel;
  }
  if (newState.Location.Altitude > newState.Maximums.Altitude) {
    newState.Maximums.Altitude = newState.Location.Altitude;
  }

  return newState;
}

export function deviceReducer(state: DeviceState, action: AltusPacket | InitAction): DeviceState {
  let newState = {
    ...state,
  };

  switch (action.Type) {
    case 0:
      if (action.DeviceName.Valid) {
        newState.Name = action.DeviceName.String;
      }

      // Loop over the packets
      if (typeof action.CombinedState !== 'undefined') {
        const packetTypes = Object.keys(action.CombinedState) as (keyof typeof action.CombinedState)[];
        for (let i = 0; i < packetTypes.length; i++) {
          const packet = action.CombinedState[packetTypes[i]];
          if (typeof packet === 'undefined') {
            continue;
          }

          newState = addPacketToState(newState, packet);
        }
      } else if (typeof action.StartPackets !== 'undefined') {
        for (let i = action.StartPackets.length - 1; i >= 0; i--) {
          newState = addPacketToState(newState, action.StartPackets[i]);
        }
      }

      return newState;
    default:
      return addPacketToState(newState, action);
  }
}

export function deviceListReducer(state: DeviceState[], action: AltusPacket | InitAction) {
  // Determine if the given serial exists
  const serialExists = typeof state.find(d => d.Serial === action.Serial) !== 'undefined';
  if (!serialExists) {
    return [
      ...state,
      deviceReducer(defaultDeviceState, action),
    ];
  }

  return state.map(d => {
    if (d.Serial === action.Serial) {
      return deviceReducer(d, action);
    }
    return d;
  });
}
