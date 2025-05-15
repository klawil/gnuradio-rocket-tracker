#include "freq_shift_impl.h"

#include <vector>
#include <string>
#include <complex>
#include <cmath>

#include <gnuradio/io_signature.h>

namespace gr {
  namespace FreqShift {
    using input_type = gr_complex;

    DoShift::sptr DoShift::make(
      int channel_freq_offset,
      uint32_t sample_rate
    ) {
      return gnuradio::get_initial_sptr(new DoShift_impl(
        channel_freq_offset,
        sample_rate
      ));
    }

    DoShift_impl::DoShift_impl(
      int channel_freq_offset,
      uint32_t sample_rate
    ) : gr::sync_block(
      "FreqShift " + std::to_string(channel_freq_offset),
      gr::io_signature::make(
        1,
        1,
        sizeof(input_type)
      ),
      gr::io_signature::make(
        1,
        1,
        sizeof(input_type)
      )
    ) {
      int32_t abs_offset = abs(channel_freq_offset);
      if (abs_offset > 0) {
        // Get the number of frequency shifting coefficients needed
        for (uint32_t i = 1; i < sample_rate; i++) {
          float float_part = (float)abs_offset * i / sample_rate;
          uint32_t int_part = abs_offset * i / sample_rate;

          if ((float)int_part == float_part) {
            num_shift_coeffs = i;
            // std::cout << "Shift coeffs for " + std::to_string(channel_freq_offset) + ": " + std::to_string(i) + "\n";
            break;
          }
        }

        // Calculate the coefficients
        for (uint32_t i = 0; i < num_shift_coeffs; i++) {
          float cos_sin_input = 2 * M_PI * abs_offset * i / sample_rate;
          float cos_value = std::cos(cos_sin_input);
          float sin_value = std::sin(cos_sin_input);

          shift_coeffs.push_back(channel_freq_offset < 0
            ? std::complex<float>(sin_value, cos_value)
            : std::complex<float>(cos_value, sin_value));
        }
      }
    }
    
    DoShift_impl::~DoShift_impl() {}

    int DoShift_impl::work(
      int noutput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items
    ) {
      auto in0 = static_cast<const input_type*>(input_items[0]);
      auto out0 = static_cast<input_type*>(output_items[0]);

      for (int i = 0; i < noutput_items; i++) {
        out0[i] = in0[i] * shift_coeffs[shift_coeff_index];
        shift_coeff_index++;
        if (shift_coeff_index >= num_shift_coeffs) {
          shift_coeff_index = 0;
        }
      }

      return noutput_items;
    }
  }
}
