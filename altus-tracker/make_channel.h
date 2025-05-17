#ifndef INCLUDED_MAKE_CHANNEL_H
#define INCLUDED_MAKE_CHANNEL_H

#include <gnuradio/attributes.h>
#include <gnuradio/sync_block.h>
#include <string>
#include <vector>

#ifdef gnuradio_Make_Channel_EXPORTS
#define MAKE_CHANNEL_API __GR_ATTR_EXPORT
#else
#define MAKE_CHANNEL_API __GR_ATTR_IMPORT
#endif

namespace gr {
  namespace MakeChannel {
    class MAKE_CHANNEL_API Channels : virtual public gr::sync_block {
      private:
        std::vector<std::vector<gr_complex>> shift_coeffs;
        std::vector<uint32_t> num_shift_coeffs;
        std::vector<uint32_t> shift_coeff_index;

        void add_channel(uint32_t sample_rate, int channel_freq_offset);

      public:
        typedef std::shared_ptr<Channels> sptr;
        static sptr make(
          uint32_t sample_rate,
          uint32_t center_freq,
          std::vector<uint32_t> channel_freq_offset
        );

        Channels(
          uint32_t sample_rate,
          uint32_t center_freq,
          std::vector<uint32_t> channel_freq_offset
        );
        ~Channels();

        int work(
          int noutput_items,
          gr_vector_const_void_star &input_items,
          gr_vector_void_star &output_items
        );
    };
  }
}

#endif
