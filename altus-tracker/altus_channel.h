#ifndef ALTUS_CHANNEL_H
#define ALTUS_CHANNEL_H

// GNU Radio Blocks
#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/blocks/rotator_cc.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/digital/constellation.h>
#include <gnuradio/digital/fll_band_edge_cc.h>
#include <gnuradio/digital/symbol_sync_ff.h>
#include <gnuradio/digital/interpolating_resampler_type.h>
#include <gnuradio/digital/timing_error_detector_type.h>
#include <gnuradio/filter/fft_filter_ccc.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/message.h>
#include <gnuradio/msg_queue.h>

#include <functional>
#include <string>
#include <vector>

#include "constants.h"
#include "altus_decoder.h"
#include "altus_packet.h"

#ifdef gnuradio_Altus_Decoder_EXPORTS
#define ALTUS_DECODER_API __GR_ATTR_EXPORT
#else
#define ALTUS_DECODER_API __GR_ATTR_IMPORT
#endif

class AltusChannel;

typedef std::shared_ptr<AltusChannel> altus_channel_sptr;


/**
 * @brief Generate an altus channel block
 * 
 * @param handle_message Callback when a message is received
 * @param channel_freq The frequency of the channel (in Hz)
 * @param center_freq The frequency of the receiver (in Hz)
 * @param input_sample_rate The starting sample rate
 * @return altus_channel_sptr The Altus Channel block
 */
altus_channel_sptr make_altus_channel(
  double channel_freq,
  double center_freq,
  double input_sample_rate
);

class AltusChannel : public gr::hier_block2 {
  /**
   * @brief Generate an altus channel block
   * 
   * @param handle_message Callback when a message is received
   * @param channel_freq The frequency of the channel (in Hz)
   * @param center_freq The frequency of the receiver (in Hz)
   * @param input_sample_rate The starting sample rate
   * @return altus_channel_sptr The Altus Channel block
   */
  friend altus_channel_sptr make_altus_channel(
    double channel_freq,
    double center_freq,
    double input_sample_rate
  );

  private:
    // Channel information
    double center_freq;
    double input_sample_rate;

    // Altus channel constants
    const uint8_t samples_per_symbol = 5;
    const uint32_t symbol_rate = 38400;
    const uint32_t channel_rate = samples_per_symbol * symbol_rate;
    const uint16_t fsk_deviation = 20500;

    // Parsing constants
    const float first_stage_channel_width = 4;
    const int32_t power_squelch_level = -25;

    // Internal blocks
    gr::filter::fft_filter_ccc::sptr first_stage_filter;
    gr::blocks::rotator_cc::sptr xlat_rotator;
    gr::filter::fft_filter_ccf::sptr second_stage_filter;
    gr::filter::pfb_arb_resampler_ccf::sptr arb_resampler;
    gr::analog::pwr_squelch_cc::sptr squelch;
    gr::digital::fll_band_edge_cc::sptr fll_band_edge;
    gr::analog::quadrature_demod_cf::sptr fmdemod;
    gr::digital::symbol_sync_ff::sptr clock_recovery;
    gr::digital::binary_slicer_fb::sptr slicer;
    gr::AltusDecoder::Decoder::sptr altus_decode;

  public:
    double channel_freq;

    /**
     * @brief Construct a new Altus Channel:: Altus Channel object
     * 
     * @param handle_message_cb The callback function to use when a message is found 
     * @param channel The channel frequency
     * @param center The center receiver frequency
     * @param s The receiver sample rate
     */
    AltusChannel(
      double channel_freq,
      double center_freq,
      double input_sample_rate
    );

    /**
     * @brief Destroy the Altus Channel:: Altus Channel object
     */
    ~AltusChannel();

    /**
     * @brief Internal message handler
     * Adds the channel information to the message and passes it to the global handler
     * @param message Bytes of the message
     * @param computed_crc Computed CRC from the message bytes
     * @param received_crc
     */
    void handle_message(
      uint8_t message[BYTES_PER_MESSAGE],
      uint16_t computed_crc,
      uint16_t received_crc
    );
    
    /**
     * @brief Set the center frequency of the receiver
     * @TODO
     * @param c The new center frequency
     */
    void set_center(double c);

    // Message queue
    std::vector<std::string> packet_queue;
    std::mutex packet_queue_mutex;
};

#endif