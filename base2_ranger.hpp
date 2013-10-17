
// Based on James Bonfield's fqz_comp

#ifndef ZP_BASES_RANGER_H
#define ZP_BASES_RANGER_H

#ifndef ZP_RANGECODER_H
#include "range_coder.hpp"
#endif
#include "common.hpp"

class BasesRanger {

    UCHAR stats[4] ;

    enum { STEP = 1 ,
           WSIZ = 256 };
    
    void normalize() {
        for (int i = 0; i < 4; i++)
            stats[i] -= stats[i] >>1 ;
    }

    int getsum() {
        int sum = (stats[0] + stats[1]) + (stats[2] + stats[3]);
        if (sum < WSIZ) return sum;
        normalize();
        return    (stats[0] + stats[1]) + (stats[2] + stats[3]);
    }

public:
    void init() { // BZERO made it even slower
        for (int i = 0; i < 4; i++)
            stats[i] = STEP;
    }

    inline void put(RangeCoder *rc, UCHAR sym) {

        int vtot = getsum();

        switch(sym) {
        case 0:
            rc->Encode( 0,
                        stats[0], vtot); break;
        case 1:
            rc->Encode( stats[0],
                        stats[1], vtot); break;
        case 2:
            rc->Encode( stats[0] + stats[1] , 
                        stats[2], vtot); break;
        case 3:
            rc->Encode((stats[0] + stats[1]) + (stats[2]),
                        stats[3], vtot); break;

        }
        stats[sym] += STEP;
    }

    inline UCHAR get(RangeCoder *rc) {
        int  vtot = getsum();
        UINT32 freq = rc->GetFreq(vtot);

        UINT32 sumfreq = 0;
        UINT32 i;
        for (i = 0; i < 4; i++) {
            if (sumfreq +  stats[i] <= freq)
                sumfreq += stats[i];
            else
                break;
        }
        assert(i<4);
        rc->Decode(sumfreq, stats[i], vtot);
        stats[i] += STEP;
        return i;
    }
} PACKED ;

#endif  // ZP_BASES_RANGER_H
