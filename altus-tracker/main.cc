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
#include "altus_channel.h"
#include "altus_packet.h"

int socketConn = -1;
int pipeFd = -1;

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

std::vector<std::string> packet_queue;
std::mutex packet_queue_mutex;
void message_handler(
  AltosBasePacket *packet
) {
  std::string msg = packet->to_string() + "\n";
  packet_queue_mutex.lock();
  packet_queue.push_back(msg);
  packet_queue_mutex.unlock();
}

void open_socket(std::string host, uint16_t port) {
  // Open the socket (synchronously)
  if ((socketConn = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cout << "Failed to create socket: " << strerror(errno) << std::endl;
    return;
  }

  // Specify the address
  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  if (inet_pton(AF_INET, host.c_str(), &serverAddress.sin_addr) <= 0) {
    std::cout << "Failed to set socket address to " << host << ": " << strerror(errno) << std::endl;
    close(socketConn);
    socketConn = -1;
    return;
  }

  // Send the connection request
  if (
    connect(socketConn, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0 &&
    errno != EINPROGRESS
  ) {
    std::cout << "Failed to open socket: " << strerror(errno) << std::endl;
    close(socketConn);
    socketConn = -1;
    return;
  } else {
    std::cout << "Socket opening in progress" << std::endl;
  }

  static char init_packet_buffer[2] = { 0 };
  bool is_connected = false;
  bool keep_loop = true;
  while (true) {
    // Read some bytes
    ssize_t valread;
    bool is_break = false;
    while ((valread = read(socketConn, &init_packet_buffer[0], 2)) != 0) {
      std::cout << "Received initial bytes: " << init_packet_buffer[0] << init_packet_buffer[1] << std::endl;
      is_connected = true;
      is_break = true;
      break;
    }
    if (is_break) {
      break;
    }

    // Check for errors
    if (
      valread < 0 &&
      errno != EWOULDBLOCK &&
      errno != EAGAIN &&
      errno != ECONNREFUSED
    ) {
      std::cout << "Failed to read from socket: " << strerror(errno) << std::endl;
      break;
    } else if (
      errno > 0 &&
      errno != EWOULDBLOCK &&
      errno != EAGAIN
    ) {
      std::cout << "Failed to read from socket(2): " << strerror(errno) << std::endl;
      break;
    }
  }

  if (is_connected) {
    std::cout << "Socket open at " << host << ":" << port << std::endl;
  } else {
    std::cout << "Socket failed to open" << std::endl;
    close(socketConn);
    socketConn = -1;
  }
}

void open_pipe(std::string pipe) {
  // Open the pipe (synchronously)
  mkfifo(pipe.c_str(), 0666);
  pipeFd = open(pipe.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (pipeFd == -1) {
    std::cout << "Error opening pipe: " << strerror(errno) << std::endl;
    return;
  }

  std::cout << "Pipe opened at " << pipe << std::endl;
}

std::chrono::milliseconds last_message = duration_cast< std::chrono::milliseconds >(
  std::chrono::system_clock::now().time_since_epoch()
);
const uint32_t min_ping_wait = 5000; // in ms
const std::string ping_message = "ping\n";

void process_queue(
  bool use_pipe,
  std::string pipe_name,
  std::string socket_host,
  uint16_t socket_port
) {
  // Set up the pipe or socket
  if (use_pipe) {
    open_pipe(pipe_name);
  } else {
    open_socket(socket_host, socket_port);
  }

  while (running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    packet_queue_mutex.lock();
    bool sent_message = false;
    for (std::vector<std::string>::iterator msg_ptr = packet_queue.begin(); msg_ptr != packet_queue.end(); msg_ptr++) {
      std::string msg = *msg_ptr;

      // Write to pipe (if indicated)
      if (pipeFd != -1) {
        sent_message = true;
        write(pipeFd, msg.c_str(), msg.length());
      } else if (socketConn != -1) {
        sent_message = true;
        write(socketConn, msg.c_str(), msg.length());
      }
    }
    packet_queue.clear();
    packet_queue_mutex.unlock();
    if (sent_message) {
      last_message = duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
      );
    } else {
      std::chrono::milliseconds now = duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
      );
      if (uint32_t(now.count() - last_message.count()) >= min_ping_wait) {
        if (pipeFd != -1) {
          write(pipeFd, ping_message.c_str(), ping_message.length());
        } else if (socketConn != -1) {
          write(socketConn, ping_message.c_str(), ping_message.length());
        } else if (use_pipe) {
          open_pipe(pipe_name);
        } else {
          open_socket(socket_host, socket_port);
        }
        last_message = now;
      }
    }
  }

  if (pipeFd != -1) {
    close(pipeFd);
  }
  if (socketConn != -1) {
    close(socketConn);
  }
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
    ("socket", po::value<std::string>(), "Socket IP to connect to (if using sockets)")
    ("port", po::value<uint16_t>(), "Socket port to connect to (if using sockets)")
    ("pipe,p", po::value<std::string>(), "Pipe file to connect to")
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

  std::string pipe_name = "/tmp/altus-tracker-fifo-out";
  std::string socket_host;
  uint16_t socket_port = 8765;
  bool use_pipe = true;
  if (vm.count("socket")) {
    use_pipe = false;
    socket_host = vm["socket"].as<std::string>();
    if (vm.count("port")) {
      socket_port = vm["port"].as<uint16_t>();
    }
  } else if (vm.count("pipe")) {
    pipe_name = vm["pipe"].as<std::string>();
  }

  std::thread packet_writer (
    process_queue,
    use_pipe,
    pipe_name,
    socket_host,
    socket_port
  );

  tb = gr::make_top_block("Altus");

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
  channels.push_back(436550000); // Wm

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

  // std::this_thread::sleep_for(std::chrono::seconds(3));
  // std::cout << "\nAdding channel\n\n";
  // uint32_t new_chan = 436550000;
  // add_channel(new_chan);

  // std::this_thread::sleep_for(std::chrono::seconds(3));
  // std::cout << "\nRemoving channel\n\n";
  // rm_channel(new_chan);

  // std::this_thread::sleep_for(std::chrono::seconds(3));
  // std::cout << "\nAdding channel\n\n";
  // add_channel(new_chan);

  // std::this_thread::sleep_for(std::chrono::seconds(3));
  // std::cout << "\nRemoving channel\n\n";
  // rm_channel(new_chan);

  // std::this_thread::sleep_for(std::chrono::seconds(3));
  // std::cout << "\nAdding channel\n\n";
  // add_channel(new_chan);

  tb->wait();

  tb->stop();
  std::cout << "\nDone Running\n";
  running = false;
  packet_writer.join();
}
