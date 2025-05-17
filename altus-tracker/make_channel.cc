#include "make_channel.h"

#include <gnuradio/io_signature.h>
#include <vector>

namespace gr {
  namespace MakeChannel {
    using input_type = gr_complex;

    Channels::sptr Channels::make(
      uint32_t sample_rate,
      uint32_t center_freq,
      std::vector<uint32_t> channel_freq
    ) {
      return gnuradio::get_initial_sptr(new Channels(
        sample_rate,
        center_freq,
        channel_freq
      ));
    }

    Channels::Channels(
      uint32_t sample_rate,
      uint32_t center_freq,
      std::vector<uint32_t> channel_freq
    ) : gr::sync_block(
      "Channelizer",
      gr::io_signature::make(
        1,
        1,
        sizeof(input_type)
      ),
      gr::io_signature::make(
        channel_freq.size(),
        channel_freq.size(),
        sizeof(input_type)
      )
    ) {
      for (uint8_t c = 0; c < channel_freq.size(); c++) {
        add_channel(sample_rate, (int)center_freq - channel_freq[c]);
      }
    }

    Channels::~Channels() {}

    void Channels::add_channel(
      uint32_t sample_rate,
      int channel_freq_offset
    ) {
      shift_coeff_index.push_back(0);
      int32_t abs_offset = abs(channel_freq_offset);
      std::vector<gr_complex> channel_coeffs;

      if (abs_offset > 0) {
        // Get the number of frequency shifting coefficients needed
        uint32_t channel_shift_coeffs = sample_rate;
        for (uint32_t i = 1; i < sample_rate; i++) {
          float float_part = (float)abs_offset * i / sample_rate;
          uint32_t int_part = abs_offset * i / sample_rate;

          if ((float)int_part == float_part) {
            channel_shift_coeffs = i;
            // std::cout << "Shift coeffs for " + std::to_string(channel_freq_offset) + ": " + std::to_string(i) + "\n";
            break;
          }
        }
        num_shift_coeffs.push_back(channel_shift_coeffs);

        // Calculate the coefficients
        for (uint32_t i = 0; i < channel_shift_coeffs; i++) {
          float cos_sin_input = 2 * M_PI * abs_offset * i / sample_rate;
          float cos_value = std::cos(cos_sin_input);
          float sin_value = std::sin(cos_sin_input);

          channel_coeffs.push_back(channel_freq_offset < 0
            ? std::complex<float>(sin_value, cos_value)
            : std::complex<float>(cos_value, sin_value));
        }
      } else {
        num_shift_coeffs.push_back(0);
      }

      shift_coeffs.push_back(channel_coeffs);
    }

    int Channels::work(
      int noutput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items
    ) {
      auto in0 = static_cast<const input_type*>(input_items[0]);

      for (int o = 0; o < num_shift_coeffs.size(); o++) {
        auto out = static_cast<input_type*>(output_items[o]);

        for (int i = 0; i < noutput_items; i++) {
          if (num_shift_coeffs[o] > 0) {
            out[i] = in0[i] * shift_coeffs[o][shift_coeff_index[o]];
            shift_coeff_index[o]++;
            if (shift_coeff_index[o] >= shift_coeffs[o].size()) {
              shift_coeff_index[o] = 0;
            }
          } else {
            out[i] = in0[i];
          }
        }
      }

      return noutput_items;
    }
  }
}
