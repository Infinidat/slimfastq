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

#ifndef ZP_BASES_RANGER_H
#define ZP_BASES_RANGER_H

#ifndef ZP_RANGECODER_H
#include "coder.hpp"
#endif
#include "common.hpp"

class Base2Ranger {

    enum { // STEP = 1,
           MAX_FREQ=254,
           INIT_VAL=3,
           M_ONES = 0x01010101,
    };
    
    union {
        UCHAR  freq[4];
        UINT32 freq_val;
    };

    void normalize() {
        freq_val =
            ((freq_val & ~M_ONES) >> 1 ) |
             (freq_val &  M_ONES);
        // (freq_val & ~M_ONES) >> 1 ;
    }

    inline UINT16 getsum() {
        // return 4 + (freq[0] + freq[1]) + (freq[2] + freq[3]);
        return (freq[0] + freq[1]) + (freq[2] + freq[3]);
    }

    inline void update_freq(int sym , UINT16 total) {
        rarely_if(freq[sym] > (MAX_FREQ))
            normalize();

        // freq[sym] += STEP;
        freq[sym]++;
    }

public:
    Base2Ranger() {
        // BZERO made it slower
        freq_val = INIT_VAL * M_ONES;
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

        update_freq(sym, total);
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

        update_freq(i, total);
        return i;
    }
} PACKED ;

#endif  // ZP_BASES_RANGER_H
