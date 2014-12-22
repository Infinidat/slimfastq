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

class FilerBase {
protected:
    enum {
        maxi_nodes = FILER_PAGE / sizeof(UINT32) - 1,
        size_nodes,
    };

    UINT64 m_page_count; 
    bool   m_valid;
    UCHAR  m_buff[ FILER_PAGE+10 ]; 
    size_t m_cur, m_count;
    bool  *m_valid_ptr;
    UINT32 m_node[ size_nodes ];
    UINT32 m_node_i;
    UINT32 m_node_p;
    UINT32 m_onef_i;

    FilerBase();
    size_t tell() const ;
};

class FilerSave : private FilerBase {
public:
    static void init(FILE* in);
    static void finit();
    static UINT64 finit_size();

    FilerSave(const char* name);
    FilerSave(int forty_two);
    ~FilerSave();

    bool is_valid() const ;
    size_t tell() const ;

    inline bool put(UCHAR c) {
        rarely_if(m_cur >= FILER_PAGE)
            save_page();
        m_buff[m_cur++] = c;
        return m_valid;
    }

private:
    void save_node(UINT32 next_node);
    void save_page(bool finit=false);
    // UINT32 findex;
};

class FilerLoad : private FilerBase {
public:
    static void init(FILE* in);
    static void confess();
    FilerLoad(const char* name, bool *valid_ptr);
    FilerLoad(int forty_two, bool* valid_ptr);
    ~FilerLoad();

    bool is_valid() const ;
    size_t tell() const ;

    inline UCHAR get() {
        rarely_if(m_count <= m_cur) load_page();
        return m_valid ? m_buff[m_cur++] : 0;
    }

private:
    void load_page();
};

#endif  // COMMON_FILER_H
