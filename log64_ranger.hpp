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

#ifndef ZP_LOG64_RANGER_H
#define ZP_LOG64_RANGER_H

#ifndef ZP_RANGECODER_H
#include "coder.hpp"
#endif

#define LAST_QLT 63

class Log64Ranger {
    enum {
        STEP=6,
        // NSYM=63,                // decrease from 64 to get better memory alignment
        NSYM=64,
        MAX_FREQ=(1<<16)-64,
    };


    UINT16 freq[NSYM];
    UINT16 iend ;
    UINT32 total;
    UCHAR count;
    UCHAR syms[NSYM];

    void normalize() {
        for (UINT32 i = total = 0; i < iend; i++)
            total += (freq[i] /= 2);
    }

    inline UCHAR down_level(int i) {

        UCHAR c = syms[i  ];
        syms[i] = syms[i-1];
        syms[i-1] = c;

        UINT16 f = freq[i];
        freq[i]  = freq[i-1];
        freq[i-1] = f;

        return c;
    }

    inline UCHAR update_freq(int i) {

        rarely_if(freq[i] > (MAX_FREQ - STEP)) {
            rarely_if((freq[i] == total))
                return syms[i];

            normalize();
        }

        freq[i] += STEP;
        total   += STEP;

        return LIKELY (i == 0 or
                       (++ count & 0xf) or
                       freq[i] <= freq[i-1] ) ?
            syms[i] :
            down_level(i);
    }

public:
    Log64Ranger() {
        // *(UINT64*) iend = 0;
        // iend = 0, total = 0, count = 0;
        bzero(&iend, 8);
    }

    inline void put(RCoder *rc, UCHAR sym) {
        UINT32 sumf  = 0;
        UINT32 i = 0;

        assert(sym < NSYM);
        rarely_if(iend <= sym)
            for (;iend <= sym; iend++)
                syms[iend] = iend;

        for (; syms[i] != sym; sumf += freq[i++]);
        sumf += i;

        UINT32 vtot = total + NSYM;
        rc->Encode(sumf, freq[i]+1, vtot);

        update_freq(i);
    }

    inline UINT16 get(RCoder *rc) {

        UINT32 vtot = total + NSYM;
        UINT32 sumf = 0;
        UINT32 i = 0;

        UINT32 prob = rc->GetFreq(vtot);

        for ( i = 0;
              i < NSYM;
              i ++ ) {

            rarely_if(iend    == i) 
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

#endif // already loaded
