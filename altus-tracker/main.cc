#include <gnuradio/logger.h>
#include <gnuradio/top_block.h>
#include <gnuradio/block.h>
#include <gnuradio/blocks/file_source.h>
#include <gnuradio/blocks/throttle.h>
#include <gnuradio/blocks/rotator_cc.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/blocks/head.h>

#include <gnuradio/digital/chunks_to_symbols.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/digital/symbol_sync_ff.h>
#include <gnuradio/digital/timing_error_detector_type.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/digital/constellation.h>
#include <gnuradio/filter/freq_xlating_fir_filter.h>

#include <gnuradio/blocks/null_sink.h>

#include <iomanip>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp> 
#include <string>
#include <math.h>
#include <csignal>

#include <ranges>

#include <vector>

#include "constants.h"
#include "altus_decoder.h"
#include "freq_shift.h"
#include "make_channel.h"

namespace po = boost::program_options;

gr::logger logger("altus-tracker");
gr::top_block_sptr tb;
bool running = true;

// These stay constant
uint32_t input_sample_rate = BAUD_RATE * 66;
uint32_t target_sample_rate = BAUD_RATE * 3;
uint32_t samples_per_symbol = 3;

// These can be changed by user input
uint32_t input_center_freq = 434800000;
uint32_t low_pass_cutoff = 26000;
uint32_t low_pass_transition = 5000;
int32_t power_squelch_level = -25;
uint8_t channel_count = 21;

// @TODO - remove
uint32_t target_center_freq = 435250000;
std::string shift_type = "internal";
std::vector<gr::block_sptr> blocks;
std::vector<gr::block_sptr> shift_blocks;
std::vector<gr::block_sptr> squelch_blocks;
std::vector<gr::AltusDecoder::Decoder::sptr> parser_blocks;

gr::block_sptr make_file_source(gr::top_block_sptr tb) {
  gr::blocks::file_source::sptr file = gr::blocks::file_source::make(
    sizeof(gr_complex),
    "/Users/william/output_test_file.data"
  );

  gr::blocks::head::sptr head = gr::blocks::head::make(
    sizeof(gr_complex),
    input_sample_rate * 10
  );
  tb->connect(file, 0, head, 0);

  gr::blocks::throttle::sptr throttle = gr::blocks::throttle::make(
    sizeof(gr_complex),
    input_sample_rate
  );
  tb->connect(head, 0, throttle, 0);

  return throttle;
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

  blocks.push_back(fmdemod);
  blocks.push_back(clock_recovery);
  blocks.push_back(slicer);

  return slicer;
}

gr::block_sptr make_channel(uint16_t channel_num, uint32_t channel_freq, std::string source_type, gr::top_block_sptr tb) {
  gr::block_sptr input;

  // Shift the frequency to the target channel
  static std::vector<float> low_pass_taps = gr::filter::firdes::low_pass(1.0, input_sample_rate, low_pass_cutoff, low_pass_transition);
  if (channel_freq != input_center_freq && shift_type != "channels") {
    if (shift_type == "internal") {
      gr::filter::freq_xlating_fir_filter_ccf::sptr freq_shift = gr::filter::freq_xlating_fir_filter_ccf::make(
        (int)(input_sample_rate / target_sample_rate),
        low_pass_taps,
        channel_freq - input_center_freq,
        input_sample_rate
      );

      shift_blocks.push_back(freq_shift);
      blocks.push_back(freq_shift);
      input = freq_shift;
    } else if (shift_type == "shift") {
      gr::FreqShift::DoShift::sptr freq_shift = gr::FreqShift::DoShift::make(
        (int)input_center_freq - channel_freq,
        input_sample_rate
      );

      shift_blocks.push_back(freq_shift);
      blocks.push_back(freq_shift);
      input = freq_shift;
    }
  }

  // Decimate and low pass filter
  gr::filter::fir_filter_ccf::sptr decimating_low_pass_filter;
  if (channel_freq == input_center_freq || shift_type == "shift" || shift_type == "channels") {
    decimating_low_pass_filter = gr::filter::fir_filter_ccf::make(
      (int)(input_sample_rate / target_sample_rate),
      low_pass_taps
    );
    shift_blocks.push_back(decimating_low_pass_filter);
    blocks.push_back(decimating_low_pass_filter);
    if (channel_freq == input_center_freq || shift_type == "channels") {
      input = decimating_low_pass_filter;
    } else {
      tb->connect(input, 0, decimating_low_pass_filter, 0);
    }
  }

  // Squelch
  gr::analog::pwr_squelch_cc::sptr power_squelch = gr::analog::pwr_squelch_cc::make(power_squelch_level, 0.5, 0, true);
  blocks.push_back(power_squelch);
  squelch_blocks.push_back(power_squelch);
  if (shift_type != "internal") {
    tb->connect(decimating_low_pass_filter, 0, power_squelch, 0);
  } else {
    tb->connect(input, 0, power_squelch, 0);
  }

  // GFSK demod
  gr::digital::binary_slicer_fb::sptr gfsk_demod = make_gfsk_demod(tb, power_squelch);

  // Decode
  gr::AltusDecoder::Decoder::sptr altus_decode = gr::AltusDecoder::Decoder::make(
    source_type,
    channel_freq,
    channel_num
  );
  blocks.push_back(altus_decode);
  parser_blocks.push_back(altus_decode);
  tb->connect(gfsk_demod, 0, altus_decode, 0);

  return input;
}

void signal_handler(int signal) {
  if (signal == SIGINT) {
    std::cout << "SIGINT received. Cleaning up and exiting..." << std::endl;
    tb->stop();
    running = false;
  }
}

std::string replaceAll(
    std::string str,
    const std::string& from,
    const std::string& to)
{
    auto&& pos = str.find(from, size_t{});
    while (pos != std::string::npos)
    {
        str.replace(pos, from.length(), to);
        // easy to forget to add to.length()
        pos = str.find(from, pos + to.length());
    }
    return str;
}

int main(int argc, char **argv) {
  po::options_description desc("Options");
  desc.add_options()
    ("help,h", "Help screen")
    ("version,v", "Version Information")
    ("center_freq,c", po::value<uint32_t>(), "Input center frequency")
    ("low_pass_cutoff", po::value<uint32_t>(), "Low pass filter cutoff frequency")
    ("low_pass_transition", po::value<uint32_t>(), "Low pass filter transition width")
    ("squelch", po::value<int32_t>(), "Squelch level for power squelch")
    ("file,f", po::value<std::string>(), "File to use as a source (complex data)")
    ("channels", "Number of channels around the center frequency")
    ("custom", "Custom Block");

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

  if (vm.count("custom")) {
    shift_type = "shift";
  }
  if (vm.count("channels")) {
    shift_type = "channels";
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

  // // Log the settings
  // std::cout << "Current settings:\n"
  //   << " - Center Freq: " << input_center_freq << "\n"
  //   << " - Low Pass Cutoff: " << low_pass_cutoff << "\n"
  //   << " - Low Pass Transition: " << low_pass_transition << "\n"
  //   << " - Squelch: " << power_squelch_level << "\n\n";

  tb = gr::make_top_block("Altus");

  gr::block_sptr source;
  std::string source_type;
  if (vm.count("file")) {
    source = make_file_source(tb);
    source_type = "file";
  }

  // // Determine the range we want to cover
  // uint32_t channel_freq;
  // if (channel_count % 2 == 0) {
  //   channel_freq = input_center_freq - (channel_count / 2) * ALTUS_CHANNEL_WIDTH;
  // } else {
  //   channel_freq = input_center_freq - ((channel_count - 1) / 2) * ALTUS_CHANNEL_WIDTH - ALTUS_CHANNEL_WIDTH / 2;
  // }

  // // Make channels for each channel we want to monitor
  // for (uint8_t channel_num = 0; channel_num < channel_count; channel_num++) {
  //   gr::basic_block_sptr channel_input = make_channel(channel_num, channel_freq, source_type, tb);
  //   tb->connect(source, 0, channel_input, 0);
  //   std::cout << "Channel " + std::to_string(channel_num) + ": " + std::to_string(channel_freq) + "\n";

  //   channel_freq += ALTUS_CHANNEL_WIDTH;
  // }

  std::vector<uint32_t> channels;
  channels.push_back(434550000);
  channels.push_back(434650000);
  channels.push_back(434750000);
  channels.push_back(434850000);
  channels.push_back(434950000);
  channels.push_back(435050000);
  channels.push_back(435150000);
  channels.push_back(435250000);
  channels.push_back(435350000);
  channels.push_back(435450000);
  // channels.push_back(input_center_freq + 450000);
  // channels.push_back(input_center_freq + 350000);
  // channels.push_back(input_center_freq + 250000);
  // channels.push_back(input_center_freq);
  // channels.push_back(input_center_freq + 150000);
  // channels.push_back(input_center_freq + 50000);

  if (shift_type == "channels") {
    gr::MakeChannel::Channels::sptr channel_block = gr::MakeChannel::Channels::make(
      input_sample_rate,
      input_center_freq,
      channels
    );
    shift_blocks.push_back(channel_block);
    blocks.push_back(channel_block);
    tb->connect(source, 0, channel_block, 0);
    for (uint8_t c = 0; c < channels.size(); c++) {
      gr::block_sptr channel_input = make_channel(c + 1, input_center_freq + channels[c], source_type, tb);
      tb->connect(channel_block, c, channel_input, 0);
    }
  } else {
    for (uint8_t c = 0; c < channels.size(); c++) {
      gr::block_sptr channel_input = make_channel(c, channels[c], source_type, tb);
      tb->connect(source, 0, channel_input, 0);
    }
  }

  // logger.warn("Starting the top block");
  tb->start();

  std::signal(SIGINT, &signal_handler);
  // logger.warn("Press Ctrl + C to exit");
  tb->wait();

  // std::cout << "\nTotal Work Time\n";
  // for (gr::block_sptr block : blocks) {
  //   std::cout << "Block " << block->name() << ": " << std::fixed << std::setprecision(0) << block->pc_work_time_total() << "\n";
  // }

  // std::cout << "\nAvg Work Time\n";
  // for (gr::block_sptr block : blocks) {
  //   std::cout << "Block " << block->name() << ": " << std::fixed << std::setprecision(2)<< block->pc_work_time_avg() << "\n";
  // }

  std::cout << "\nShift Type: " << shift_type << "\n";

  // std::cout << "\nCustom Avg\n";
  float total_work_time = 0;
  float all_work_time = 0;
  float total_squelch_samples = 0;
  for (gr::block_sptr block : shift_blocks) {
    total_work_time += block->pc_work_time_total();
    // std::cout << replaceAll(block->name(), ",", "") << "," << std::fixed << std::setprecision(0) << block->pc_work_time_total() << ",";
    // std::cout << std::fixed << std::setprecision(0) << block->nitems_read(0) << "\n";
    // std::cout << std::fixed << std::setprecision(2) << block->nitems_read(0) << "\n";
  }
  for (gr::block_sptr block : blocks) {
    all_work_time += block->pc_work_time_total();
    // std::cout << replaceAll(block->name(), ",", "") << "," << std::fixed << std::setprecision(0) << block->pc_work_time_total() << ",";
    // std::cout << std::fixed << std::setprecision(0) << block->nitems_read(0) << "\n";
    // std::cout << std::fixed << std::setprecision(2) << block->nitems_read(0) << "\n";
  }
  for (gr::block_sptr block : squelch_blocks) {
    total_squelch_samples += block->nitems_written(0);
    // std::cout << "Block input samples " << block->name() << ": " << std::fixed << block->nitems_read(0) << " In, "
    //   << std::fixed << block->nitems_written(0) << " Out\n";
  }
  std::cout << "File Ouputs: " << std::fixed << source->nitems_written(0) << "\n";
  for (gr::AltusDecoder::Decoder::sptr block : parser_blocks) {
    std::cout << "Passed " << block.get()->get_passed() << " of " << block.get()->get_parsed() << "\n";
  }
  std::cout << "All Work Time: " << std::fixed << std::setprecision(0) << all_work_time << "\n";
  std::cout << "Shift Work Time: " << std::fixed << std::setprecision(0) << total_work_time << "\n";
  std::cout << "Total Squelch Samples: " << std::fixed << std::setprecision(0) << total_squelch_samples << "\n";
}
