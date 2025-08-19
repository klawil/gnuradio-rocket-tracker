import { FlightState } from "@/types/Altus";
import { ParsedDeviceState } from "@/types/DevicesApi";

export function getDeviceState(d: ParsedDeviceState) {
  const state = d.CombinedState;
  if (state[9]) {
    return state[9].State;
  } else if (state[21]) {
    return state[21].State;
  } else if (state[10]) {
    return state[10].State;
  } else if (state[16]) {
    return state[16].State;
  } else if (state[17]) {
    return state[17].State;
  }

  return FlightState.STATELESS;
}

/**
 * All screens
 * Call sign - 4
 * Serial number - any
 * Flight number - 4
 * State - 9, 10, 16, 17, 21
 * RSSI? / Other signal quality metric - any
 * Channel (MHz) - any
 * Age (last GPS packet?) - any/5
 */

/**
 * Pad Screen:
 * Battery voltage - 4?, 9, 10, 21, 16, 17
 * Data logging - ??
 * GPS locked (+ sat counts) - 5, 6
 * GPS ready - 5
 * Apogee ignitor (plus indicator light) - 9, 10, 16, 17, 21
 * Main ignitor (plus indicator light) - 9, 10, 16, 17, 21
 * Arrow to rocket
 * Apogee delay - 4
 * Main deploy altitude - 4
 */

/**
 * Flight Screen:
 * Speed - 1, 2, 3, 9, 10, 16, 17, 21
 * Height - 1, 2, 3, 9, 10, 16, 17, 21
 * Altitude
 * Max Speed
 * Max Height
 * Max Altitude
 * Elevation
 * Range
 * Arrow to rocket
 * Apogee Igniter (plus indicator light) - 9, 10, 16, 17, 21
 * Main ignitor (plus indicator light) - 9, 10, 16, 17, 21
 */

/**
 * Recover Screen:
 * Bearing
 * Distance
 * Max height
 * Max speed
 * Max accel?
 */
