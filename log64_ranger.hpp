

// Based on James Bonfield's fqz_comp

#ifndef ZP_LOG64_RANGER_H
#define ZP_LOG64_RANGER_H

#ifndef ZP_RANGECODER_H
#include "coder.hpp"
#endif

class Log64Ranger {
    enum {
        STEP=8,
        NSYM=64,
        MAX_FREQ=(1<<16)-32,
    };

    UINT32 total;
    UINT16 freq[NSYM];
    UINT16 iend ;

    UCHAR count;
    UCHAR syms[NSYM];

    void normalize() {
        for (UINT32 i = total = 0; i < iend; i++)
            total += (freq[i] /= 2);
    }

    inline UCHAR sort_of_sort(int i) {
        likely_if (i == 0 or
                   (++ count & 0xf) or
                   freq[i] <= freq[i-1] )
            return syms[i];

        UCHAR c = syms[i  ];
        syms[i] = syms[i-1];
        syms[i-1] = c;

        UINT16 f = freq[i];
        freq[i]  = freq[i-1];
        freq[i-1] = f;

        return c;
    }

public:

    inline void put(RCoder *rc, UCHAR sym) {
        UINT32 sumf  = 0;
        UINT32 i = 0;

        assert(sym < 64);
        rarely_if(iend <= sym)
            for (;iend <= sym; iend++)
                syms[iend] = iend;

        for (; syms[i] != sym and LIKELY(i < iend); sumf += freq[i++]);
        sumf += i;

        UINT32 vtot = total + NSYM;
        rc->Encode(sumf, freq[i]+1, vtot);

        rarely_if(freq[i] > (MAX_FREQ - STEP))
            normalize();

        freq[i] += STEP;
        total   += STEP;

        sort_of_sort(i);
    }

    inline UINT16 get(RCoder *rc) {

        UINT32 vtot = total + NSYM;
        UINT32 sumf = 0;
        UINT32 i = 0;

        UINT32 prob = rc->GetFreq(vtot);

        for ( i = 0;
              i < NSYM;
              i ++ ) {

            rarely_if(i >= iend) 
                syms[ iend++ ] = i;
        
            if (sumf +  freq[i] + 1 <= prob)
                sumf += freq[i] + 1;
            else
                break;
        }

        rc->Decode(sumf, freq[i]+1, vtot);

        rarely_if(freq[i] > (MAX_FREQ - STEP))
            normalize();

        freq[i] += STEP;
        total   += STEP;

        return sort_of_sort(i);
    }

} PACKED;

#endif // already loaded
