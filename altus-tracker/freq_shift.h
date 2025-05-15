#ifndef INCLUDED_FREQ_SHIFT_H
#define INCLUDED_FREQ_SHIFT_H

#include <gnuradio/block.h>
#include <gnuradio/attributes.h>
#include <gnuradio/sync_block.h>
#include <string>

#ifdef gnuradio_Freq_Shift_EXPORTS
#define FREQ_SHIFT_API __GR_ATTR_EXPORT
#else
#define FREQ_SHIFT_API __GR_ATTR_IMPORT
#endif

namespace gr {
  namespace FreqShift {
    class FREQ_SHIFT_API DoShift : virtual public gr::sync_block {
      public:
        typedef std::shared_ptr<DoShift> sptr;
        static sptr make(
          int channel_freq_offset,
          uint32_t sample_rate
        );
    };
  }
}

#endif
