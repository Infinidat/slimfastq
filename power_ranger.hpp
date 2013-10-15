


// Based on James Bonfield's excellent fqz_comp

#ifndef ZP_POWER_RANGER_H
#define ZP_POWER_RANGER_H

#include "range_coder.hpp"

#ifdef __SSE__
// This prefetch saves 8 seconds (one 5.4G fastq), but seems to little use slightly
// more cpu (or is it just doing the same work quota at lesser time?)
#   include <xmmintrin.h>
#   define PREFETCH(X) _mm_prefetch((const char *)(X), _MM_HINT_T0)
#else
#   define PREFETCH(X)
#endif


#define MAX_FREQ (1<<16)-32

template <int NBITS>
class PowerRanger {
    enum {
        STEP=8,
        NSYM=(1<<NBITS),
    };

    UINT32 total;     // Total frequency
    UINT16 counter;   // Periodic counter for bubble sort step
    UINT16 iend ;     // keep it 4 bytes alignment

    // Array of Symbols approximately sorted by Freq. 
    struct SymFreqs {
        UINT16 symbol;
        UINT16 freq;
    } PACKED F[NSYM];

    void normalize() {
        total=0;
        for (UINT32 i = 0; i <= iend; i++)
            total += 
                F[i].freq -= F[i].freq >> 1;

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
    // SIMPLE_MODEL() {}

    // void init() { - just bzero me
    //     BZERO(F);
    //     total = 0;
    //     counter = 0;
    //     return ;
    // 
    //     // for ( int i=0; i<NSYM; i++ ) {
    //     //     F[i].symbol = i;
    //     //     F[i].freq   = 1;
    //     // }
    //     // 
    //     // total         = NSYM;
    //     // // sentinel.symbol = 0;
    //     // // sentinel.freq   = MAX_FREQ; // Always first; simplifies sorting.
    //     // counter          = 0;
    // 
    //     // F[NSYM].Freq = 0; // terminates normalize() loop. See below.
    // }

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
        F[i].freq += STEP;
        total += STEP;

        if (vtot + STEP > MAX_FREQ)
            normalize();


        if (((++counter&15)==0) and
            i and
            F[i].freq > F[i-1].freq) {
            SymFreqs t = F[i];
            F[i] = F[i-1];
            F[i-1] = t;
        }

        // SymFreqs *s = F;
        // while (s->symbol != sym)
        //     sumfreq += s++->Freq;
        // 
        // rc->Encode(sumfreq, s->Freq, total);
        // s->Freq += STEP;
        // total += STEP;
        // 
        // if (total > MAX_FREQ)
        //     normalize();
        // 
        // Keep approx sorted
        // if (((++counter&15)==0) && s[0].Freq > s[-1].Freq) {
        //     SymFreqs t = s[0];
        //     s[0] = s[-1];
        //     s[-1] = t;
        // }
    }

    inline UINT16 get(RangeCoder *rc) {
        // SymFreqs* s = F;
        
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

        // while ( sumfreq + F[i].freq <= freq  ) {
        //     rarely_if(i+1 > iend and i+1 < NSYM)
        //         expand_to(i+1);
        // 
        //     sumfreq += F[i++].freq;
        // }

        rc->Decode(sumfreq, F[i].freq, vtot);
        F[i].freq += STEP;
        total += STEP;

        if (vtot + STEP > MAX_FREQ)
            normalize();

        /* Keep approx sorted */
        if (((++counter&15)==0) and
            i and
            F[i].freq > F[i-1].freq) {
            SymFreqs t = F[i];
            F[i] = F[i-1];
            F[--i] = t;
            // return t.symbol;
        }
        return F[i].symbol;

        // for (sumfreq = 0; (sumfreq += s->Freq) <= freq; s++);
        // sumfreq -= s->Freq;
        // 
        // rc->Decode(sumfreq, s->Freq, total);
        // s->Freq += STEP;
        // total += STEP;
        // 
        // if (total > MAX_FREQ)
        //     normalize();
        // 
        // /* Keep approx sorted */
        // if (((++counter&15)==0) && s[0].freq > s[-1].freq) {
        //     SymFreqs t = s[0];
        //     s[0] = s[-1];
        //     s[-1] = t;
        //     return t.symbol;
        // }
        // 
        // return s->symbol;
    }
} PACKED;

#endif // ZP_POWER_RANGER_H
