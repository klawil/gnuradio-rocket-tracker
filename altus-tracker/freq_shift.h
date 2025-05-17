#ifndef INCLUDED_FREQ_SHIFT_H
#define INCLUDED_FREQ_SHIFT_H

#include <gnuradio/attributes.h>
#include <gnuradio/sync_block.h>
#include <string>

#ifdef gnuradio_Freq_Shift_EXPORTS
#define FREQ_SHIFT_API __GR_ATTR_EXPORT
#else
#define FREQ_SHIFT_API __GR_ATTR_IMPORT
#endif

namespace gr {
  namespace FreqShift {
    class FREQ_SHIFT_API DoShift : virtual public gr::sync_block {
      private:
        std::vector<gr_complex> shift_coeffs;
        uint32_t num_shift_coeffs;
        uint32_t shift_coeff_index;

      public:
        typedef std::shared_ptr<DoShift> sptr;
        static sptr make(
          int channel_freq_offset,
          uint32_t sample_rate
        );

        DoShift(
          int channel_freq_offset,
          uint32_t sample_rate
        );
        ~DoShift();

        int work(
          int noutput_items,
          gr_vector_const_void_star &input_items,
          gr_vector_void_star &output_items
        );
    };
  }
}

#endif
