#include "constants.h"
#include "altus_decoder.h"
#include <gnuradio/io_signature.h>
#include <format>
#include <iostream>
#include <string>

const uint16_t packet_length_in_bits = BYTES_PER_MESSAGE * 8;

static const uint8_t fec_encode_table[NUM_V_STATE * 2] = {
	0, 3, /* 000 to 0000 or 0001 */
	1, 2, /* 001 to 0010 or 0011 */
	3, 0, /* 010 to 0100 or 101 */
	2, 1, /* 011 to 0110 or 0111 */
	3, 0, /* 100 to 1000 or 1001 */
	2, 1, /* 101 to 1010 or 1011 */
	0, 3, /* 110 to 1100 or 1101 */
	1, 2  /* 111 to 1110 or 1111 */
};

namespace gr {
  namespace AltusDecoder {
    using input_type = uint8_t;
    
    Decoder::sptr Decoder::make(
      std::string source_type_input,
      uint32_t channel_freq_input,
      uint16_t channel_num_input
    ) {
      return gnuradio::get_initial_sptr(new Decoder(
        source_type_input,
        channel_freq_input,
        channel_num_input
      ));
    }

    // Private constructor
    Decoder::Decoder(
      std::string source_type_input,
      uint32_t channel_freq_input,
      uint16_t channel_num_input
    ) : gr::block(
      "AltusDecoder CH " + std::to_string(channel_num_input),
      gr::io_signature::make(
        1,
        1,
        sizeof(input_type)
      ),
      gr::io_signature::make(
        0,
        0,
        0
      )
    ) {
      source_type = source_type_input;
      channel_freq = channel_freq_input;
      channel_num = channel_num_input;
      reset();
    }

    // Virtual destructor
    Decoder::~Decoder() {}

    // Reset function
    void Decoder::reset() {
      // // Check for a reset occurring when sync word has been found
      // if (
      //   source_type == "file" &&
      //   found_sync_word &&
      //   buffers_filled_for_packet * 2 < BYTES_PER_MESSAGE
      // ) {
      //   d_logger->warn("Reset in the middle of a packet");
      // }

      // Reset the variables used when looking for the sync word
      last_16_bits = 0;
      found_sync_word = false;

      // Reset the buffer variables
      buffers_filled_for_packet = 0;
      bits_buffer = 0;
      bits_buffer_idx = 0;

      // Reset the viterbi state
      cost_index = 0;
      bits_parsed = 0;
      bits_saved = 0;
      for (uint8_t state = 0; state < NUM_V_STATE; state++) {
        cost[0][state] = state == 0 ? 0 : 1 << 7;
        bits[0][state] = 0;
      }
      message_index = 0;

      // Whitening state
      whiten = 0x1FF;

      // CRC
      received_crc = 0;
      computed_crc = 0xFFFF;
    }

    uint32_t Decoder::deinterleave(uint32_t base) {
      uint32_t return_data = 0;
      for (uint8_t bit = 0; bit < 4 * 4; bit++) {
        uint8_t bit_shift = (bit & 0x3) << 3;
        uint8_t byte_shift = (bit & 0xc) >> 1;

        return_data = (return_data << 2) + ((base >> (byte_shift + bit_shift)) & 0x3);
      }

      return return_data;
    }

    void Decoder::get_viterbi_bytes() {
      // Determine the minimum cost path
      uint8_t min_state = 0;
      uint8_t min_cost = cost[cost_index][0];
      for (uint8_t i = 1; i < NUM_V_STATE; i++) {
          if (cost[cost_index][i] < min_cost) {
              min_state = i;
              min_cost = cost[cost_index][i];
          }
      }

      // Override the state if we're in the trellis terminator
      if (bits_parsed == (BYTES_PER_MESSAGE - 1) * 8) {
          min_state = 0;
      } else if (bits_parsed == packet_length_in_bits) {
          min_state = 0;
      }

      // Get the bits from the path
      uint32_t state_bits = bits[cost_index][min_state];

      // Get the byte or bytes
      if (bits_parsed < packet_length_in_bits) {
          uint8_t byte = (state_bits >> NUM_V_HIST) & 0xff;
          message[message_index] = byte;
          message_index++;
      } else {
          uint8_t b;
          for (b = 0; b <= bits_saved && message_index < BYTES_PER_MESSAGE; b += 8) {
              uint8_t byte = (state_bits >> (NUM_V_HIST - b)) & 0xff;
              message[message_index] = byte;
              message_index++;
          }
      }
    }

    void Decoder::viterbi_decode(uint32_t base) {
      for (uint8_t d = 0; d < BITS_TO_BUFFER; d += 2) {
        uint8_t last_cost_index = cost_index;
        cost_index ^= 1;

        // Get the next 2 bits
        uint8_t s = (base >> (BITS_TO_BUFFER - d - 2)) & 0x3;

        // Loop over the possible states
        for (uint8_t state = 0; state < NUM_V_STATE; state++) {
          // What states can we get to this state from?
          uint8_t state1 = state;
          uint8_t state2 = state + (1 << 3);

          // What bits would be used to transition from each of those states?
          uint8_t transition_bits_1 = fec_encode_table[state1];
          uint8_t transition_bits_2 = fec_encode_table[state2];

          // Get the cost of the transition
          uint8_t cost1_bits = (s ^ transition_bits_1);
          uint8_t cost2_bits = (s ^ transition_bits_2);
          uint8_t cost1 = 0;
          uint8_t cost2 = 0;
          while (cost1_bits || cost2_bits) {
              cost1 += cost1_bits & 1;
              cost2 += cost2_bits & 1;

              cost1_bits >>= 1;
              cost2_bits >>= 1;
          }

          // Get the total cost of each path
          uint8_t t_cost1 = cost[last_cost_index][state1 >> 1] + cost1;
          uint8_t t_cost2 = cost[last_cost_index][state2 >> 1] + cost2;

          // Save the lower cost path
          uint8_t last_state = state2;
          uint8_t last_cost = t_cost2;
          if (t_cost1 < t_cost2) {
              last_state = state1;
              last_cost = t_cost1;
          }

          uint32_t new_bits = bits[last_cost_index][last_state >> 1] << 1;
          uint8_t new_bit = last_state & 1;
          new_bits += new_bit;

          bits[cost_index][state] = new_bits;
          cost[cost_index][state] = last_cost;
        }

        bits_parsed++;
        bits_saved++;
        if (bits_saved >= 8 + NUM_V_HIST) {
          bits_saved -= 8;
          get_viterbi_bytes();
        }
      }
    }

    void Decoder::parse_packet_bytes() {
      // Deinterleave
      uint32_t raw_bytes = deinterleave(bits_buffer);

      // Parse from viterbi (reduces number of bits by 2)
      viterbi_decode(raw_bytes);
    }

    uint8_t Decoder::whiten_byte(uint8_t byte) {
      uint8_t whitened = byte ^ (whiten & 0xFF);

      // Increment the whitening integer
      for (uint8_t i = 0; i < 8; i++) {
        whiten = ((whiten >> 1) + (((whiten & 0x1) ^ ((whiten >> 5) & 0x1)) << 8)) & 0x1FF;
      }

      return whitened;
    }

    void Decoder::add_byte_to_crc(uint8_t byte, uint8_t idx) {
      if (idx < 32) {
        // The first 32 bytes are used to compute the CRC
        for (uint8_t i = 0; i < 8; i++) {
            if (((computed_crc & 0x8000) >> 8) ^ (byte & 0x80)) {
                computed_crc = ((computed_crc << 1) ^ 0x8005);
            } else {
                computed_crc = (computed_crc << 1);
            }
            byte = byte << 1;
        }
      } else if (idx < 34) {
        received_crc = (received_crc << 8) + byte;
      }

      // Indexes 34 and 35 are trellis terminators and ignored
    }

    void Decoder::parse_full_packet() {
      for (uint8_t b = 0; b < BYTES_PER_MESSAGE; b++) {
        // Whiten bytes
        message[b] = whiten_byte(message[b]);
        
        // Add to CRC
        add_byte_to_crc(message[b], b);
      }
      
      // Check CRC match
      if (source_type == "file") {
        _total++;
        if (computed_crc == received_crc) {
          _passed++;
        //   d_logger->warn("CRC matched (msg type: {}, passed: {}, total: {})", message[4], _passed, _total);
        // } else {
        //   d_logger->warn("CRC failed (msg type: {}, passed: {}, total: {})", message[4], _passed, _total);
        }
      }

      // Send out message (log for now)
      // @TODO
    }

    uint32_t Decoder::get_parsed() {
      return _total;
    }

    uint32_t Decoder::get_passed() {
      return _passed;
    }

    // Work function
    int Decoder::general_work(
      int ninput_items,
      gr_vector_int &unknown,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items
    ) {
      auto in = static_cast<const input_type*>(input_items[0]);
      for (int index = 0; index < ninput_items; index++) {
        // Reset whenever the squelch re-opens
        std::vector<tag_t> tags;
        get_tags_in_window(tags, 0, index, index + 1, pmt::mp("squelch_sob"));
        if (tags.size() > 0) {
          reset();
        }

        // Look for the sync word
        if (!found_sync_word) {
          last_16_bits = (last_16_bits << 1) | (in[index]);
          if (last_16_bits == SYNC_WORD) {
            found_sync_word = true;
          }
        } else {
          // Cache some bits to be passed to the next function
          bits_buffer = (bits_buffer << 1) + in[index];
          bits_buffer_idx++;

          // Send the buffer to the next step when it is full
          if (bits_buffer_idx >= BITS_TO_BUFFER) {
            parse_packet_bytes();
            
            // Each filled buffer equates to 2 bytes from the message
            bits_buffer = 0;
            bits_buffer_idx = 0;
            buffers_filled_for_packet++;
          }

          // Once all the bytes for the packet are parsed, reset the state
          if (buffers_filled_for_packet * 2 >= BYTES_PER_MESSAGE) {
            parse_full_packet();
            reset();
          }
        }
      }

      consume_each(ninput_items);

      return 0;
    }
  }
}
