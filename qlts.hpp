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


#ifndef FQ_QLTS_H
#define FQ_QLTS_H

#include "common.hpp"
#include "config.hpp"
#include "filer.hpp"
#include "log64_ranger.hpp"
#include "power_ranger.hpp"

#define RANGER_SIZE_2 (1<<16)
#define RANGER_MASK_2 (RANGER_SIZE_2-1)

#define RANGER_SIZE_1 (1<<12)
#define RANGER_MASK_1 (RANGER_SIZE_1-1)

class QltBase {
protected:

    Log64Ranger* ranger;
    PowerRanger  exranger;
    RCoder rcoder;
    bool m_valid;

    size_t ranger_cnt();
    // void   range_init();

    inline static UINT32 calc_last_1 (UINT32 last, UCHAR b) {
        return ( b | (last << 6) ) & RANGER_MASK_1;
    }
    inline static UINT32 calc_last_2 (UINT32 last, UCHAR b) {
        return ( b | (last << 6) ) & RANGER_MASK_2;
    }
    // inline static UINT32 calc_last_3 (UINT32 last, UCHAR b) {
    //     return ( b | (last << 6) ) & RANGER_MASK_3;
    //     // TODO: use delta
    // }
    inline static UINT32 calc_last_delta(UINT32 &delta, UCHAR q, UCHAR q1, UCHAR q2) {

        // This brilliant code could only be of James Bonfield
        if (q1>q)
            delta += (q1-q);

        return
            ( q
              | ((q1 < q2        ? q2         : q1) <<  6)
              | ((q1 == q2                        ) << 12)
              | ((7 > (delta>>3) ? (delta>>3) : 7 ) << 13)
              ) & RANGER_MASK_2 ;
    }
};

class QltSave : private QltBase {
public:
    QltSave();
    ~QltSave();

    void save_1(const UCHAR* buf, size_t size);
    void save_2(const UCHAR* buf, size_t size);
    void save_3(const UCHAR* buf, size_t size);
    // void filer_init();
    bool is_valid();
private:
    FilerSave* filer;
};

class QltLoad : private QltBase {
public:
    QltLoad();
    ~QltLoad();

    bool is_valid();
    UINT32 load_1 (UCHAR* buffer, const size_t size);
    UINT32 load_2 (UCHAR* buffer, const size_t size);
    UINT32 load_3 (UCHAR* buffer, const size_t size);
private:
    FilerLoad* filer;
};

#endif  // FQ_QLTS_H
