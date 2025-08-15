#include <cmath>

#include "altus_power_level.h"

altus_power_level_sptr make_altus_power_level(
  double input_sample_rate,
  uint16_t fft_size
) {
  return gnuradio::get_initial_sptr(new AltusPowerLevel(
    input_sample_rate,
    fft_size
  ));
}

AltusPowerLevel::~AltusPowerLevel() {}


AltusPowerLevel::AltusPowerLevel(
  double input_sample_rate,
  uint16_t fft_size
) : gr::hier_block2(
  "AltusPowerLevel",
  gr::io_signature::make(
    1,
    1,
    sizeof(gr_complex)
  ),
  gr::io_signature::make(
    1,
    1,
    sizeof(float) * fft_size
  )
) {
  int one_in_n = input_sample_rate / fft_size / 30;
  static std::vector<float> window = gr::fft::window::blackman_harris(
    fft_size
  );

  float window_power = 0;
  for (std::vector<float>::iterator it = window.begin(); it != window.end(); it++) {
    window_power += *it;
  }

  stream_to_vector = gr::blocks::stream_to_vector::make(
    sizeof(gr_complex),
    fft_size
  );
  keep_one_in_n = gr::blocks::keep_one_in_n::make(
    sizeof(gr_complex) * fft_size,
    one_in_n
  );
  fft = gr::fft::fft_v<gr_complex, true>::make(
    fft_size,
    window,
    true
  );
  complex_to_mag = gr::blocks::complex_to_mag_squared::make(fft_size);
  single_pole_filter = gr::filter::single_pole_iir_filter_ff::make(
    1.0,
    fft_size
  );
  nlog10 = gr::blocks::nlog10_ff::make(
    10,
    fft_size,
    -20 * std::log10(fft_size)
    -10 * std::log10(float(window_power) / fft_size)
    -20 * std::log10(1.0)
  );

  connect(self(), 0, stream_to_vector, 0);
  connect(stream_to_vector, 0, keep_one_in_n, 0);
  connect(keep_one_in_n, 0, fft, 0);
  connect(fft, 0, complex_to_mag, 0);
  connect(complex_to_mag, 0, single_pole_filter, 0);
  connect(single_pole_filter, 0, nlog10, 0);
  connect(nlog10, 0, self(), 0);
}
