#ifndef POWER_LEVEL_H
#define POWER_LEVEL_H

#include <gnuradio/hier_block2.h>

#include <gnuradio/blocks/stream_to_vector.h>
#include <gnuradio/blocks/keep_one_in_n.h>
#include <gnuradio/fft/fft.h>
#include <gnuradio/fft/fft_v.h>
#include <gnuradio/fft/window.h>
#include <gnuradio/blocks/complex_to_mag_squared.h>
#include <gnuradio/filter/single_pole_iir_filter_ff.h>
#include <gnuradio/blocks/nlog10_ff.h>

class AltusPowerLevel;

typedef std::shared_ptr<AltusPowerLevel> altus_power_level_sptr;

altus_power_level_sptr make_altus_power_level(
  double input_sample_rate,
  uint16_t fft_size
);

class AltusPowerLevel : public gr::hier_block2 {
  friend altus_power_level_sptr make_altus_power_level(
    double input_sample_rate,
    uint16_t fft_size
  );

  private:
    gr::blocks::stream_to_vector::sptr stream_to_vector;
    gr::blocks::keep_one_in_n::sptr keep_one_in_n;
    gr::fft::fft_v<gr_complex, true>::sptr fft;
    gr::blocks::complex_to_mag_squared::sptr complex_to_mag;
    gr::filter::single_pole_iir_filter_ff::sptr single_pole_filter;
    gr::blocks::nlog10_ff::sptr nlog10;

  public:
    AltusPowerLevel(
      double input_sample_rate,
      uint16_t fft_size
    );
    ~AltusPowerLevel();
};

#endif
