

// Based on James Bonfield's fqz_comp

#ifndef ZP_LOG64_RANGER_H
#define ZP_LOG64_RANGER_H

#ifndef ZP_RANGECODER_H
#include "coder.hpp"
#endif

class Log64Ranger {
    enum {
        STEP=3,
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
            total += (freq[i] -= (freq[i] >> 1));

        /* Faster than F[i].Freq for 0 <= i < NSYM */
        // for (SymFreqs* s = F; s < F+iend; s++)
        //     total +=
        //         (s->freq -= (s->freq>>1));
    }

    UCHAR bubbly(int a) {
        UCHAR c = syms[a  ];
        syms[a] = syms[a-1];
                  syms[a-1] = c;

        UINT16 f = freq[a];
        freq[a]  = freq[a-1];
                   freq[a-1] = f;

       return c;
    }

public:

    inline void put(RCoder *rc, UCHAR sym) {
        UINT32 sumf  = 0;
        UINT32 i = 0;

        likely_if( sym < iend)
           for (; syms[i] != sym; sumf += freq[i++]);
        else {
            assert(sym < NSYM);
            total += sym - iend + 1;
            sumf = total -1;
            i = sym;
            for (; iend <= sym; iend ++) {
                freq[iend] = 1;
                syms[iend] = iend;
            }
        }

        UINT32 vtot = total + NSYM - iend;
        rc->Encode(sumf, freq[i], vtot);

        rarely_if(freq[i] > (MAX_FREQ - STEP))
            normalize();

        freq[i] += STEP;
        total   += STEP;

        // if (vtot + STEP > MAX_FREQ)
        //     normalize();

        rarely_if
            (
             i and
             0 == (++count&15) and
             freq[i] > freq[i-1])
            bubbly(i); 
    }

    inline UINT16 get(RCoder *rc) {

        UINT32 vtot = total + NSYM - iend;
        UINT32 sumf = 0;
        UINT32 i;

        UINT32 prob = rc->GetFreq(vtot);
        for ( i = 0;
              i < NSYM;
              i ++ ) {

            rarely_if(i >= iend) {
                freq[iend] = 1;
                syms[iend] = i;
                iend ++ ; total ++;
            }

            if (sumf +  freq[i] <= prob)
                sumf += freq[i];
            else
                break;
        }

        rc->Decode(sumf, freq[i], vtot);

        rarely_if(freq[i] > (MAX_FREQ - STEP))
            normalize();

        freq[i] += STEP;
        total   += STEP;

        // if (vtot + STEP > MAX_FREQ)
        //     normalize();

        /* Keep approx sorted */
        return
            ( i and
              0 ==  (++count&15) and
              freq[i] > freq[i-1]) ? 
            bubbly(i) :
            syms[i];
    }

} PACKED;

#endif // already loaded
