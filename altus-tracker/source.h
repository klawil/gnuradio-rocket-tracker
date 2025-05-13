#ifndef SOURCE_H
#define SOURCE_H

#include <osmosdr/source.h>

class Source {
  private:
    double center;
    double rate;
    std::string device;
    double error;
    double ppm;
    osmosdr::source::sptr source_block;

  public:
    Source(double center_freq, double sample_rate, std::string dev);
    osmosdr::source::sptr get_src_block();
};

#endif
