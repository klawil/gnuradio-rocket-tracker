#include <gnuradio/logger.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/file_source.h>
#include <gnuradio/blocks/throttle.h>
#include <gnuradio/blocks/rotator_cc.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/analog/pwr_squelch_cc.h>

#include <gnuradio/digital/chunks_to_symbols.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/digital/symbol_sync_ff.h>
#include <gnuradio/digital/timing_error_detector_type.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/digital/constellation.h>

#include <gnuradio/blocks/null_sink.h>

#include <iostream>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp> 
#include <string>
#include <math.h>
#include <csignal>

#include <vector>

#include "constants.h"
#include "altus_decoder.h"
// #include "source.h"

namespace po = boost::program_options;

gr::logger logger("altus-tracker");
gr::top_block_sptr tb;

uint32_t input_sample_rate = BAUD_RATE * 66;
uint32_t input_center_freq = 434800000;

uint32_t target_sample_rate = BAUD_RATE * 3;
uint32_t target_center_freq = 435250000;
uint32_t samples_per_symbol = 3;

uint32_t low_pass_cutoff = 26000;
uint32_t low_pass_transition = 5000;

int32_t power_squelch_level = -45;

// Source* make_source_block(gr::top_block_sptr tb) {
//   return new Source(
//     435050000,
//     SAMPLE_RATE,
//     ""
//   );
// }

gr::basic_block_sptr make_file_source(gr::top_block_sptr tb) {
  gr::blocks::file_source::sptr file = gr::blocks::file_source::make(
    sizeof(gr_complex),
    "/Users/william/output_test_file.data"
  );

  gr::blocks::throttle::sptr throttle = gr::blocks::throttle::make(
    sizeof(gr_complex),
    input_sample_rate
  );
  tb->connect(file, 0, throttle, 0);

  return throttle->to_basic_block();
}

gr::digital::binary_slicer_fb::sptr make_gfsk_demod(gr::top_block_sptr tb, gr::analog::pwr_squelch_cc::sptr source) {
  float sensitivity = 5;

  float gain_mu = 0.175;
  float omega_relative_limit = 0;
  float freq_error = 0;
  float omega = samples_per_symbol * (1 + freq_error);
  float gain_omega = 0.25 * gain_mu * gain_mu;

  float dampening = 1.0;
  float loop_bw = -1 * std::log(((gain_mu + gain_omega) / -2) + 1);
  float max_dev = omega_relative_limit * samples_per_symbol;

  gr::analog::quadrature_demod_cf::sptr fmdemod = gr::analog::quadrature_demod_cf::make(1 / sensitivity);
  gr::digital::symbol_sync_ff::sptr clock_recovery = gr::digital::symbol_sync_ff::make(
    gr::digital::ted_type::TED_MUELLER_AND_MULLER,
    omega,
    loop_bw,
    dampening,
    1.0,
    max_dev,
    1,
    gr::digital::constellation_bpsk::make(),
    gr::digital::ir_type::IR_MMSE_8TAP,
    128,
    std::vector< float >()
  );
  gr::digital::binary_slicer_fb::sptr slicer = gr::digital::binary_slicer_fb::make();
  tb->connect(source, 0, fmdemod, 0);
  tb->connect(fmdemod, 0, clock_recovery, 0);
  tb->connect(clock_recovery, 0, slicer, 0);

  return slicer;
}

bool running = true;

void signal_handler(int signal) {
  if (signal == SIGINT) {
    std::cout << "SIGINT received. Cleaning up and exiting..." << std::endl;
    tb->stop();
    running = false;
  }
}

int main(int argc, char **argv) {
  po::options_description desc("Options");
  desc.add_options()
    ("help,h", "Help screen")
    ("version,v", "Version Information")
    ("center_freq,c", po::value<uint32_t>(), "Input center frequency")
    ("low_pass_cutoff", po::value<uint32_t>(), "Low pass filter cutoff frequency")
    ("low_pass_transition", po::value<uint32_t>(), "Low pass filter transition width")
    ("squelch", po::value<int32_t>(), "Squelch level for power squelch");

  po::variables_map vm;
  po::store(parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("version")) {
    std::cout << "Version 0.0.1\n";
    exit(0);
  }

  if (vm.count("help")) {
    std::cout << "Usage: trunk-recorder [options]\n";
    std::cout << desc;
    exit(0);
  }

  // Parse the input configurations out
  if (vm.count("center_freq")) {
    input_center_freq = vm["center_freq"].as<uint32_t>();
  }
  if (vm.count("low_pass_cutoff")) {
    low_pass_cutoff = vm["low_pass_cutoff"].as<uint32_t>();
  }
  if (vm.count("low_pass_transition")) {
    low_pass_transition = vm["low_pass_transition"].as<uint32_t>();
  }
  if (vm.count("squelch")) {
    power_squelch_level = vm["squelch"].as<int32_t>();
  }

  // Log the settings
  std::cout << "Current settings:\n"
    << " - Center Freq: " << input_center_freq << "\n"
    << " - Low Pass Cutoff: " << low_pass_cutoff << "\n"
    << " - Low Pass Transition: " << low_pass_transition << "\n"
    << " - Squelch: " << power_squelch_level << "\n\n";
  return 0;

  tb = gr::make_top_block("Altus");

  gr::basic_block_sptr file_source = make_file_source(tb);

  // Shift the frequency to the target channel
  gr::blocks::rotator_cc::sptr freq_shift = gr::blocks::rotator_cc::make(
    2 * M_PI * ((double)(input_center_freq) - target_center_freq) / (double)(input_sample_rate)
  );
  tb->connect(file_source, 0, freq_shift, 0);

  // Decimate and low pass filter
  static std::vector<float> low_pass_taps = gr::filter::firdes::low_pass(1.0, input_sample_rate, low_pass_cutoff, low_pass_transition);
  gr::filter::fir_filter_ccf::sptr decimating_low_pass_filter = gr::filter::fir_filter_ccf::make(
    (int)(input_sample_rate / target_sample_rate),
    low_pass_taps
  );
  tb->connect(freq_shift, 0, decimating_low_pass_filter, 0);

  // Squelch
  gr::analog::pwr_squelch_cc::sptr power_squelch = gr::analog::pwr_squelch_cc::make(-45, 0.5);
  tb->connect(decimating_low_pass_filter, 0, power_squelch, 0);

  // GFSK demod
  gr::digital::binary_slicer_fb::sptr gfsk_demod = make_gfsk_demod(tb, power_squelch);

  // Decode
  gr::AltusDecoder::Decoder::sptr altus_decode = gr::AltusDecoder::Decoder::make();
  tb->connect(gfsk_demod, 0, altus_decode, 0);

  logger.warn("Starting the top block");
  tb->start();

  std::signal(SIGINT, &signal_handler);
  logger.warn("Press Ctrl + C to exit");
  tb->wait();
}
