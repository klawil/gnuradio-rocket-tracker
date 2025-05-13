#include <gnuradio/logger.h>
// #include <gnuradio/top_block.h>
// #include <gnuradio/blocks/file_source.h>
// #include <gnuradio/blocks/throttle.h>
// #include <gnuradio/blocks/rotator_cc.h>
// #include <gnuradio/Altus/FindSyncWord.h>
// #include <gnuradio/gr_complex.h>
#include <iostream>
#include <boost/program_options.hpp>
#include <string>
#include <math.h>

#include "constants.h"
// #include "source.h"

gr::logger logger("altus-tracker");

uint32_t input_sample_rate = BAUD_RATE * 66;
uint32_t input_center_freq = 434800000;

uint32_t target_sample_rate = BAUD_RATE * 3;
uint32_t target_center_freq = 435250000;

// Source* make_source_block(gr::top_block_sptr tb) {
//   return new Source(
//     435050000,
//     SAMPLE_RATE,
//     ""
//   );
// }

// gr::blocks::throttle::sptr make_file_source(gr::top_block_sptr tb) {
//   gr::blocks::file_source::sptr file = gr::blocks::file_source::make(
//     sizeof(gr_complex),
//     "/Users/william/output_test_file.data"
//   );

//   gr::blocks::throttle::sptr throttle = gr::blocks::throttle::make(
//     sizeof(gr_complex),
//     input_sample_rate
//   );
//   tb->connect(file, 0, throttle, 0);

//   return throttle;
// }

int main(int argc, char **argv) {
  boost::program_options::options_description desc("Options");
  desc.add_options()
    ("help,h", "Help screen")
    ("version,v", "Version Information");

  boost::program_options::variables_map vm;
  boost::program_options::store(parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);

  if (vm.count("version")) {
    std::cout << "Version 0.0.1\n";
    exit(0);
  }

  if (vm.count("help")) {
    std::cout << "Usage: trunk-recorder [options]\n";
    std::cout << desc;
    exit(0);
  }

  // gr::top_block_sptr tb = gr::make_top_block("Altus");

  // gr::blocks::throttle::sptr file_source = make_file_source(tb);

  // gr::blocks::rotator_cc::sptr freq_shift = gr::blocks::rotator_cc::make(
  //   2 * M_PI * ((double)(input_center_freq) - target_center_freq) / (double)(input_sample_rate)
  // );
  // tb->connect(file_source, 0, freq_shift, 0);

  logger.warn("test 2");
}
