#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp> 

#include <gnuradio/block.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/blocks/file_source.h>
#include <gnuradio/blocks/head.h>
#include <gnuradio/blocks/throttle.h>
#include <gnuradio/top_block.h>

#include <osmosdr/source.h>

#include <chrono>
#include <csignal>
#include <iostream>
#include <math.h>
#include <string>
#include <thread>
#include <vector>

#include "constants.h"
#include "altus_channel.h"
#include "altus_packet.h"

namespace po = boost::program_options;

gr::top_block_sptr tb;
bool running = true;

// These stay constant
const double sample_rate = 10000000;
const char * data_file = "../data.cfile";
uint32_t input_center_freq = 434800000;

gr::basic_block_sptr source;
std::vector<uint32_t> channels;
std::vector<altus_channel_sptr> channel_blocks;

gr::block_sptr make_file_source(
  gr::top_block_sptr tb,
  const char * file_path,
  bool do_throttle
) {
  gr::blocks::file_source::sptr file = gr::blocks::file_source::make(
    sizeof(gr_complex),
    file_path
  );

  if (!do_throttle) {
    return file;
  }

  gr::blocks::throttle::sptr throttle = gr::blocks::throttle::make(
    sizeof(gr_complex),
    sample_rate
  );
  tb->connect(file, 0, throttle, 0);

  return throttle;
}

void signal_handler(int signal) {
  if (signal == SIGINT) {
    std::cout << "SIGINT received. Cleaning up and exiting..." << std::endl;
    tb->stop();
    running = false;
  }
}

void message_handler(
  AltosBasePacket *packet
) {
  std::cout << packet->to_string() << std::endl;
}

void add_channel(uint32_t channel_freq) {
  // Check to see if the channel already exists
  for (std::vector<uint32_t>::iterator c = channels.begin(); c != channels.end(); c++) {
    if (*c == channel_freq) {
      return;
    }
  }

  // Add the channel
  tb->lock();
  channels.push_back(channel_freq);
  altus_channel_sptr channel = make_altus_channel(
    message_handler,
    channel_freq,
    double(input_center_freq),
    double(sample_rate)
  );
  tb->connect(source, 0, channel, 0);
  channel_blocks.push_back(channel);
  tb->unlock();
}

void rm_channel(uint32_t channel_freq) {
  // Check to see if the channel already exists
  uint8_t exists = 0;
  for (std::vector<uint32_t>::iterator c = channels.begin(); c != channels.end(); c++) {
    if (*c == channel_freq) {
      exists = 1;
    }
  }
  if (exists == 0) {
    return;
  }

  // Remove the channel
  tb->lock();
  for (size_t ci = channel_blocks.size() - 1; ci >= 1; ci--) {
    altus_channel_sptr cb = channel_blocks[ci];
    if (cb->channel_freq == double(channel_freq)) {
      tb->disconnect(cb);
      channel_blocks.erase(channel_blocks.begin() + ci);

      break;
    }
  }

  for (size_t ci = channels.size() - 1; ci >= 1; ci--) {
    if (channels[ci] == channel_freq) {
      channels.erase(channels.begin() + ci);
      break;
    }
  }
  tb->unlock();
}

int main(int argc, char **argv) {
  po::options_description desc("Options");
  desc.add_options()
    ("help,h", "Help screen")
    ("version,v", "Version Information")
    ("center_freq,c", po::value<uint32_t>(), "Input center frequency")
    ("squelch", po::value<int32_t>(), "Squelch level for power squelch")
    ("file,f", po::value<std::string>(), "File to use as a source (complex data)")
    ("throttle", "Throttle (only applies to file source)");

  po::variables_map vm;
  po::store(parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("version")) {
    std::cout << "Version 0.0.1\n";
    return 0;
  }

  if (vm.count("help")) {
    std::cout << "Usage: trunk-recorder [options]\n";
    std::cout << desc;
    return 0;
  }

  tb = gr::make_top_block("Altus");

  std::string source_type = "sdr";
  if (vm.count("file")) {
    std::cout << "Using data from file " << data_file << "\n";
    source = make_file_source(
      tb,
      data_file,
      vm.count("throttle") > 0
    );
    source_type = "file";
  } else {
    osmosdr::source::sptr osmo_source = osmosdr::source::make();
    osmo_source->set_sample_rate(sample_rate);
    osmo_source->set_center_freq(input_center_freq);
    osmo_source->set_gain_mode(true);
    source = osmo_source;
    std::cout << "Radio source " << input_center_freq << "\n";

    gr::blocks::file_sink::sptr file = gr::blocks::file_sink::make(
      sizeof(gr_complex),
      data_file,
      false
    );
    tb->connect(source, 0, file, 0);
  }

  // Generate all of the channel frequencies
  channels.push_back(434550000); // CH 01
  channels.push_back(434650000); // CH 02
  channels.push_back(434750000); // CH 03
  channels.push_back(434850000); // CH 04
  channels.push_back(434950000); // CH 05
  channels.push_back(435050000); // CH 06
  channels.push_back(435150000); // CH 07
  channels.push_back(435250000); // CH 08
  channels.push_back(435350000); // CH 09
  channels.push_back(435450000); // CH 10
  // channels.push_back(436550000); // Wm

  for (uint8_t c = 0; c < channels.size(); c++) {
    altus_channel_sptr channel = make_altus_channel(
      message_handler,
      double(channels[c]),
      double(input_center_freq),
      double(sample_rate)
    );
    tb->connect(source, 0, channel, 0);
    channel_blocks.push_back(channel);
  }

  tb->start();
  std::signal(SIGINT, &signal_handler);
  std::cout << "\nRunning, press Ctrl + C to exit\n\n";

  std::this_thread::sleep_for(std::chrono::seconds(3));
  std::cout << "\nAdding channel\n\n";
  uint32_t new_chan = 436550000;
  add_channel(new_chan);

  std::this_thread::sleep_for(std::chrono::seconds(3));
  std::cout << "\nRemoving channel\n\n";
  rm_channel(new_chan);

  std::this_thread::sleep_for(std::chrono::seconds(3));
  std::cout << "\nAdding channel\n\n";
  add_channel(new_chan);

  std::this_thread::sleep_for(std::chrono::seconds(3));
  std::cout << "\nRemoving channel\n\n";
  rm_channel(new_chan);

  std::this_thread::sleep_for(std::chrono::seconds(3));
  std::cout << "\nAdding channel\n\n";
  add_channel(new_chan);

  tb->wait();

  std::cout << "\nDone Running\n";
  tb->stop();
}
