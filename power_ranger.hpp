

// Based on James Bonfield's fqz_comp

#ifndef ZP_POWER_RANGER_H
#define ZP_POWER_RANGER_H

#ifndef ZP_RANGECODER_H
#include "range_coder.hpp"
#endif

// #define MAX_FREQ (1<<16)-32
#define MAX_FREQ (1<<16)-32

template <int NBITS>
class PowerRanger {
    enum {
        STEP=3,
        NSYM=(1<<NBITS),
    };

    UINT32 total;     // Total frequency
    UINT16 counter;   // Periodic counter for bubble sort step
    UINT16 iend ;     // keep it 4 bytes alignment

    // Array of Symbols approximately sorted by Freq. 
    struct SymFreqs {
        UCHAR symbol;
        UCHAR freq;
        // UINT16 symbol;
        // UINT16 freq;
    } PACKED F[NSYM];

    void normalize() {
        total=0;
        for (SymFreqs* s = F; s < F+iend; s++)
            total +=
                (s->freq -= (s->freq>>1));

        // for (UINT32 i = 0; i <= iend; i++)
        //     total += 
        //         F[i].freq -= F[i].freq >> 1;

        /* Faster than F[i].Freq for 0 <= i < NSYM */
        // for (SymFreqs *s = F; s->Freq; s++) {
        //     s->Freq -= s->Freq>>1;
        //     total += s->Freq;
        // }
    }

    UINT32 expand_to(UINT16 sym) {
        assert(sym >= iend);
        assert(sym < NSYM);
        while (iend <= sym) {
            F[iend].freq = 1;
            F[iend].symbol = iend;
            iend ++;
            total ++;
        }
        return total;
    }

public:

    inline void put(RangeCoder *rc, UINT16 sym) {
        UINT32 sumfreq  = 0;
        UINT32 i = 0;

        likely_if( sym < iend)
            while (F[i].symbol != sym)
                sumfreq += F[i++].freq ;
        else
            sumfreq = (expand_to((i=sym))-1);

        UINT32 vtot = total + NSYM - iend;
        rc->Encode(sumfreq, F[i].freq, vtot);

        rarely_if(F[i].freq > (255 - STEP))
            normalize();

        F[i].freq += STEP;
        total += STEP;

        // if (vtot + STEP > MAX_FREQ)
        //     normalize();

        rarely_if
            (((++counter&15)==0) and
             i and
             F[i].freq > F[i-1].freq) {
            SymFreqs t = F[i];
            F[i] = F[i-1];
            F[i-1] = t;
        }
    }

    inline UINT16 get(RangeCoder *rc) {

        UINT32 vtot = total + NSYM - iend;
        UINT32 freq = rc->GetFreq(vtot);
        UINT32 sumfreq = 0;
        UINT32 i;

        for ( i = 0;
              i < NSYM;
              i ++ ) {
            rarely_if(i >= iend)
                expand_to(i);

            if (sumfreq +  F[i].freq <= freq)
                sumfreq += F[i].freq;
            else
                break;
        }

        rc->Decode(sumfreq, F[i].freq, vtot);

        rarely_if(F[i].freq > (255 - STEP))
            normalize();

        F[i].freq += STEP;
        total += STEP;

        // if (vtot + STEP > MAX_FREQ)
        //     normalize();

        /* Keep approx sorted */
        if (((++counter&15)==0) and
            i and
            F[i].freq > F[i-1].freq) {
            SymFreqs t = F[i];
            F[i] = F[i-1];
            F[--i] = t;
        }
        return F[i].symbol;
    }

} PACKED;

#undef MAX_FREQ 

#endif // ZP_POWER_RANGER_H
