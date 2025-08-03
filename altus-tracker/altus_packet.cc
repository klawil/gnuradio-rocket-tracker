#include <format>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>

#include "altus_packet.h"

AltusPacket::AltusPacket(
  uint8_t new_message[BYTES_PER_MESSAGE],
  bool new_crc_match,
  double new_channel_freq
) {
  for (uint8_t i = 0; i < BYTES_PER_MESSAGE; i++) {
    message[i] = new_message[i];
  }
  crc_match = new_crc_match;
  channel_freq = new_channel_freq;
}

AltusPacket::~AltusPacket() {}

std::string AltusPacket::to_string() {
  std::stringstream s;
  float channel_mhz = channel_freq / 1000000;
  s << "Message Channel " << std::fixed << std::setprecision(3) << channel_mhz;
  s << " MHz, Type 0x";
  s << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << +message[4] << ", ";
  s << (crc_match ? "PASS" : "FAIL");
  return s.str();
}
