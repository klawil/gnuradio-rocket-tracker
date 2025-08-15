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

#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "constants.h"
#include "blocks/altus_channel.h"
#include "altus_packet.h"
#include "blocks/altus_power_level.h"
#include "blocks/altus_detector.h"

int socket_conn = -1;
bool socket_connected = false;

namespace po = boost::program_options;

gr::top_block_sptr tb;
bool running = true;

// These stay constant
const double sample_rate = 10000000;
// const double sample_rate = 2500000;
const char * data_file = "../data.cfile";
uint32_t input_center_freq = 434800000;
// uint32_t input_center_freq = 435750000;

gr::basic_block_sptr source;
const int channel_count = 10;
altus_channel_sptr channel_blocks[channel_count];
int channel_idx = 0;

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

void open_socket(std::string host, uint16_t port) {
  // Check for already connected socket
  if (socket_connected) {
    std::cout << "Tried to open already open socket" << std::endl;
    return;
  }

  // Open the socket (synchronously)
  if ((socket_conn = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cout << "Failed to create socket: " << strerror(errno) << std::endl;
    return;
  }

  // Specify the address
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  if (inet_pton(AF_INET, host.c_str(), &server_address.sin_addr) <= 0) {
    std::cout << "Failed to set socket address to " << host << ": " << strerror(errno) << std::endl;
    close(socket_conn);
    socket_conn = -1;
    return;
  }

  // Make the socket async
  int flags = fcntl(socket_conn, F_GETFL, 0);
  if (flags == -1) {
    flags = 0;
  }
  flags = (flags | O_NONBLOCK);
  if (fcntl(socket_conn, F_SETFL, flags) != 0) {
    std::cout << "Failed to set socket flags: " << strerror(errno) << std::endl;
    close(socket_conn);
    socket_conn = -1;
    return;
  }

  // Send the connection request
  if (
    connect(socket_conn, (struct sockaddr*)&server_address, sizeof(server_address)) < 0 &&
    errno != EINPROGRESS
  ) {
    std::cout << "Failed to open socket: " << strerror(errno) << std::endl;
    close(socket_conn);
    socket_conn = -1;
    return;
  } else {
    std::cout << "Socket opening in progress" << std::endl;
  }
}

std::chrono::milliseconds last_message = duration_cast< std::chrono::milliseconds >(
  std::chrono::system_clock::now().time_since_epoch()
);
const uint32_t min_ping_wait = 5000; // in ms
const std::string ping_message = "ping\n";

std::stringstream message_in;
std::vector<std::string> outgoing_messages;
std::mutex outgoing_messages_mutex;
void handle_message() {
  std::cout << "New message in: " << message_in.str() << std::endl;

  // Pull out the message and clear the buffer
  std::string msg = message_in.str();
  message_in.str(std::string());

  if (msg == "!!") {
    std::cout << "Init command" << std::endl;
    outgoing_messages_mutex.lock();
    for (int i = 0; i < channel_count; i++) {
      std::stringstream msg;
      msg << "c:" << std::fixed << std::setprecision(0) << channel_blocks[i]->channel_freq << "\n";
      std::cout << "Msg out: " << msg.str();
      outgoing_messages.push_back(msg.str());
    }
    outgoing_messages_mutex.unlock();
  }
}

void process_queue(
  std::string socket_host,
  uint16_t socket_port
) {
  // Set up the socket
  open_socket(socket_host, socket_port);

  while (running) {
    // Check for incoming messages
    if (socket_conn != -1) {
      ssize_t valread;
      static char buffer[1] = { 0 };
      while ((valread = read(socket_conn, &buffer[0], 1)) > 0) {
        socket_connected = true;

        if (buffer[0] == '\n') {
          handle_message();
        } else {
          message_in << buffer[0];
          std::cout << "Received: " << buffer[0] << std::endl;
        }
      }

      // Check for socket errors
      if (
        valread < 0 &&
        errno != EWOULDBLOCK &&
        errno != EAGAIN
      ) {
        std::cerr << "Failed to read from socket: " << strerror(errno) << std::endl;
        socket_conn = -1;
        socket_connected = false;
      }
    }

    bool sent_message = false;
    uint16_t packets_sent = 0;
    std::stringstream msg_value;
    for (int i = 0; i < channel_count; i++) {
      auto chan = channel_blocks[i];
      chan->packet_queue_mutex.lock();
      for (std::vector<std::string>::iterator msg_ptr = chan->packet_queue.begin(); msg_ptr != chan->packet_queue.end(); msg_ptr++) {
        std::string msg = *msg_ptr;
        msg_value << msg;
        packets_sent++;
      }
      chan->packet_queue.clear();
      chan->packet_queue_mutex.unlock();
    }
    outgoing_messages_mutex.lock();
    for (std::vector<std::string>::iterator msg_ptr = outgoing_messages.begin(); msg_ptr != outgoing_messages.end(); msg_ptr++) {
      std::string msg = *msg_ptr;
      msg_value << msg;
      packets_sent++;
    }
    outgoing_messages.clear();
    outgoing_messages_mutex.unlock();
    if (packets_sent > 0) {
      // Write to socket (if indicated)
      if (socket_connected) {
        std::string msg = msg_value.str();
        write(socket_conn, msg.c_str(), msg.length());
        sent_message = true;
      }
    }
    if (sent_message) {
      last_message = duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
      );
    } else {
      std::chrono::milliseconds now = duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
      );
      if (uint32_t(now.count() - last_message.count()) >= min_ping_wait) {
        if (socket_connected) {
          std::cout << "Ping after " << uint32_t(now.count() - last_message.count()) << std::endl;
          write(socket_conn, ping_message.c_str(), ping_message.length());
        } else if (socket_conn == -1) {
          open_socket(socket_host, socket_port);
        }
        last_message = now;
      }
    }
    
    // Wait before starting the next run
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  if (socket_conn != -1) {
    close(socket_conn);
  }
}

void add_channel(uint32_t channel_freq) {
  // Check to see if the channel already exists
  for (int i = 0; i < channel_count; i++) {
    if (channel_blocks[i]->channel_freq == channel_freq) {
      return;
    }
  }

  // Add the channel
  uint32_t channel_being_removed = channel_blocks[channel_idx]->channel_freq;
  channel_blocks[channel_idx]->set_channel(channel_freq);

  // Increment the index
  channel_idx++;
  if (channel_idx >= channel_count) {
    channel_idx = 0;
  }

  // Send a socket message (if open)
  if (socket_connected) {
    std::stringstream msg;
    msg << "r:" << channel_being_removed << "\n";
    msg << "c:" << channel_freq << "\n";

    outgoing_messages_mutex.lock();
    outgoing_messages.push_back(msg.str());
    outgoing_messages_mutex.unlock();
  }
}

void build_channel(uint32_t channel_freq) {
  // Add the channel
  altus_channel_sptr channel = make_altus_channel(
    channel_freq,
    double(input_center_freq),
    sample_rate
  );
  tb->connect(source, 0, channel, 0);
  channel_blocks[channel_idx] = channel;
  channel_idx++;
  if (channel_idx >= channel_count) {
    channel_idx = 0;
  }

  // // Add the channel message
  // if (socket_connected) {
  //   std::stringstream msg;
  //   msg << "c:" << channel_freq << "\n";

  //   outgoing_messages_mutex.lock();
  //   outgoing_messages.push_back(msg.str());
  //   outgoing_messages_mutex.unlock();
  // }
}

int main(int argc, char **argv) {
  // uint8_t byte0 = 0x76;
  // uint8_t byte1 = 0xf1;
  // uint8_t byte2 = 0xa1;
  // uint8_t byte3 = 0x16;
  // uint32_t v1 = byte0 + (byte1 << 8) + (byte2 << 16) + (byte3 << 24);
  // std::cout << "UInt: " << std::fixed << std::setprecision(0) << v1 << std::endl;
  // std::cout << "Int: " << std::fixed << std::setprecision(0) << int32_t(v1) << std::endl;
  // std::cout << "Float: " << std::fixed << std::setprecision(7) << (int32_t(v1) * 1e-7) << std::endl;
  // return 0;

  po::options_description desc("Options");
  desc.add_options()
    ("help,h", "Help screen")
    ("version,v", "Version Information")
    ("center_freq,c", po::value<uint32_t>(), "Input center frequency")
    ("squelch", po::value<int32_t>(), "Squelch level for power squelch")
    ("file,f", po::value<std::string>(), "File to use as a source (complex data)")
    ("socket", po::value<std::string>(), "Socket IP to connect to (default 127.0.0.1)")
    ("port", po::value<uint16_t>(), "Socket port to connect to (default 8765)")
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

  // Build the top block
  tb = gr::make_top_block("Altus");

  // Build the input
  std::string source_type = "sdr";
  if (vm.count("file")) {
    data_file = vm["file"].as<std::string>().c_str();
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

  // Generate all of the channel blocks
  build_channel(434550000); // Ch 01
  build_channel(434650000); // Ch 02
  build_channel(434750000); // Ch 03
  build_channel(434850000); // Ch 04
  build_channel(434950000); // Ch 05
  build_channel(435050000); // Ch 06
  build_channel(435150000); // Ch 07
  build_channel(435250000); // Ch 08
  build_channel(435350000); // Ch 09
  build_channel(435450000); // Ch 10

  // Parse the socket options
  std::string socket_host = "127.0.0.1";
  uint16_t socket_port = 8765;
  if (vm.count("socket")) {
    socket_host = vm["socket"].as<std::string>();
  }
  if (vm.count("port")) {
    socket_port = vm["port"].as<uint16_t>();
  }

  // Open the socket and wait for events
  std::thread packet_writer (
    process_queue,
    socket_host,
    socket_port
  );

  // Build the detector
  uint16_t fft_size = 2048 * 2;
  auto b1 = make_altus_power_level(
    sample_rate,
    fft_size
  );
  tb->connect(source, 0, b1, 0);
  auto b2 = gr::AltusDecoder::Detector::make(
    add_channel,
    input_center_freq,
    sample_rate,
    fft_size,
    5
  );
  tb->connect(b1, 0, b2, 0);

  tb->start();
  std::signal(SIGINT, &signal_handler);
  std::cout << "\nRunning, press Ctrl + C to exit\n\n";

  tb->wait();

  // tb->stop();
  std::cout << "\nDone Running\n\n";
  running = false;
  packet_writer.join();
}
