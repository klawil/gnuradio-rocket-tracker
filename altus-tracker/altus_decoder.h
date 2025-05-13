#ifndef INCLUDED_ALTUS_DECODER_H
#define INCLUDED_ALTUS_DECODER_H

#include <gnuradio/block.h>
#include <gnuradio/attributes.h>

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
        static sptr make();
    };
  }
}

#endif

