#include <format>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>

#include "altus_packet.h"

AltosBasePacket::AltosBasePacket(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
) {
  for (uint8_t i = 0; i < BYTES_PER_MESSAGE; i++) {
    message[i] = new_message[i];
  }
  serial = (message[1] << 8) | message[0];
  rockettime = (message[3] << 8) | message[2];
  type = message[4];

  channel_freq = new_channel_freq;
  std::chrono::milliseconds ms = duration_cast< std::chrono::milliseconds >(
    std::chrono::system_clock::now().time_since_epoch()
  );

  str_value << "{\"serial\":" << std::fixed << std::setprecision(0) << serial << ",";
  str_value << "\"freq\":" << std::fixed << std::setprecision(3) << (channel_freq / 1000000) << ",";
  str_value << "\"type\":" << +type << ",";
  str_value << "\"rtime\":" << std::fixed << std::setprecision(0) << rockettime << ",";
  str_value << "\"time\":" << std::fixed << std::setprecision(0) << ms.count() << ",";
  str_value << "\"raw\":\"" << std::hex;
  for (uint8_t i = 0; i < BYTES_PER_MESSAGE; i++) {
    str_value << std::setw(2) << std::setfill('0') << +message[i];
  }
  str_value << std::dec << "\",";
}
AltosBasePacket::~AltosBasePacket() {}

std::string AltosBasePacket::to_string() {
  return str_value.str();
}

double AltosBasePacket::mega_battery_voltage(int16_t v) {
  return 3.3 * (v / 4095.0) * (5.6 + 10.0) / 10.0;
}

double AltosBasePacket::mega_pyro_voltage(int16_t v) {
  return 3.3 * (v / 4095.0) * (100.0 + 27.0) / 27.0;
}

double AltosBasePacket::mega_pyro_voltage_30v(int16_t v) {
  return 3.3 * (v / 4095.0) * (100.0 + 12.0) / 12.0;
}

double AltosBasePacket::tele_mini_2_voltage(int16_t v) {
  return v / 32767.0 * 3.3 * 127.0 / 27.0;
}

double AltosBasePacket::tele_mini_3_battery_voltage(int16_t v) {
  return 3.3 * (v / 4095.0) * (5.6 + 10.0) / 10.0;
}

double AltosBasePacket::tele_mini_3_pyro_voltage(int16_t v) {
  return 3.3 * (v / 4095.0) * (100.0 + 27.0) / 27.0;
}

AltosTelemetrySensor::AltosTelemetrySensor(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
) : AltosBasePacket(new_message, new_channel_freq) {
  if (type == 0x01) {
    str_value << "\"ground_accel\":" << std::fixed << std::setprecision(0) << int16(24) << ",";
    str_value << "\"accel_plus_g\":" << std::fixed << std::setprecision(0) << int16(28) << ",";
    str_value << "\"accel_minus_g\":" << std::fixed << std::setprecision(0) << int16(30) << ",";
    str_value << "\"accel\":" << std::fixed << std::setprecision(0) << int16(6) << ",";
  }
  double ground_pressure = ((int16(24) / 16.0) / 2047.0 + 0.095) / 0.009 * 1000.0;
  double pressure = ((int16(8) / 16.0) / 2047.0 + 0.095) / 0.009 * 1000.0;
  str_value << "\"grd_press\":" << std::fixed << std::setprecision(0) << ground_pressure << ",";
  str_value << "\"press\":" << std::fixed << std::setprecision(0) << pressure << ",";
  double temp = (int16(10) - 19791.268) / 32728.0 * 1.25 / 0.00247;
  str_value << "\"temp\":" << std::fixed << std::setprecision(0) << temp << ",";
  if (type == 0x01 || type == 0x02) {
    double apogee = int16(14) / 32767 * 15.0;
    double main = int16(16) / 32767 * 15.0;
    str_value << "\"apogee\":" << std::fixed << std::setprecision(0) << apogee << ",";
    str_value << "\"main\":" << std::fixed << std::setprecision(0) << main << ",";
  }

  str_value << "\"height\":" << std::fixed << std::setprecision(0) << int16(22) << ",";
  str_value << "\"speed\":" << std::fixed << std::setprecision(0) << int16(20) << ",";
  str_value << "\"accel\":" << std::fixed << std::setprecision(0) << int16(18) << "}";
}

AltosTelemetryConfiguration::AltosTelemetryConfiguration(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
) : AltosBasePacket(new_message, new_channel_freq) {
  uint8_t device_type = uint8(5);
  uint16_t flight = uint16(6);
  uint8_t config_major = uint8(8);
  uint8_t config_minor = uint8(9);
  uint16_t apogee_delay = uint16(10);
  uint16_t main_deploy = uint16(12);
  uint16_t v_batt = uint16(10);
  uint16_t flight_log_max = uint16(14);
  std::string callsign = string(16, 8);
  std::string version = string(24, 8);

  str_value << "\"device\":" << std::fixed << std::setprecision(0) << +device_type << ",";
  str_value << "\"flight\":" << std::fixed << std::setprecision(0) << flight << ",";
  str_value << "\"conf_maj\":" << std::fixed << std::setprecision(0) << +config_major << ",";
  str_value << "\"conf_min\":" << std::fixed << std::setprecision(0) << +config_minor << ",";
  str_value << "\"apo_delay\":" << std::fixed << std::setprecision(0) << apogee_delay << ",";
  str_value << "\"main_deploy\":" << std::fixed << std::setprecision(0) << main_deploy << ",";
  str_value << "\"v_batt\":" << std::fixed << std::setprecision(0) << v_batt << ",";
  str_value << "\"max_log\":" << std::fixed << std::setprecision(0) << flight_log_max << ",";
  str_value << "\"callsign\":\"" << callsign << "\",";
  str_value << "\"version\":\"" << version << "\"}";
}

AltosTelemetryLocation::AltosTelemetryLocation(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
) : AltosBasePacket(new_message, new_channel_freq) {
  uint8_t flags = uint8(5);
  uint8_t nsat = flags & 0xf;
  uint8_t locked = (flags & (1 << 4)) > 0;
  uint8_t connected = (flags & (1 << 5)) > 0;
  uint8_t mode = uint8(25);
  int32_t altitude = int16(6);
  if (mode) {
    altitude = (int8(31) << 16) | uint16(6);
  }
  uint32_t latitude = uint32(8);
  uint32_t longitude = uint32(12);
  uint8_t year = uint8(16);
  uint8_t month = uint8(17);
  uint8_t day = uint8(18);
  uint8_t hour = uint8(19);
  uint8_t minute = uint8(20);
  uint8_t second = uint8(21);
  uint8_t pdop = uint8(22);
  uint8_t hdop = uint8(23);
  uint8_t vdop = uint8(24);
  uint16_t ground_speed = uint16(26);
  int16_t climb_rate = int16(28);
  uint8_t course = uint8(30);

  str_value << "\"nsat\":" << std::fixed << std::setprecision(0) << +nsat << ",";
  str_value << "\"locked\":" << (locked ? "true" : "false") << ",";
  str_value << "\"connected\":" << (connected ? "true" : "false") << ",";
  str_value << "\"mode\":" << std::fixed << std::setprecision(0) << +mode << ",";
  str_value << "\"altitude\":" << std::fixed << std::setprecision(0) << altitude << ",";
  str_value << "\"latitude\":" << std::fixed << std::setprecision(0) << latitude << ",";
  str_value << "\"longitude\":" << std::fixed << std::setprecision(0) << longitude << ",";
  str_value << "\"year\":" << std::fixed << std::setprecision(0) << +year << ",";
  str_value << "\"month\":" << std::fixed << std::setprecision(0) << +month << ",";
  str_value << "\"day\":" << std::fixed << std::setprecision(0) << +day << ",";
  str_value << "\"hour\":" << std::fixed << std::setprecision(0) << +hour << ",";
  str_value << "\"minute\":" << std::fixed << std::setprecision(0) << +minute << ",";
  str_value << "\"second\":" << std::fixed << std::setprecision(0) << +second << ",";
  str_value << "\"pdop\":" << std::fixed << std::setprecision(0) << +pdop << ",";
  str_value << "\"hdop\":" << std::fixed << std::setprecision(0) << +hdop << ",";
  str_value << "\"vdop\":" << std::fixed << std::setprecision(0) << +vdop << ",";
  str_value << "\"ground_speed\":" << std::fixed << std::setprecision(0) << ground_speed << ",";
  str_value << "\"climb_rate\":" << std::fixed << std::setprecision(0) << climb_rate << ",";
  str_value << "\"course\":" << std::fixed << std::setprecision(0) << +course << "}";
}

AltosTelemetrySatellite::AltosTelemetrySatellite(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
) : AltosBasePacket(new_message, new_channel_freq) {
  uint8_t channels = uint8(5);
  if (channels > 12) {
    channels = 12;
  }
  if (channels > 0) {
    str_value << "\"sats\":[";
    for (uint8_t i = 0; i < channels; i++) {
      if (i > 0) {
        str_value << ",";
      }
      str_value << "{\"svid\":" << std::fixed << std::setprecision(0) << +uint8(6 + i * 2 + 0) << ",";
      str_value << "\"c_n_1\":" << std::fixed << std::setprecision(0) << +uint8(6 + i * 2 + 1) << "}";
    }
    str_value << "],";
  }
  str_value << "\"channels\":" << std::fixed << std::setprecision(0) << +channels << "}";
}

AltosTelemetryCompanion::AltosTelemetryCompanion(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
) : AltosBasePacket(new_message, new_channel_freq) {
  uint8_t board_id = uint8(5);
  uint8_t update_period = uint8(6);
  uint8_t channels = uint8(7);
  if (channels > 0) {
    str_value << "\"board_id\":" << std::fixed << std::setprecision(0) << +board_id << ",";
    str_value << "\"update_period\":" << std::fixed << std::setprecision(0) << +update_period << ",";

    str_value << "\"data\":[";
    for (uint8_t i = 0; i < channels; i++) {
      if (i > 0) {
        str_value << ",";
      }
      str_value << std::fixed << std::setprecision(0) << uint16(8 + i * 2);
    }
    str_value << "],";
  }
  str_value << "\"channels\":" << std::fixed << std::setprecision(0) << +channels << "}";
}

AltosTelemetryMegaSensor::AltosTelemetryMegaSensor(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
) : AltosBasePacket(new_message, new_channel_freq) {
  uint8_t v4_sensor = 0x08 == type;

  int16_t accel_across = v4_sensor ? -1 * int16(16) : int16(14);
  int16_t accel_along = v4_sensor ? int16(14) : int16(16);
  int16_t accel_through = int16(18);

  int16_t gyro_roll = v4_sensor ? int16(20) : int16(22);
  int16_t gyro_pitch = v4_sensor ? int16(22) : int16(20);
  int16_t gyro_yaw = int16(24);

  int16_t mag_across = v4_sensor ? -1 * int16(28) : int16(26);
  int16_t mag_along = v4_sensor ? int16(26) : int16(28);
  int16_t mag_through = int16(30);

  int8_t orient = int8(5);
  int16_t accel = int16(6);
  int16_t pres = int32(8);
  float temp = int16(12) / 100.0;

  str_value << "\"accel_across\":" << std::fixed << std::setprecision(0) << accel_across << ",";
  str_value << "\"accel_along\":" << std::fixed << std::setprecision(0) << accel_along << ",";
  str_value << "\"accel_through\":" << std::fixed << std::setprecision(0) << accel_through << ",";
  str_value << "\"gyro_roll\":" << std::fixed << std::setprecision(0) << accel_across << ",";
  str_value << "\"gyro_pitch\":" << std::fixed << std::setprecision(0) << accel_along << ",";
  str_value << "\"gyro_yaw\":" << std::fixed << std::setprecision(0) << accel_through << ",";
  str_value << "\"mag_across\":" << std::fixed << std::setprecision(0) << accel_across << ",";
  str_value << "\"mag_along\":" << std::fixed << std::setprecision(0) << accel_along << ",";
  str_value << "\"mag_through\":" << std::fixed << std::setprecision(0) << accel_through << ",";
  str_value << "\"orient\":" << std::fixed << std::setprecision(0) << orient << ",";
  str_value << "\"accel\":" << std::fixed << std::setprecision(0) << accel << ",";
  str_value << "\"pres\":" << std::fixed << std::setprecision(0) << pres << ",";
  str_value << "\"temp\":" << std::fixed << std::setprecision(2) << temp << "}";
}

AltosTelemetryMegaData::AltosTelemetryMegaData(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
) : AltosBasePacket(new_message, new_channel_freq) {
  uint8_t state = uint8(5);
  double v_batt = mega_battery_voltage(int16(6));
  double v_pyro = type == 0x09
    ? mega_pyro_voltage(int16(8))
    : mega_pyro_voltage_30v(int16(8));
  
  str_value << "\"pyro\":[";
  for (int i = 0; i < 6; i++) {
    uint8_t v = uint8(10 + i);
    if (i > 0) {
      str_value << ",";
    }
    double val = type == 0x09
      ? mega_pyro_voltage((v << 4) | (v >> 4))
      : mega_pyro_voltage_30v((v << 4) | (v >> 4));
    str_value << std::fixed << std::setprecision(2) << val;
  }
  str_value << "],";
  
  int32_t ground_pres = int32(16);
  int16_t ground_accel = int16(20);
  int16_t accel_plus_g = int16(22);
  int16_t accel_minus_g = int16(24);
  int16_t acceleration = int16(26);
  int16_t speed = int16(28);
  int16_t height = int16(30);
  
  str_value << "\"state\":" << std::fixed << std::setprecision(0) << +state << ",";
  str_value << "\"v_batt\":" << std::fixed << std::setprecision(2) << v_batt << ",";
  str_value << "\"v_pyro\":" << std::fixed << std::setprecision(2) << v_pyro << ",";
  str_value << "\"ground_pres\":" << std::fixed << std::setprecision(0) << ground_pres << ",";
  str_value << "\"ground_accel\":" << std::fixed << std::setprecision(0) << ground_accel << ",";
  str_value << "\"accel_plus_g\":" << std::fixed << std::setprecision(0) << accel_plus_g << ",";
  str_value << "\"accel_minus_g\":" << std::fixed << std::setprecision(0) << accel_minus_g << ",";
  str_value << "\"accel\":" << std::fixed << std::setprecision(0) << acceleration << ",";
  str_value << "\"speed\":" << std::fixed << std::setprecision(0) << speed << ",";
  str_value << "\"height\":" << std::fixed << std::setprecision(0) << height << "}";
}

AltosTelemetryMetrumSensor::AltosTelemetryMetrumSensor(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
) : AltosBasePacket(new_message, new_channel_freq) {
  uint8_t state = uint8(5);
  int16_t accel = int16(6);
  int32_t pres = int32(8);
  int16_t temp = int16(12);
  int16_t acceleration = int16(14);
  int16_t speed = int16(16);
  int16_t height = int16(18);
  double v_batt = mega_battery_voltage(int16(20));
  double sense_a = mega_pyro_voltage(int16(22));
  double sense_m = mega_pyro_voltage(int16(24));

  str_value << "\"state\":" << std::fixed << std::setprecision(0) << +state << ",";
  str_value << "\"accel\":" << std::fixed << std::setprecision(0) << accel << ",";
  str_value << "\"pres\":" << std::fixed << std::setprecision(0) << pres << ",";
  str_value << "\"temp\":" << std::fixed << std::setprecision(2) << (temp / 100.0) << ",";
  str_value << "\"acceleration\":" << std::fixed << std::setprecision(2) << (acceleration / 16.0) << ",";
  str_value << "\"speed\":" << std::fixed << std::setprecision(2) << (speed / 16.0) << ",";
  str_value << "\"height\":" << std::fixed << std::setprecision(0) << height << ",";
  str_value << "\"v_batt\":" << std::fixed << std::setprecision(2) << v_batt << ",";
  str_value << "\"sense_a\":" << std::fixed << std::setprecision(2) << sense_a << ",";
  str_value << "\"sense_m\":" << std::fixed << std::setprecision(2) << sense_m << "}";
}

AltosTelemetryMetrumData::AltosTelemetryMetrumData(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
) : AltosBasePacket(new_message, new_channel_freq) {
  int32_t ground_pres = int32(8);
  int32_t ground_accel = int16(12);
  int32_t accel_plus_g = int16(14);
  int32_t accel_minus_g = int16(16);

  str_value << "\"ground_pres\":" << std::fixed << std::setprecision(0) << ground_pres << ",";
  str_value << "\"ground_accel\":" << std::fixed << std::setprecision(0) << ground_accel << ",";
  str_value << "\"accel_plus_g\":" << std::fixed << std::setprecision(0) << accel_plus_g << ",";
  str_value << "\"accel_minus_g\":" << std::fixed << std::setprecision(0) << accel_minus_g << "}";
}

AltosTelemetryMini::AltosTelemetryMini(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
) : AltosBasePacket(new_message, new_channel_freq) {
  uint8_t state = uint8(5);
  double v_batt = type == 0x10 ? tele_mini_2_voltage(int16(6)) : tele_mini_3_battery_voltage(int16(6));
  double sense_a = type == 0x10 ? tele_mini_2_voltage(int16(8)) : tele_mini_3_pyro_voltage(int16(8));
  double sense_m = type == 0x10 ? tele_mini_2_voltage(int16(10)) : tele_mini_3_pyro_voltage(int16(10));
  int32_t pres = int32(12);
  double temp = int16(16) / 100.0;
  double acceleration = int16(18) / 16.0;
  double speed = int16(20) / 16.0;
  int16_t height = int16(22);
  int32_t ground_pres = int32(24);

  str_value << "\"state\":" << std::fixed << std::setprecision(0) << +state << ",";
  str_value << "\"v_batt\":" << std::fixed << std::setprecision(2) << v_batt << ",";
  str_value << "\"sense_a\":" << std::fixed << std::setprecision(2) << sense_a << ",";
  str_value << "\"sense_m\":" << std::fixed << std::setprecision(2) << sense_m << ",";
  str_value << "\"pres\":" << std::fixed << std::setprecision(0) << pres << ",";
  str_value << "\"temp\":" << std::fixed << std::setprecision(2) << temp << ",";
  str_value << "\"acceleration\":" << std::fixed << std::setprecision(2) << acceleration << ",";
  str_value << "\"speed\":" << std::fixed << std::setprecision(2) << speed << ",";
  str_value << "\"height\":" << std::fixed << std::setprecision(0) << height << ",";
  str_value << "\"ground_pres\":" << std::fixed << std::setprecision(0) << ground_pres << "}";
}

AltosTelemetryMegaNorm::AltosTelemetryMegaNorm(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
) : AltosBasePacket(new_message, new_channel_freq) {
  int8_t orient = int8(5);
  int16_t accel = int16(6);
  int32_t pres = int32(8);
  double temp = int16(12) / 100.0;
  int16_t accel_along = int16(14);
  int16_t accel_across = int16(16);
  int16_t accel_through = int16(18);
  int16_t gyro_roll = int16(20);
  int16_t gyro_pitch = int16(22);
  int16_t gyro_yaw = int16(24);
  int16_t mag_along = int16(26);
  int16_t mag_across = int16(28);
  int16_t mag_through = int16(30);

  str_value << "\"orient\":" << std::fixed << std::setprecision(0) << orient << ",";
  str_value << "\"accel\":" << std::fixed << std::setprecision(0) << accel << ",";
  str_value << "\"pres\":" << std::fixed << std::setprecision(0) << pres << ",";
  str_value << "\"temp\":" << std::fixed << std::setprecision(2) << temp << ",";
  str_value << "\"accel_along\":" << std::fixed << std::setprecision(0) << accel_along << ",";
  str_value << "\"accel_across\":" << std::fixed << std::setprecision(0) << accel_across << ",";
  str_value << "\"accel_through\":" << std::fixed << std::setprecision(0) << accel_through << ",";
  str_value << "\"gyro_roll\":" << std::fixed << std::setprecision(0) << gyro_roll << ",";
  str_value << "\"gyro_pitch\":" << std::fixed << std::setprecision(0) << gyro_pitch << ",";
  str_value << "\"gyro_yaw\":" << std::fixed << std::setprecision(0) << gyro_yaw << ",";
  str_value << "\"mag_along\":" << std::fixed << std::setprecision(0) << mag_along << ",";
  str_value << "\"mag_across\":" << std::fixed << std::setprecision(0) << mag_across << ",";
  str_value << "\"mag_through\":" << std::fixed << std::setprecision(0) << mag_through << "}";
}

AltosBasePacket *make_altus_packet(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
) {
  switch (new_message[4]) {
    case 0x01:
    case 0x02:
    case 0x03: {
      return new AltosTelemetrySensor(
        new_message,
        new_channel_freq
      );
    }
    case 0x04: {
      return new AltosTelemetryConfiguration(
        new_message,
        new_channel_freq
      );
    }
    case 0x05: {
      return new AltosTelemetryLocation(
        new_message,
        new_channel_freq
      );
    }
    case 0x06: {
      return new AltosTelemetrySatellite(
        new_message,
        new_channel_freq
      );
    }
    case 0x07: {
      return new AltosTelemetryCompanion(
        new_message,
        new_channel_freq
      );
    }
    case 0x08:
    case 0x12: {
      return new AltosTelemetryMegaSensor(
        new_message,
        new_channel_freq
      );
    }
    case 0x09:
    case 0x15: {
      return new AltosTelemetryMegaData(
        new_message,
        new_channel_freq
      );
    }
    case 0x0A: {
      return new AltosTelemetryMetrumSensor(
        new_message,
        new_channel_freq
      );
    }
    case 0x0B: {
      return new AltosTelemetryMetrumData(
        new_message,
        new_channel_freq
      );
    }
    case 0x10:
    case 0x11: {
      return new AltosTelemetryMini(
        new_message,
        new_channel_freq
      );
    }
    case 0x13:
    case 0x14: {
      return new AltosTelemetryMegaNorm(
        new_message,
        new_channel_freq
      );
    }
    default: {
      return new AltosBasePacket(
        new_message,
        new_channel_freq
      );
    }
  }
}
