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


#ifndef COMMON_FILER_H
#define COMMON_FILER_H

#include "common.hpp"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define FILER_PAGE 0x2000

class FilerLoad {
public:
    FilerLoad(FILE* fh, bool *valid_ptr);
    ~FilerLoad();

    bool is_valid() const ;
    size_t    tell() const ;

    inline UCHAR get() {
        rarely_if(m_count <= m_cur) load_page();
        return m_valid ? m_buff[m_cur++] : 0;
    }

    inline UINT64 get4() { return getN(4);}
    inline UINT64 get8() { return getN(8);}

private:
    inline UINT64 getN(int N) {
        UINT64 val = 0;
        for (int i = N-1; i>=0; i--)
            val |= (UINT64(get()) << (8*i));
        return val;
    }
    void load_page();

    bool   m_valid;
    UCHAR  m_buff[ FILER_PAGE+10 ]; 
    size_t m_cur, m_count;
    FILE  *m_in;
    bool  *m_valid_ptr;
    UINT64 m_page_count; 
};

class FilerSave {
public:
    FilerSave(FILE* fh);
    ~FilerSave();
    bool is_valid() const ;
    inline bool put(UCHAR c) {
        rarely_if(m_cur >= FILER_PAGE)
            save_page();
        m_buff[m_cur++] = c;
        return m_valid;
    }
    // inline bool putS(UCHAR* str, size_t len) {
    // 
    //     while (len) {
    //         size_t wlen = len < FILER_PAGE-m_cur ? len : FILER_PAGE-m_cur;
    //         memcpy(m_buff+m_cur, str, wlen);
    //         len -= wlen ;
    //         likely_if (len <= 0)
    //             return m_valid;
    //         save_page();
    //     }
    //     assert(0);
    //     return m_valid;
    // }

    // inline bool put4(UINT64 val) { return putN(4, val);}
    // inline bool put8(UINT64 val) { return putN(8, val);}

private:
    // inline bool putN(int N, UINT64 val) {
    //     for (int i = N-1; i>=0 ; i--)
    //         put( (val >> (8*i)) & 0xff );
    //     return m_valid;
    // };
    void save_page();

    bool   m_valid;
    UCHAR  m_buff[ FILER_PAGE+10 ]; 
    size_t m_cur;
    FILE  *m_out;
    UINT64 m_page_count; 
};


#endif  // COMMON_FILER_H
