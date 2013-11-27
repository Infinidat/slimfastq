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

#include <string.h>

class PowerRanger {
    enum {
        STEP=14,
        NSYM=256,
        MAX_FREQ=(1<<15)-32,
    };

    UINT32 total;
    UINT16 freq[NSYM];
    UINT16 iend ;

    UCHAR count;
    UCHAR syms[NSYM];

    void normalize() {
        for (UINT32 i = total = 0; i < iend; i++)
            total += (freq[i] >>= 1 ); 
    }

    inline UCHAR sort_of_sort(int i) {

        UCHAR t = syms[i  ];
        syms[i] = syms[i-1];
        syms[i-1] = t;

        UINT16 f = freq[i];
        freq[i]  = freq[i-1];
        freq[i-1] = f;

        return t;
    }

    inline UCHAR update_freq(int i) {

        rarely_if(freq[i] > (MAX_FREQ - STEP)) {
            if  (freq[i] + 256U > total)
                return syms[i];

            normalize();
        }

        freq[i] += STEP;
        total   += STEP;

        return LIKELY (i == 0 or
                       (++ count & 0xf) or
                       freq[i] <= freq[i-1] ) ?
            syms[i] :
            sort_of_sort(i);
    }

public:
    PowerRanger() {
        bzero(this, sizeof(*this));
    }

    void put(RCoder *rc, UCHAR sym) {
        UINT32 sumf  = 0;
        UINT32 i = 0;

        rarely_if(iend <= sym)
            for (;iend <= sym; iend++)
                  syms[iend] = iend;

        for (; syms[i] != sym; sumf += freq[i++]);
        sumf += i;

        UINT32 vtot = total + NSYM;
        rc->Encode(sumf, freq[i]+1, vtot);

        update_freq(i);
    }

    UINT16 get(RCoder *rc) {

        UINT32 vtot = total + NSYM; // - iend;
        UINT32 sumf = 0;
        UINT32 i;

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

        return update_freq(i);
    }
} PACKED;

class PowerRangerU {

    PowerRanger p[14];

public:
    bool put_u(RCoder *rc, UINT64 num) {

    likely_if(num <= 0x7f) {
        p[0].put(rc, 0xff & num);
        return false;
    }
    likely_if (num < 0x7ffe) {
        p[0].put(rc, 0xff & (0x80 | (num>>8)));
        p[1].put(rc, 0xff & num);
        return false;
    }
    p[0].put(rc, 0xff);
    if (num < 1ULL<<32) {
        p[1].put(rc, 0xfe);
        for (int shift=0, i=2; shift < 32; shift+=8, i++)
            p[i].put(rc, 0xff & (num>>shift));
        return true;
    }
    {
        p[1].put(rc, 0xff);
        for (int shift=0, i=6; shift < 64; shift+=8, i++)
            p[i].put(rc, 0xff & (num>>shift));
        return true;
    }
}

UINT64 get_u(RCoder *rc) {

    UINT64 num = p[0].get(rc);
    rarely_if(num > 0x7f) {
    
        num <<= 8;
        num  |= p[1].get(rc);
        likely_if(num < 0xfffe)
            num &= 0x7fff;

        else likely_if (num == 0xfffe) {
            num = 0;
            for (int shift=0, i = 2; shift < 32; shift+=8, i++) {
                UINT64 c = p[i].get(rc);
                num  |= (c<<shift);
            }
        }
        else {
            num = 0;
            for (int shift=0, i=6; shift < 64; shift+=8, i++) {
                UINT64 c = p[i].get(rc);
                num  |= (c<<shift);
            }
        }
    }
    return num;
}

} PACKED;

class PowerRangerI {

    PowerRanger p[15];

public:
    bool put_i(RCoder* rc, long long num) {

        likely_if( num >= -0x80+3 and
                   num <= 0x7f) {
            p[0].put(rc, num & 0xff);
            return false;
        }
        likely_if (num >= -0x8000 and
                   num <=  0x7fff) {
            p[0].put(rc, 0x80);
            p[1].put(rc, 0xff & (num));
            p[2].put(rc, 0xff & (num>>8));
            return false;
        }
        if (num >= -0x80000000LL and
            num <=  0x7fffffffLL ) {
            p[0].put(rc, 0x81);
            for (int shift = 0, i=3; shift < 32; shift+=8, i++)
                p[i].put(rc, 0xff&(num>>shift));
            return true;
        }
        {
            p[0].put(rc, 0x82);
            for (int shift = 0, i=7; shift < 64; shift+=8, i++)
                p[i].put(rc, 0xff&(num>>shift));
        
            return true;
        }
    }

    long long get_i(RCoder* rc) {
        long long num = p[0].get(rc);

        rarely_if(num == 0x80 ) {
            num  = p[1].get(rc);
            num |= p[2].get(rc) << 8;
            num  = (short) num;
        }
        else rarely_if(num == 0x81) {
                num = 0;
                for (int shift = 0, i=3; shift < 32; shift+=8, i++) {
                    int c = p[i].get(rc);
                    num |= c << shift;
                }
                num  = (int) num;
            }
        else rarely_if(num == 0x82) {
                num = 0;
                for (int shift = 0, i=7; shift < 64; shift+=8, i++) {
                    UINT64 c = p[i].get(rc);
                    num |=  c << shift;
                }
            }
        else if (0x80&num)
            num = (char)num;

        return num ;
    }

} PACKED ;

#endif 
