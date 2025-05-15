#ifndef INCLUDED_ALTUS_DECODER_H
#define INCLUDED_ALTUS_DECODER_H

#include <gnuradio/block.h>
#include <gnuradio/attributes.h>
#include <string>

#ifdef gnuradio_Altus_Decoder_EXPORTS
#define ALTUS_DECODER_API __GR_ATTR_EXPORT
#else
#define ALTUS_DECODER_API __GR_ATTR_IMPORT
#endif

namespace gr {
  namespace AltusDecoder {
    class ALTUS_DECODER_API Decoder : virtual public gr::block {
      public:
        typedef std::shared_ptr<Decoder> sptr;
        static sptr make(
          std::string source_type_input,
          uint32_t channel_freq_input,
          uint16_t channel_num_input
        );
    };
  }
}

#endif
