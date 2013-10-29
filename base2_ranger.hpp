
// Based on James Bonfield's fqz_comp

#ifndef ZP_BASES_RANGER_H
#define ZP_BASES_RANGER_H

#ifndef ZP_RANGECODER_H
#include "coder.hpp"
#endif
#include "common.hpp"

class Base2Ranger {

    enum { STEP = 1,
           // MAX_FREQ=(1<<16)-32,
           MAX_FREQ=250,
    };
    
    // UINT16 total;
    UCHAR  freq[4];

    void normalize() {
        // total =
        //     // pipe it up
        //     ( (freq[0] -= (freq[0]>>1)) + 
        //       (freq[1] -= (freq[1]>>1)) ) +
        //     ( (freq[2] -= (freq[2]>>1)) +
        //       (freq[3] -= (freq[3]>>1)) ) ;
        (freq[0] -= (freq[0]>>1),
         freq[1] -= (freq[1]>>1)),
        (freq[2] -= (freq[2]>>1),
         freq[3] -= (freq[3]>>1));
        // freq[0] /= 2;
        // freq[1] /= 2;
        // freq[2] /= 2;
        // freq[3] /= 2;
    }

    inline UINT16 getsum() {
        // return 4 + (freq[0] + freq[1]) + (freq[2] + freq[3]);
        return (freq[0] + freq[1]) + (freq[2] + freq[3]);
    }

public:
    void init() { // BZERO made it even slower
        for (int i = 0; i < 4; i++)
            freq[i] = 2;
        // total = 4;
    }

    inline void put(RCoder *rc, UCHAR sym) {
        UINT16 total = getsum();
        switch(sym) {
        case 0: rc->Encode( 0,
                            freq[0], total); break;
        case 1: rc->Encode( freq[0],
                            freq[1], total); break;
        case 2: rc->Encode( freq[0] + freq[1] , 
                            freq[2], total); break;
        case 3: rc->Encode( freq[0] + (freq[1] + freq[2]),
                            freq[3], total); break;
        // case 0: rc->Encode( 0,
        //                     1+ freq[0], total); break;
        // case 1: rc->Encode( 1+ freq[0],
        //                     1+ freq[1], total); break;
        // case 2: rc->Encode( 2+ freq[0] + freq[1] , 
        //                     1+ freq[2], total); break;
        // case 3: rc->Encode( 3+ freq[0] + (freq[1] + freq[2]),
        //                     1+ freq[3], total); break;
        }

        rarely_if(freq[sym] > (MAX_FREQ))
            normalize();

        freq[sym] += STEP;
    }

    inline UCHAR get(RCoder *rc) {

        UINT16 total = getsum();
        UINT32 prob = rc->GetFreq(total);

        UINT32 sumf = 0;
        int i;
        for (i = 0; i < 4; i++) {
            // if (sumf +  freq[i] + 1 <= prob)
            //     sumf += freq[i] + 1;
            if (sumf +  freq[i] <= prob)
                sumf += freq[i];
            else
                break;
        }
        assert(i<4);
        // rc->Decode(sumf, freq[i] +1, total);
        rc->Decode(sumf, freq[i], total);

        rarely_if(freq[i] > (MAX_FREQ))
            normalize();

        freq[i] += STEP;

        return i;
    }
} PACKED ;

#endif  // ZP_BASES_RANGER_H
