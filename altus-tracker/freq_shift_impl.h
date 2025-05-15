#ifndef INCLUDED_FREQ_SHIFT_IMPL_H
#define INCLUDED_FREQ_SHIFT_IMPL_H

#include <vector>
#include <gnuradio/io_signature.h>

#include "freq_shift.h"

namespace gr {
  namespace FreqShift {
    class DoShift_impl : public DoShift {
      private:
        std::vector<gr_complex> shift_coeffs;
        uint32_t num_shift_coeffs;
        uint32_t shift_coeff_index;

      public:
        DoShift_impl(
          int channel_freq_offset,
          uint32_t sample_rate
        );
        ~DoShift_impl();

        int work(
          int noutput_items,
          gr_vector_const_void_star &input_items,
          gr_vector_void_star &output_items
        );
    };
  }
}

#endif
