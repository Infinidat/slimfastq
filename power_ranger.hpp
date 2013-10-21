

// Based on James Bonfield's fqz_comp

#ifndef ZP_POWER_RANGER
#define ZP_POWER_RANGER

#ifndef ZP_RANGECODER_H
#include "coder.hpp"
#endif

class PowerRanger {
    enum {
        STEP=3,
        NSYM=256,
        MAX_FREQ=(1<<16)-32,
    };

    struct {
        UINT32 total;
        UINT16 freq[NSYM];
        UINT16 iend ;

        UCHAR count;
        UCHAR syms[NSYM];
    } p[32] PACKED ;

    void normalize(int c) {
        for (UINT32 i = p[c].total = 0; i < p[c].iend; i++)
            p[c].total += (p[c].freq[i] -= (p[c].freq[i] >> 1));

        /* Faster than F[i].Freq for 0 <= i < NSYM */
        // for (SymFreqs* s = F; s < F+iend; s++)
        //     total +=
        //         (s->freq -= (s->freq>>1));
    }

    UCHAR bubbly(int c, int a) {
        UCHAR t =
        p[c].syms[a  ];
        p[c].syms[a  ] = p[c].syms[a-1];
        p[c].syms[a-1] = t;

        UINT16 f =
        p[c].freq[a  ];
        p[c].freq[a  ] = p[c].freq[a-1];
        p[c].freq[a-1] = f;

       return t;
    }

protected:
    void put(RCoder *rc, int c, UCHAR sym) {
        UINT32 sumf  = 0;
        UINT32 i = 0;

        likely_if(sym < p[c].iend)
           for (; p[c].syms[i] != sym; sumf += p[c].freq[i++]);
        else {
            assert(sym < NSYM);
            p[c].total += sym - p[c].iend + 1;
            sumf = p[c].total -1;
            i = sym;
            for (; p[c].iend <= sym; p[c].iend ++) {
                p[c].freq[p[c].iend] = 1;
                p[c].syms[p[c].iend] = p[c].iend;
            }
        }

        UINT32 vtot = p[c].total + NSYM - p[c].iend;
        rc->Encode(sumf, p[c].freq[i], vtot);

        rarely_if(p[c].freq[i] > (MAX_FREQ - STEP))
            normalize(c);

        p[c].freq[i] += STEP;
        p[c].total   += STEP;

        // if (vtot + STEP > MAX_FREQ)
        //     normalize();

        rarely_if
            (
             i and
             0 == (++p[c].count&15) and
             p[c].freq[i] > p[c].freq[i-1])
            bubbly(c, i); 
    }

    UINT16 get(RCoder *rc, int c) {

        UINT32 vtot = p[c].total + NSYM - p[c].iend;
        UINT32 sumf = 0;
        UINT32 i;

        UINT32 prob = rc->GetFreq(vtot);
        for ( i = 0;
              i < NSYM;
              i ++ ) {

            rarely_if(i >= p[c].iend) {
                p[c].freq[p[c].iend] = 1;
                p[c].syms[p[c].iend] = i;
                p[c].iend ++ ;
                p[c].total++;
            }

            if (sumf +  p[c].freq[i] <= prob)
                sumf += p[c].freq[i];
            else
                break;
        }

        rc->Decode(sumf, p[c].freq[i], vtot);

        rarely_if(p[c].freq[i] > (MAX_FREQ - STEP))
            normalize(c);

        p[c].freq[i] += STEP;
        p[c].total   += STEP;

        // if (vtot + STEP > MAX_FREQ)
        //     normalize();

        /* Keep approx sorted */
        return
            ( i and
              0 ==  (++p[c].count&15) and
              p[c].freq[i] > p[c].freq[i-1]) ? 
            bubbly(c, i) :
            p[c].syms[i];
    }

public:

    bool put_i(RCoder* rc, long long num, UINT64 * old=NULL) {
        if (old) {
            // assert(num >= *old); should we assert at in-efficiency?
            long long t = num - *old;
            *old = num;
            num = t;
        }
        likely_if( num >= -0x80+3 and
                   num <= 0x7f) {
            put(rc, 0, num & 0xff);
            return false;
        }
        likely_if (num >= -0x8000 and
                   num <=  0x7fff) {
            put(rc, 0, 0x80);
            put(rc, 1, 0xff & (num));
            put(rc, 2, 0xff & (num>>8));
            return false;
        }
        if (num >= -0x80000000LL and
            num <=  0x7fffffffLL ) {
            put(rc, 0, 0x81);
            for (int shift = 0, i=3; shift < 32; shift+=8, i++)
                put(rc, i, 0xff&(num>>shift));
            return true;
        }
        {
            put(rc, 0, 0x82);
            for (int shift = 0, i=7; shift < 64; shift+=8, i++)
                put(rc, i, 0xff&(num>>shift));

            return true;
        }
    }

    long long get_i(RCoder* rc, UINT64* old=NULL) {
        long long num = get(rc, 0);
        rarely_if(num == 0x80 ) {
            num  = get(rc, 1);
            num |= get(rc, 2) << 8;
            num  = (short) num;
        }
        else rarely_if(num == 0x81) {
                num = 0;
                for (int shift = 0, i=3; shift < 32; shift+=8, i++)
                    num |= get(rc, i) << shift;
                num  = (int) num;
            }
        else rarely_if(num == 0x82) {
                num = 0;
                for (int shift = 0, i=7; shift < 64; shift+=8, i++)
                    num |= get(rc, i) << shift;
            }
        else if (0x80&num)
            num = (char)num;

        return
            ( old) ?
            (*old += num) :
            num ;
    }

    bool put_u(RCoder *rc, UINT64 num, UINT64* old=NULL) {
    if (old) {
        // assert(num >= *old); what's worse?
        UINT64 t = num - *old;
        *old = num;
        num = t;
    }
    likely_if(num <= 0x7f) {
        put(rc, 16, 0xff & num);
        return false;
    }
    likely_if (num < 0x7ffe) {
        put(rc, 16, 0xff & (0x80 | (num>>8)));
        put(rc, 17, 0xff & num);
        return false;
    }
    put(rc, 16, 0xff);
    if (num < 1ULL<<32) {
        put(rc, 17, 0xfe);
        for (int shift=0, i=18; shift < 32; shift+=8, i++)
            put(rc, i, 0xff & (num>>shift));
        return true;
    }
    {
        put(rc, 17, 0xff);
        for (int shift=0, i=22; shift < 64; shift+=8, i++)
            put(rc, i, 0xff & (num>>shift));
        return true;
    }
}

UINT64 get_u(RCoder *rc, UINT64* old=NULL) {

    UINT64 num = get(rc, 16);
    rarely_if(num > 0x7f) {

        num <<= 8;
        num  |= get(rc, 17);
        likely_if(num < 0xfffe)
            num &= 0x7fff;
        else likely_if (num == 0xfffe) {
            num = 0;
            for (int shift=0, i = 18; shift < 32; shift+=8, i++) {
                UINT64 c = get(rc, i);
                num  |= (c<<shift);
            }
        }
        else {
            num = 0;
            for (int shift=0, i=22; shift < 64; shift+=8, i++) {
                UINT64 c = get(rc, i);
                num  |= (c<<shift);
            }
        }
    }
    return
        ( old) ?
        (*old += num) :
        num ;
}

} PACKED;

#endif // already loaded
