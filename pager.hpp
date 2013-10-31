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



#ifndef FQ_PAGER_H
#define FQ_PAGER_H

#include "common.hpp"
#include <stdio.h>
#include <assert.h>

#define PAGER_PAGE 0x200

class PagerLoad {
public:
    PagerLoad(FILE* fh, bool *valid_ptr);
    ~PagerLoad();

    bool is_valid();
    UINT64 get();
    UINT64 tell() const ;

protected:
    inline UINT64 getN(const int N) {
        if (m_cur_indx == 0) {
            m_cur_long  = get();
            m_cur_indx  = 64;
        }
        UINT64 word = (m_cur_long >> (64 - m_cur_indx)) & ~(-1ULL << N);
        m_cur_indx -= N;
        return word;
    }
    inline UINT64 predictN(const int N) {
        if (m_cur_indx == 0) {
            m_cur_long  = get();
            m_cur_indx  = 64;
        }
        return (m_cur_long >> (64 - m_cur_indx)) & ~(-1ULL << N);
    }

private:
    UINT64 m_cur_long;
    int    m_cur_indx;

    void load_page();

    bool   m_valid;
    UINT64 m_buff[PAGER_PAGE+10];  // NOTE: No endian correction, for now
    size_t m_cur, m_count;
    FILE  *m_in;
    bool  *m_valid_ptr;
    int    m_page_count;        // debugging
};

class PagerLoad02 : private PagerLoad {
public:
    PagerLoad02(FILE* fh, bool *valid_ptr) : PagerLoad(fh, valid_ptr) {}
    ~PagerLoad02() {}
    inline UINT64 get02() { return getN(2); }
};

class PagerLoad16 : private PagerLoad {
public:
    PagerLoad16(FILE* fh, bool *valid_ptr) : PagerLoad(fh, valid_ptr) {}
    ~PagerLoad16() {}
    inline UINT64 get16  () { return getN(16); }
    inline UINT16 predict() { return (UINT16) predictN( 16); }
    inline UINT64 getgap() {
        UINT64 gap = get16();
        rarely_if (gap == 0xffff) {
            gap  = get16();
            return
                (gap == 0xffff)  ?
                (get16() << 48) |
                (get16() << 32) |
                (get16() << 16) |
                (get16()      )
                :
                (gap << 16) |
                (get16()  );
        }
        return gap;
    }
};

class PagerSave {

public:
    PagerSave(FILE* fh);
    ~PagerSave();

    bool is_valid();
    bool put(UINT64 n);

protected:
    inline bool putN(UINT64 n, const int N) { // kind of template
        UINT64 word = n;
        assert(word < (1ULL<<N));
        m_cur_long |= (word<<m_cur_indx);
        m_cur_indx += N;
        if (m_cur_indx >= 64) {
            if (not put(m_cur_long))
                return false;
            m_cur_long = m_cur_indx = 0;
        }
        return true;
    }

private:
    void save_page();

    UINT64 m_buff[PAGER_PAGE+10];
    size_t m_cur;
    FILE  *m_out;
    int    m_page_count;        // debugging

    UINT64 m_cur_long;
    int    m_cur_indx;
    bool   m_valid;
};  // PagerSave

class PagerSave02 : private PagerSave {
public:
    PagerSave02(FILE* fh) : PagerSave(fh) {}
    ~PagerSave02() {}
    inline bool put02 (UCHAR  n) { return putN(n, 2); }
};

class PagerSave16 : private PagerSave {
public:
    PagerSave16(FILE* fh) : PagerSave(fh) {}
    ~PagerSave16() {}
    inline bool put16 (UINT16  n) { return putN(n, 16); }
    inline bool putgap( UINT64 gap) {
        likely_if (gap < 0xfff0) {
            put16(gap);
            return false;
        }
        int nwords;
        if(gap > 0xffffffff)  {
            nwords = 4;
            put16(0xffff);
            put16(0xffff);
        }
        else {
            nwords = 2;
            put16(0xffff);
        }
        for (int i = nwords-1; i >= 0; i--)
            put16(0xffff & (gap >> (i*16)));
        return true;
    }
};

#endif  // FQ_PAGER_H
