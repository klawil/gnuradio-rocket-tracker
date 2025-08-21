#include "altus_detector.h"
#include <gnuradio/io_signature.h>
#include <cmath>

const int max_channel_width = 40000;

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
      int flex_channels,
      uint32_t min_channel,
      uint32_t max_channel
    ) {
      return gnuradio::get_initial_sptr(new Detector(
        peak_callback,
        center_freq,
        sample_rate,
        fft_size,
        flex_channels,
        min_channel,
        max_channel
      ));
    }

    Detector::Detector(
      peak_detected_t peak_callback,
      uint32_t center_freq,
      double sample_rate,
      uint16_t fft_size_p,
      int flex_channels,
      uint32_t min_channel_f,
      uint32_t max_channel_f
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
      min_channel = min_channel_f;
      max_channel = max_channel_f;
    }

    Detector::~Detector() {}

    uint32_t Detector::bucket_to_freq(int bucket) {
      // Get the bottom of the range
      uint32_t min_bucket = center - (samp_rate / 2);
      uint32_t bucket_freq = bucket * samp_rate / fft_size;

      return min_bucket + bucket_freq;
    }

    uint32_t Detector::round_freq(uint32_t freq) {
      uint32_t result = freq + (ROUND_CHANNEL_TO / 2) - 1;
      result -= result % ROUND_CHANNEL_TO;
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
        auto start_in = in;
        float all_v = 0;
        for (int i = 0; i < fft_size; i++) {
          float v;
          std::memcpy(&v, in, itemsize);
          all_v += v;
        }
        float threshold = (all_v + fft_size * last_threshold) / (fft_size * 2);
        last_threshold = threshold;

        in = start_in;
        std::vector<peak_t> peaks;
        for (int i = 0; i < fft_size; i++) {
          float v;
          std::memcpy(&v, in, itemsize);
          if (v > threshold + 60) {
            peak_t peak = { bucket_to_freq(i), v };
            peaks.push_back(peak);
          }
          in += itemsize;
        }

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
            if (channel < min_channel || channel > max_channel) {
              continue;
            }
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
