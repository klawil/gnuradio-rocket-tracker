#ifndef INCLUDED_ALTUS_DECORDER_IMPL_H
#define INCLUDED_ALTUS_DECORDER_IMPL_H

#include <string>

#include "altus_decoder.h"
#include "constants.h"

namespace gr {
  namespace AltusDecoder {
    class Decoder_impl : public Decoder {
      private:
        // Channel information
        std::string source_type;
        uint32_t channel_freq;
        uint16_t channel_num;
        
        // Sync word detection
        uint16_t last_16_bits;
        bool found_sync_word;  

        // Buffering of bits for interleaving
        uint32_t bits_buffer;
        uint8_t bits_buffer_idx;
        uint8_t buffers_filled_for_packet;

        // State for the viterbi decoding
        uint8_t cost_index;
        uint8_t cost[2][NUM_V_STATE];
        uint32_t bits[2][NUM_V_STATE];
        uint16_t bits_parsed;
        uint16_t bits_saved;
        uint8_t message[BYTES_PER_MESSAGE];
        uint8_t message_index;

        // Whitening of bytes
        uint32_t whiten;

        // CRC
        uint16_t computed_crc;
        uint16_t received_crc;

        void reset();
        void parse_packet_bytes(); // Deinterleave and FEC decode
        void parse_full_packet(); // Whiten and check CRC
        uint32_t deinterleave(uint32_t base);
        void viterbi_decode(uint32_t base); // Generates the viterbi state
        void get_viterbi_bytes(); // Retrieves bytes of data from the viterbi state
        uint8_t whiten_byte(uint8_t byte);
        void add_byte_to_crc(uint8_t byte, uint8_t idx);

      public:
        Decoder_impl(
          std::string source_type_input,
          uint32_t channel_freq_input,
          uint16_t channel_num_input
        );
        ~Decoder_impl();

        int general_work(
          int noutput_items,
          gr_vector_int& ninput_items,
          gr_vector_const_void_star& input_items,
          gr_vector_void_star& output_items
        );
    };
  }
}

#endif
