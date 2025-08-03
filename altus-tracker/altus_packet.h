#ifndef PACKET_H
#define PACKET_H

#include "constants.h"

class AltusPacket;

class AltusPacket {
  friend std::ostream& operator<<(std::ostream& os, const AltusPacket& obj);

  private:
    uint8_t message[BYTES_PER_MESSAGE];
    bool crc_match;
    double channel_freq;

    uint16_t serial;
    uint16_t rockettime;
    uint8_t type;

  public:
    AltusPacket(
      uint8_t new_message[BYTES_PER_MESSAGE],
      bool new_crc_match,
      double new_channel_freq
    );
    ~AltusPacket();

    std::string to_string();
};

#endif
