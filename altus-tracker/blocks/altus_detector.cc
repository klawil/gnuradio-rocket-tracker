#include "altus_detector.h"
#include <gnuradio/io_signature.h>
#include <cmath>

const int max_channel_width = 30000;
const uint32_t round_channel_to = 50000;

namespace gr {
  namespace AltusDecoder {
    struct peak_t {
      uint32_t freq;
      float amp;
    };

    Detector::sptr Detector::make(
      peak_detected_t peak_callback,
      uint32_t center_freq,
      double sample_rate,
      uint16_t fft_size,
      int flex_channels
    ) {
      return gnuradio::get_initial_sptr(new Detector(
        peak_callback,
        center_freq,
        sample_rate,
        fft_size,
        flex_channels
      ));
    }

    Detector::Detector(
      peak_detected_t peak_callback,
      uint32_t center_freq,
      double sample_rate,
      uint16_t fft_size_p,
      int flex_channels
    ) : gr::block(
      "AltusDetector",
      gr::io_signature::make(
        1,
        1,
        sizeof(float) * fft_size_p
      ),
      gr::io_signature::make(0, 0, 0)
    ) {
      callback = peak_callback;
      center = center_freq;
      samp_rate = sample_rate;
      fft_size = fft_size_p;
      total_channels = flex_channels;
      if (total_channels > 10) {
        total_channels = 10;
      }
    }

    Detector::~Detector() {}

    uint32_t Detector::bucket_to_freq(int bucket) {
      // Get the bottom of the range
      uint32_t min_bucket = center - (samp_rate / 2);
      uint32_t bucket_freq = bucket * samp_rate / fft_size;

      return min_bucket + bucket_freq;
    }

    uint32_t Detector::round_freq(uint32_t freq) {
      uint32_t result = freq + (round_channel_to / 2) - 1;
      result -= result % round_channel_to;
      return result;
    }

    int Detector::general_work(
      int noutput_items,
      gr_vector_int& ninput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items
    ) {
      size_t itemsize = sizeof(float);
      const char* in = (const char*)input_items[0];
      for (int j = 0; j < noutput_items; j++) {
        std::vector<peak_t> peaks;
        float all_v = 0;
        for (int i = 0; i < fft_size; i++) {
          float v;
          std::memcpy(&v, in, itemsize);
          all_v += v;
          if (v > -50) {
            peak_t peak = { bucket_to_freq(i), v };
            peaks.push_back(peak);
            // std::cout << "Peak at " << i << " (" << peak.freq << ")" << std::endl;
          // } else if (bucket_to_freq(i) >= 436545000 && bucket_to_freq(i) <= 436555000) {
          //   std::cout << "Bucket " << std::fixed << std::setprecision(0) << i << " ";
          //   std::cout << bucket_to_freq(i) << " ";
          //   std::cout << v << std::endl;
          }
          in += itemsize;
        }
        // std::cout << "Amp: " << std::fixed << std::setprecision(0) << (all_v / fft_size) << std::endl;

        if (peaks.size() > 0) {
          std::vector<peak_t> new_peaks;
          uint32_t first_peak_freq = 0;
          uint32_t last_peak_freq = 0;
          peak_t kept_peak;

          for (std::vector<peak_t>::iterator it = peaks.begin(); it != peaks.end(); it++) {
            peak_t peak = *it;

            // Check for discontinuity
            if (
              first_peak_freq == 0 ||
              (
                first_peak_freq != 0 &&
                peak.freq - first_peak_freq > max_channel_width
              )
            ) {
              if (first_peak_freq > 0) {
                new_peaks.push_back(kept_peak);
              }
              last_peak_freq = peak.freq;
              first_peak_freq = peak.freq;
              kept_peak = peak;
            } else {
              last_peak_freq = peak.freq;
              if (peak.amp > kept_peak.amp) {
                kept_peak = peak;
              }
            }
          }
          new_peaks.push_back(kept_peak);

          // Check for any net-new channels
          for (std::vector<peak_t>::iterator it = new_peaks.begin(); it != new_peaks.end(); it++) {
            peak_t peak = *it;
            uint32_t channel = round_freq(peak.freq);
            bool keep = true;
            for (int i = 0; i < total_channels; i++) {
              if (channel == last_n_channels[i]) {
                keep = false;
                break;
              }
            }
            if (keep) {
              last_n_channels[channel_idx] = channel;
              channel_idx++;
              if (channel_idx >= total_channels) {
                channel_idx = 0;
              }
              callback(channel);
            }
          }
        }
      }

      consume_each(noutput_items);

      return 0;
    }
  }
}
