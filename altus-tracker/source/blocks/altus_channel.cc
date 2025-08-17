/**
 * This module takes complex input from a source and processes it into Altus packets
 * which are then sent to a given destination
 */

#include "altus_channel.h"
#include <boost/log/trivial.hpp>

altus_channel_sptr make_altus_channel(
  double channel_freq,
  double center_freq,
  double input_sample_rate
) {
  return gnuradio::get_initial_sptr(new AltusChannel(
    channel_freq,
    center_freq,
    input_sample_rate
  ));
}

AltusChannel::~AltusChannel() {}

void AltusChannel::handle_message(
  uint8_t message[BYTES_PER_MESSAGE],
  uint16_t computed_crc,
  uint16_t received_crc
) {
  auto packet = make_altus_packet(
    message,
    channel_freq
  );
  std::string msg = packet->to_string() + "\n";
  packet_queue_mutex.lock();
  packet_queue.push_back(msg);
  packet_queue_mutex.unlock();
}

void AltusChannel::set_channel(uint32_t c) {
  if (channel_freq == c) {
    std::cout << "Creating channel on " << std::fixed << std::setprecision(3) << (float(c) / 1000000) << std::endl;
  } else {
    std::cout << "Changing from " << std::fixed << std::setprecision(3) << (float(channel_freq) / 1000000) << " to ";
    std::cout << std::fixed << std::setprecision(3) << (float(c) / 1000000) << std::endl;
  }
  channel_freq = c;

  float channel_offset = channel_freq - center_freq;
  int first_stage_decimation = floor(input_sample_rate / (channel_rate * first_stage_channel_width)); // 13.02

  std::vector<gr_complex> base_first_stage_taps = gr::filter::firdes::complex_band_pass_2(
    1,
    input_sample_rate,
    float(channel_rate) / -1,
    float(channel_rate) / 1,
    float(channel_rate) / 4,
    10
  );

  // Generate the taps for filtering the offset frequency
  float phase_inc = (2.0 * M_PI * channel_offset) / input_sample_rate;
  std::vector<gr_complex> first_stage_taps;
  gr_complex I = gr_complex(0.0, 1.0);
  int i = 0;
  for (
    std::vector<gr_complex>::iterator it = base_first_stage_taps.begin();
    it != base_first_stage_taps.end();
    it++
  ) {
    gr_complex f = *it;
    first_stage_taps.push_back(f * exp(i * phase_inc * I));
    i++;
  }
  const float rotator_phase_inc = -1 * first_stage_decimation * phase_inc;

  first_stage_filter->set_taps(first_stage_taps);
  xlat_rotator->set_phase_inc(rotator_phase_inc);

  altus_decode->reset();
}

AltusChannel::AltusChannel(
  double channel,
  double center,
  double s
) : gr::hier_block2(
  "AltusChannel " + std::to_string(int(channel)),
  gr::io_signature::make(
    1,
    1,
    sizeof(gr_complex)
  ),
  gr::io_signature::make(0, 0, 0)
) {
  // Save the internal values
  channel_freq = channel;
  center_freq = center;
  input_sample_rate = s;

  // Calculate the values needed to generate filters
  // float channel_offset = channel_freq - center_freq;
  int first_stage_decimation = floor(input_sample_rate / (channel_rate * first_stage_channel_width)); // 13.02
  float first_stage_sample_rate = input_sample_rate / float(first_stage_decimation);
  int second_stage_decimation = floor(first_stage_sample_rate / channel_rate); // 4.006
  float second_stage_sample_rate = first_stage_sample_rate / second_stage_decimation; // 192,307
  int total_decimation = floor(input_sample_rate / channel_rate); // 100
  
  // Parameters for GFSK demodulation
  float gain_mu = 0.175;
  float gain_omega = 0.25 * gain_mu * gain_mu;
  float loop_bw = -1 * std::log(((gain_mu + gain_omega) / -2) + 1);
  float max_dev = 0.005 * samples_per_symbol;

  // Generate the internal coefficients
  std::vector<gr_complex> base_first_stage_taps = gr::filter::firdes::complex_band_pass_2(
    1,
    input_sample_rate,
    channel_rate / -1,
    channel_rate / 1,
    channel_rate / 4,
    10
  );
  std::vector<float> second_stage_taps = gr::filter::firdes::low_pass_2(
    1.0,
    first_stage_sample_rate,
    fsk_deviation * 1.5,
    fsk_deviation / 2,
    60
  );

  // Make the blocks
  first_stage_filter = gr::filter::fft_filter_ccc::make(
    first_stage_decimation,
    base_first_stage_taps
  );
  xlat_rotator = gr::blocks::rotator_cc::make(0);
  second_stage_filter = gr::filter::fft_filter_ccf::make(
    second_stage_decimation,
    second_stage_taps
  );
  // squelch = gr::analog::pwr_squelch_cc::make(
  //   power_squelch_level,
  //   0.0001,
  //   0,
  //   true
  // );
  fll_band_edge = gr::digital::fll_band_edge_cc::make(
    samples_per_symbol,
    0.2,
    2 * samples_per_symbol + 1,
    (2.0 * M_PI) / samples_per_symbol / 250
  );
  fmdemod = gr::analog::quadrature_demod_cf::make(
    channel_rate / (2 * M_PI * fsk_deviation)
  );
  clock_recovery = gr::digital::symbol_sync_ff::make(
    gr::digital::ted_type::TED_MUELLER_AND_MULLER,
    samples_per_symbol,
    loop_bw,
    1.0,
    1.0,
    max_dev,
    1,
    gr::digital::constellation_bpsk::make(),
    gr::digital::ir_type::IR_MMSE_8TAP,
    128
  );
  slicer = gr::digital::binary_slicer_fb::make();
  altus_decode = gr::AltusDecoder::Decoder::make(
    [this](
      uint8_t msg[BYTES_PER_MESSAGE],
      uint16_t c_crc,
      uint16_t r_crc
    ) { 
      handle_message(msg, c_crc, r_crc);
    }
    // std::bind(&AltusChannel::handle_message, this)
    // handle_message
  );

  // Connect things up
  connect(self(), 0, first_stage_filter, 0);
  connect(first_stage_filter, 0, xlat_rotator, 0);
  connect(xlat_rotator, 0, second_stage_filter, 0);
  
  // Conditionally add in arb
  double arb_rate = channel_rate / second_stage_sample_rate;
  if (arb_rate != 1.0) {
    // d_logger->warn("Using ARB Resampler");
    double arb_size = 32;
    double arb_atten = 10;
    double percent = 0.80;
    double halfband = 0.5 * arb_rate;
    double b_w = percent * halfband;
    double t_b = (percent / 2.0) * halfband;
    std::vector<float> arb_taps = gr::filter::firdes::low_pass_2(
      arb_size,
      arb_size,
      b_w,
      t_b,
      arb_atten,
      gr::fft::window::WIN_BLACKMAN_HARRIS
    );
    gr::filter::pfb_arb_resampler_ccf::sptr arb_resampler = gr::filter::pfb_arb_resampler_ccf::make(
      arb_rate,
      arb_taps
    );
    connect(second_stage_filter, 0, arb_resampler, 0);
    // connect(arb_resampler, 0, squelch, 0);
    connect(arb_resampler, 0, fll_band_edge, 0);
  } else {
    // connect(second_stage_filter, 0, squelch, 0);
    connect(second_stage_filter, 0, fll_band_edge, 0);
  }
  // connect(squelch, 0, fll_band_edge, 0);
  connect(fll_band_edge, 0, fmdemod, 0);
  connect(fmdemod, 0, clock_recovery, 0);
  connect(clock_recovery, 0, slicer, 0);
  connect(slicer, 0, altus_decode, 0);

  set_channel(channel);
}
