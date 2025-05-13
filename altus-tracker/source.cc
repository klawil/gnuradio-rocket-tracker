#include <string>

#include "source.h"

osmosdr::source::sptr Source::get_src_block() {
  return source_block;
}

Source::Source(
  double center_freq,
  double sample_rate,
  std::string dev
) {
  center = center_freq;
  rate = sample_rate;
  device = dev;

  osmosdr::source::sptr osmo_src = osmosdr::source::make();
  
  // Set the sample rate
  osmo_src->set_sample_rate(rate);
  double actual_rate = osmo_src->get_sample_rate();
  rate = round(actual_rate);

  // Set the center frequency
  osmo_src->set_center_freq(center);

  // Set the gain control to auto
  osmo_src->set_gain_mode(true);

  // Save the source block
  source_block = osmo_src;
}
