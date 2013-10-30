/***********************************************************************************************************************/
/* This program was written by Josef Ezra  <jezra@infinidat.com>                                                       */
/* Copyright (c) 2013, infinidat                                                                                       */
/* All rights reserved.                                                                                                */
/*                                                                                                                     */
/* Redistribution and use in source and binary forms, with or without modification, are permitted provided that        */
/* the following conditions are met:                                                                                   */
/*                                                                                                                     */
/* Redistributions of source code must retain the above copyright notice, this list of conditions and the following    */
/* disclaimer.                                                                                                         */
/* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following */
/* disclaimer in the documentation and/or other materials provided with the distribution.                              */
/* Neither the name of the <ORGANIZATION> nor the names of its contributors may be used to endorse or promote products */
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





#include "pager.hpp"
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

PagerLoad::PagerLoad(FILE* fh, bool* valid_ptr) {
    m_in = fh;
    m_valid_ptr = valid_ptr;
    * valid_ptr = true;
    m_valid = true ;
    m_page_count = 0;
    load_page();

    m_cur_long = m_cur_indx = 0;
}

PagerLoad::~PagerLoad() {
    fclose(m_in);
}

bool PagerLoad::is_valid() { return m_valid; }

UINT64 PagerLoad::tell() const {
    return
        ( m_page_count )
        ? 
        ((m_page_count-1) * PAGER_PAGE) +
        ( m_cur * sizeof(m_buff[0])) -
        ( m_cur_indx )
        :
        0;
}

void PagerLoad::load_page() {
    rarely_if(not m_valid)
        return;
    size_t cnt = fread(m_buff, sizeof(m_buff[0]), PAGER_PAGE, m_in);
    rarely_if (cnt == 0) {
        *m_valid_ptr = m_valid = false;
        return;
    }
    m_cur = 0;
    m_count = cnt;
    m_page_count++;
}

UINT64 PagerLoad::get() {
    if (m_count <= m_cur)
        load_page();
    return
        m_valid ? m_buff[m_cur++] : 0;     // caller should check is_valid()
}

PagerSave::PagerSave(FILE* fh) {
    m_out = fh;
    m_valid = true;
    m_cur = 0;
    m_page_count = 0;

    m_cur_long = m_cur_indx = 0;
}

PagerSave::~PagerSave() {
    if (m_cur_indx)
        put(m_cur_long);

    save_page();
    fclose(m_out);
}

bool PagerSave::is_valid() { return m_valid; }

void PagerSave::save_page() {
    if (not m_valid or
        not m_cur)
        return ;

    assert(m_cur <= PAGER_PAGE);
    size_t cnt = fwrite(m_buff, sizeof(m_buff[0]), m_cur, m_out);
    if (cnt != m_cur) {
        fprintf(stderr, "Error saving pager: %s m_cur=%lu cnt=%lu\n", strerror(errno), m_cur, cnt);
        exit(1); // remove and handle error at production
        m_valid = false;
    }
    m_cur = 0;
    m_page_count++;
}

bool PagerSave::put(UINT64 n) {
    if (m_cur >= PAGER_PAGE)
        save_page();
    m_buff[m_cur++] = n;
    return m_valid;
}

