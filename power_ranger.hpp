/***********************************************************************************************************************/
/* This program was written by Josef Ezra  <jezra@infinidat.com>                                                       */
/* Copyright (c) 2013, Infinidat                                                                                       */
/* All rights reserved.                                                                                                */
/*                                                                                                                     */
/* Redistribution and use in source and binary forms, with or without modification, are permitted provided that        */
/* the following conditions are met:                                                                                   */
/*                                                                                                                     */
/* Redistributions of source code must retain the above copyright notice, this list of conditions and the following    */
/* disclaimer.                                                                                                         */
/* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following */
/* disclaimer in the documentation and/or other materials provided with the distribution.                              */
/* Neither the name of the Infinidat nor the names of its contributors may be used to endorse or promote products      */
/* derived from this software without specific prior written permission.                                               */
/*                                                                                                                     */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,  */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE   */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,  */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR     */
/* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,   */
/* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE    */
/* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                            */
/***********************************************************************************************************************/


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
    } p[16] PACKED ;

    void normalize(int c) {
        for (UINT32 i = p[c].total = 0; i < p[c].iend; i++)
            p[c].total += (p[c].freq[i] /=2 ); 
    }

    inline UCHAR sort_of_sort(int c, int i) {
        likely_if (i == 0 or
                   (++ p[c].count & 0xf) or
                   p[c].freq[i] <= p[c].freq[i-1] )
            return p[c].syms[i];

        UCHAR t = p[c].syms[i  ];
        p[c].syms[i] = p[c].syms[i-1];
        p[c].syms[i-1] = t;

        UINT16 f = p[c].freq[i];
        p[c].freq[i]  = p[c].freq[i-1];
        p[c].freq[i-1] = f;

        return t;
    }

protected:
    void put(RCoder *rc, int c, UCHAR sym) {
        UINT32 sumf  = 0;
        UINT32 i = 0;

        rarely_if(p[c].iend <= sym)
            for (;p[c].iend <= sym;      p[c].iend++)
                  p[c].syms[p[c].iend] = p[c].iend;

        for (; p[c].syms[i] != sym; sumf += p[c].freq[i++]);
        sumf += i;

        UINT32 vtot = p[c].total + NSYM;
        rc->Encode(sumf, p[c].freq[i]+1, vtot);

        rarely_if(p[c].freq[i] > (MAX_FREQ - STEP))
            normalize(c);

        p[c].freq[i] += STEP;
        p[c].total   += STEP;

        sort_of_sort(c, i);
    }

    UINT16 get(RCoder *rc, int c) {

        UINT32 vtot = p[c].total + NSYM; // - p[c].iend;
        UINT32 sumf = 0;
        UINT32 i;

        UINT32 prob = rc->GetFreq(vtot);
        for ( i = 0;
              i < NSYM;
              i ++ ) {

            rarely_if(i >= p[c].iend) 
                p[c].syms[ p[c].iend++ ] = i;
        
            if (sumf +  p[c].freq[i] + 1 <= prob)
                sumf += p[c].freq[i] + 1;
            else
                break;
        }

        rc->Decode(sumf, p[c].freq[i]+1, vtot);

        rarely_if(p[c].freq[i] > (MAX_FREQ - STEP))
            normalize(c);

        p[c].freq[i] += STEP;
        p[c].total   += STEP;

        return sort_of_sort(c, i);
    }

public:
    void put_c(RCoder* rc, UCHAR chr) {
        put(rc, 8, chr);
    }

    UCHAR get_c(RCoder* rc) {
        return get(rc, 8);
    }

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
                for (int shift = 0, i=3; shift < 32; shift+=8, i++) {
                    int c = get(rc, i);
                    num |= c << shift;
                }
                num  = (int) num;
            }
        else rarely_if(num == 0x82) {
                num = 0;
                for (int shift = 0, i=7; shift < 64; shift+=8, i++) {
                    UINT64 c = get(rc, i);
                    num |=  c << shift;
                }
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
        put(rc, 0, 0xff & num);
        return false;
    }
    likely_if (num < 0x7ffe) {
        put(rc, 0, 0xff & (0x80 | (num>>8)));
        put(rc, 1, 0xff & num);
        return false;
    }
    put(rc, 0, 0xff);
    if (num < 1ULL<<32) {
        put(rc, 1, 0xfe);
        for (int shift=0, i=2; shift < 32; shift+=8, i++)
            put(rc, i, 0xff & (num>>shift));
        return true;
    }
    {
        put(rc, 1, 0xff);
        for (int shift=0, i=8; shift < 64; shift+=8, i++)
            put(rc, i, 0xff & (num>>shift));
        return true;
    }
}

UINT64 get_u(RCoder *rc, UINT64* old=NULL) {

    UINT64 num = get(rc, 0);
    rarely_if(num > 0x7f) {
    
        num <<= 8;
        num  |= get(rc, 1);
        likely_if(num < 0xfffe)
            num &= 0x7fff;

        else likely_if (num == 0xfffe) {
            num = 0;
            for (int shift=0, i = 2; shift < 32; shift+=8, i++) {
                UINT64 c = get(rc, i);
                num  |= (c<<shift);
            }
        }
        else {
            num = 0;
            for (int shift=0, i=8; shift < 64; shift+=8, i++) {
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
