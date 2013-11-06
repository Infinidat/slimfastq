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



#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "filer.hpp"

FilerLoad::FilerLoad(FILE* fh, bool* valid_ptr) {
    m_in = fh;
    m_valid_ptr = valid_ptr;
    * valid_ptr = m_valid = true ;
    m_page_count = 0;
    m_cur = m_count = 0;                // happy valgrind 
    load_page();
}

FilerLoad::~FilerLoad() {
    fclose(m_in);
    m_in = NULL;
    *m_valid_ptr = false;
}

bool FilerLoad::is_valid() const { return m_valid ; }

size_t FilerLoad::tell() const {
    return
        ( m_page_count  ) ?
        ((m_page_count-1) * FILER_PAGE) +
        ( m_cur ) :
        0;
}

void FilerLoad::load_page() {
    rarely_if(not m_valid)
        return;
    size_t cnt = fread(m_buff, sizeof(m_buff[0]), FILER_PAGE, m_in);
    rarely_if (cnt == 0) {
        *m_valid_ptr = m_valid = false;
        return;
    }
    m_cur = 0;
    m_count = cnt;
    m_page_count++;
}

// UINT64 FilerLoad::getN(int N) {
//     UINT64 val = 0;
//     for (int i = N-1; i>=0; i--)
//         val |= (UINT64(get()) << (8*i));
//     return val;
// }

FilerSave::FilerSave(FILE* fh) {
    m_out = fh;
    m_valid = true;
    m_cur = 0;
    m_page_count = 0;
}

FilerSave::~FilerSave() {
    save_page();
    fclose(m_out);
    m_out = NULL;
    m_valid = false;
}

bool FilerSave::is_valid() const { return m_valid; }

void FilerSave::save_page() {
    if (not m_valid or
        not m_cur)
        return ;

    assert(m_cur <= FILER_PAGE);
    size_t cnt = fwrite(m_buff, sizeof(m_buff[0]), m_cur, m_out);
    if (cnt != m_cur) {
        fprintf(stderr, "Error saving pager: %s m_cur=%lu cnt=%lu\n", strerror(errno), m_cur, cnt);
        exit(1); // remove and handle error at production
        m_valid = false;
    }
    m_cur = 0;
    m_page_count++;
}


// bool FilerSave::putN(int N, UINT64 val) {
//     for (int i = N-1; i>=0 ; i--)
//         put( (val >> (8*i)) & 0xff );
//     return m_valid;
// }

// use it later
// UINT64 FilerLoad::getUL() {
//     UINT64 val = get();
//     likely_if (val < 128)
//         return val;
//     val <<= 8;
//     val  |= get();
//     likely_if (val < 0xfffe)
//         return (val & 0x7fff);
// 
//     int nb = (val == 0xfffe) ? 4 : 8;
//     val = 0;
//     for (int i = nb ; i ; i--)
//         val |= (get() << (i-1));
//     return val;
// }


