#ifndef PACKET_H
#define PACKET_H

#include <chrono>

#include "constants.h"

class AltosBasePacket {
  protected:
    int8_t int8(int i) {
      return int(message[i]);
    }

    uint8_t uint8(int i) {
      return int(message[i]);
    }

    int16_t int16(int i) {
      return int(message[i] + (message[i + 1] << 8));
    }

    uint16_t uint16(int i) {
      return message[i] + (message[i + 1] << 8);
    }

    uint32_t uint32(int i) {
      return message[i] +
        (message[i+1] << 8) +
        (message[i+1] << 16) +
        (message[i+1] << 24);
    }

    int32_t int32(int i) {
      return int(uint32(i));
    }

    std::string string(int i, int l) {
      std::stringstream s;
      for (int x = i; x < i + l; x++) {
        if (message[x] != 0) {
          s << message[x];
        }
      }
      return s.str();
    }

    uint8_t message[BYTES_PER_MESSAGE];
    double channel_freq;
    uint8_t type;
    uint16_t rockettime;
    uint16_t serial;

  public:
    AltosBasePacket(
      uint8_t new_message[BYTES_PER_MESSAGE],
      double new_channel_freq
    );
    ~AltosBasePacket();

    double mega_battery_voltage(int16_t v);
    double mega_pyro_voltage(int16_t v);
    double mega_pyro_voltage_30v(int16_t v);
    double tele_mini_2_voltage(int16_t v);
    double tele_mini_3_battery_voltage(int16_t v);
    double tele_mini_3_pyro_voltage(int16_t v);

    std::stringstream str_value;
    std::string to_string();
};

// 0x01, 0x02, 0x03
class AltosTelemetrySensor : public AltosBasePacket {
  public:
    AltosTelemetrySensor(
      uint8_t new_message[BYTES_PER_MESSAGE],
      double new_channel_freq
    );
};

// 0x04
class AltosTelemetryConfiguration : public AltosBasePacket {
  public:
    AltosTelemetryConfiguration(
      uint8_t new_message[BYTES_PER_MESSAGE],
      double new_channel_freq
    );
};

// 0x05
class AltosTelemetryLocation : public AltosBasePacket {
  public:
    AltosTelemetryLocation(
      uint8_t new_message[BYTES_PER_MESSAGE],
      double new_channel_freq
    );
};

// 0x06
class AltosTelemetrySatellite : public AltosBasePacket {
  public:
    AltosTelemetrySatellite(
      uint8_t new_message[BYTES_PER_MESSAGE],
      double new_channel_freq
    );
};

// 0x07
class AltosTelemetryCompanion : public AltosBasePacket {
  public:
    AltosTelemetryCompanion(
      uint8_t new_message[BYTES_PER_MESSAGE],
      double new_channel_freq
    );
};

// 0x08, 0x12
class AltosTelemetryMegaSensor : public AltosBasePacket {
  public:
    AltosTelemetryMegaSensor(
      uint8_t new_message[BYTES_PER_MESSAGE],
      double new_channel_freq
    );
};

// 0x09, 0x15
class AltosTelemetryMegaData : public AltosBasePacket {
  public:
    AltosTelemetryMegaData(
      uint8_t new_message[BYTES_PER_MESSAGE],
      double new_channel_freq
    );
};

// 0x0A
class AltosTelemetryMetrumSensor : public AltosBasePacket {
  public:
    AltosTelemetryMetrumSensor(
      uint8_t new_message[BYTES_PER_MESSAGE],
      double new_channel_freq
    );
};

// 0x0B
class AltosTelemetryMetrumData : public AltosBasePacket {
  public:
    AltosTelemetryMetrumData(
      uint8_t new_message[BYTES_PER_MESSAGE],
      double new_channel_freq
    );
};

// 0x10, 0x11
class AltosTelemetryMini : public AltosBasePacket {
  public:
    AltosTelemetryMini(
      uint8_t new_message[BYTES_PER_MESSAGE],
      double new_channel_freq
    );
};

// 0x13, 0x14
class AltosTelemetryMegaNorm : public AltosBasePacket {
  public:
    AltosTelemetryMegaNorm(
      uint8_t new_message[BYTES_PER_MESSAGE],
      double new_channel_freq
    );
};

AltosBasePacket *make_altus_packet(
  uint8_t new_message[BYTES_PER_MESSAGE],
  double new_channel_freq
);

#endif
