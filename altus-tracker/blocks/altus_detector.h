#ifndef INCLUDED_ALTUS_DETECTOR_H
#define INCLUDED_ALTUS_DETECTOR_H

#include <gnuradio/attributes.h>
#include <gnuradio/block.h>

#include <functional>

#ifdef gnuradio_Altus_Decoder_EXPORTS
#define ALTUS_DECODER_API __GR_ATTR_EXPORT
#else
#define ALTUS_DECODER_API __GR_ATTR_IMPORT
#endif

typedef std::function<void (
  uint32_t
)> peak_detected_t;

namespace gr {
  namespace AltusDecoder {
    class ALTUS_DECODER_API Detector : virtual public gr::block {
      private:
        peak_detected_t callback;

        uint32_t center;
        double samp_rate;
        uint16_t fft_size;

        uint32_t bucket_to_freq(int bucket);
        uint32_t round_freq(uint32_t freq);
        uint32_t last_n_channels[10];
        int channel_idx = 0;
        int total_channels;
        float last_threshold;

      public:
        typedef std::shared_ptr<Detector> sptr;
        static sptr make(
          peak_detected_t peak_callback,
          uint32_t center_freq,
          double sample_rate,
          uint16_t fft_size,
          int flex_channels
        );

        Detector(
          peak_detected_t peak_callback,
          uint32_t center_freq,
          double sample_rate,
          uint16_t fft_size,
          int flex_channels
        );
        ~Detector();

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
