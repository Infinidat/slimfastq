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


#include "qlts.hpp"
#include <assert.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <algorithm>

size_t QltBase::ranger_cnt() {
    return
        conf.level == 1 ? RANGER_SIZE_1 :
        // conf.level == 3 ? RANGER_SIZE_3 :
                          RANGER_SIZE_2 ;
}

// void QltBase::range_init() {
//     // bzero(ranger, sizeof(ranger[0]) * ranger_cnt());
// }

QltSave::QltSave()  {
    filer  = new FilerSave("qlt");
    ranger = new Log64Ranger[ranger_cnt()];
    assert(filer);
    assert(ranger);
    rcoder.init(filer);
    BZERO(m_last);
    // range_init();
}

QltSave::~QltSave() {
    rcoder.done();
    if (not conf.quiet and filer)
        fprintf(stderr, "::: QLT comp size: %lu \t| high qlt cnt: %u\n", filer->tell(), m_last.extra_hi_qlt);

    DELETE(ranger);
    DELETE(filer);
}

bool QltSave::is_valid() {
    return
        m_valid and
        filer and
        filer->is_valid();
}

void QltSave::save_1(const UCHAR* buf, size_t size) {
    UINT32 last = 0;
    for (const UCHAR* p = buf ; p < buf + size; p++) {
        UCHAR b = UCHAR(*p-'!');

        PREFETCH(ranger + last);
        likely_if(b < LAST_QLT)
            ranger[last].put(&rcoder, b);
        else {
            ranger[last].put(&rcoder, LAST_QLT);
            exranger.put(&rcoder, b);
            m_last.extra_hi_qlt ++ ;
        }
        last = calc_last_1(last, b); 
    }
}

void QltSave::save_2(const UCHAR* buf, size_t size) {
    UINT32 last = 0;
    for (const UCHAR* p = buf ; p < buf + size; p++) {
        UCHAR b = UCHAR(*p-'!');

        PREFETCH(ranger + last);
        likely_if(b < LAST_QLT)
            ranger[last].put(&rcoder, b);
        else {
            ranger[last].put(&rcoder, LAST_QLT);
            exranger.put(&rcoder, b);
            m_last.extra_hi_qlt ++ ;
        }
        last = calc_last_2(last, b); 
    }
}

void QltSave::save_3(const UCHAR* buf, size_t size) {
    UINT32 last = 0;
    UINT32 delta = 5;
    UCHAR q1 = 0, q2 = 0;
    UINT32 di = 0;

    for (const UCHAR* p = buf ; p < buf + size; p++) {
        UCHAR b = UCHAR(*p-'!');

        PREFETCH(ranger + last);
        likely_if(b < LAST_QLT)
            ranger[last].put(&rcoder, b);
        else {
            ranger[last].put(&rcoder, LAST_QLT);
            exranger.put(&rcoder, b);
            m_last.extra_hi_qlt ++ ;
        }

        if (++ di & 1) {
            last = calc_last_delta(delta, b, q1, q2);
            q2 = b;
        }
        else {
            last = calc_last_delta(delta, b, q2, q1);
            q1 = b;
        }
    }
}

//////////
// load //
//////////

QltLoad::QltLoad() {
    filer = new FilerLoad("qlt", &m_valid);
    ranger = new Log64Ranger[ranger_cnt()];

    rcoder.init(filer);
    // range_init();
}

QltLoad::~QltLoad() {
    rcoder.done();
    DELETE(filer);
    DELETE(ranger);
}

bool QltLoad::is_valid() {
    return
        m_valid and
        filer and
        filer->is_valid();
}

UINT32 QltLoad::load_1(UCHAR* buf, const size_t size) {

    UINT32 last = 0 ;

    for (UCHAR* p = buf; p < buf + size ; p++) {
        UCHAR b = ranger[last].get(&rcoder);

        likely_if(b < LAST_QLT)
            *p = UCHAR('!' + b);
        else {
            b = exranger.get(&rcoder);
            *p = UCHAR('!' + b);
        }
        last = calc_last_1(last, b);
    }
    return m_valid ? size : 0;
}

UINT32 QltLoad::load_2(UCHAR* buf, const size_t size) {

    UINT32 last = 0 ;
    for (UCHAR* p = buf; p < buf + size ; p++) {
        PREFETCH(ranger + last);
        UCHAR b = ranger[last].get(&rcoder);

        likely_if(b < LAST_QLT)
            *p = UCHAR('!' + b);
        else {
            b = exranger.get(&rcoder);
            *p = UCHAR('!' + b);
        }
        last = calc_last_2(last, b);
    }
    return m_valid ? size : 0;
}

UINT32 QltLoad::load_3(UCHAR* buf, const size_t size) {

    UINT32 last = 0 ; 
    UINT32 delta = 5;
    UCHAR  q1 = 0, q2 = 0;
    UINT32 di = 0;
    
    for (UCHAR* p = buf; p < buf + size ; p++) {
        PREFETCH(ranger + last);
        UCHAR b = ranger[last].get(&rcoder);

        likely_if(b < LAST_QLT)
            *p = UCHAR('!' + b);
        else {
            b = exranger.get(&rcoder);
            *p = UCHAR('!' + b);
        }
        *p = UCHAR('!' + b);

        if (++di & 1) {
            last = calc_last_delta(delta, b, q1, q2);
            q2 = b;
        }
        else {
            last = calc_last_delta(delta, b, q2, q1);
            q1 = b;
        }
    }
    return m_valid ? size : 0;
}

