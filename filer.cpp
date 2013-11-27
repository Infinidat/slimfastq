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

static void croak(const char* fmt, long long num) __attribute__ ((noreturn, cold));
static void croak(const char* fmt, long long num) {
    fprintf(stderr, fmt, num, errno ? strerror(errno) : "");
    fprintf(stderr, "\n");
    exit(1);
}

struct OneFile {
    struct file_attr_t {

        UINT64 name;
        UINT64 size ; // in bytes
        UINT32 first; // page index
        UINT32 node ; // page index
    } PACKED;
    enum {
        max_root_files = FILER_PAGE / sizeof(file_attr_t),
        max_node_files = FILER_PAGE / 32,
    };
    file_attr_t files[max_root_files+1]; // pad to fit one page
    uint   next_findex;                  // offset in files table
    UINT32 num_pages;                    // free alloc index

    // UINT32 files_index;  - currently limitted to single root dir (341 files).

    FILE* m_out;
    FILE* m_in ;

    uint get_findex() {
        assert(m_out);
        rarely_if(next_findex >= max_root_files)
            croak("Internal error: Too many open files: %d %s", next_findex);
        return next_findex ++;
    }
    uint get_findex(UINT64 name) {
        assert(m_in);
        for(uint i = 1; i < num_pages; i++)
            if (files[i].name == name)
                return i;
        return 0;
    }
    uint allocate() {
        return num_pages ++;
    }
    void read_page(UINT32 offset, UCHAR* page) {
        UINT64 offs = offset* FILER_PAGE;
        fseek(m_in, offs , SEEK_SET);
        UINT32 cnt = fread(page, 1, FILER_PAGE, m_in);
        if (cnt != FILER_PAGE)
            croak("Failed reading page index %d: %s", offset);
    }
    void write_page(UINT32 offset, UCHAR* page) {
        UINT64 offs = offset* FILER_PAGE;
        fseek(m_out, offs, SEEK_SET);
        UINT32 cnt = fwrite(page, 1, FILER_PAGE, m_out);
        if (cnt != FILER_PAGE)
            croak("Failed writing page index %d: %s", offset);
    }
    OneFile() {
        m_out = m_in = NULL;
    }
    void init_read(FILE* in) {
        next_findex = 1;
        num_pages = 2;
        m_in  = in;
        m_out = NULL;
        read_page(1, (UCHAR*)files);
        num_pages = files[0].node;
    }
    void init_write(FILE* out) {
        files[0].name = 0;
        files[0].first = 0;
        next_findex = 1;
        num_pages = 2;
        m_in  = NULL;
        m_out = out;
    }
    void finit_write() {
        if (m_out) {
            files[0].node = num_pages;
            write_page(1, (UCHAR*)files);
            fclose(m_out);
            m_out = NULL;
        }
    }
    ~OneFile() { finit_write() ; } // call explicitly from config?
} onef ;

void FilerSave::init(FILE* out) {
    onef.init_write(out);
}

void FilerSave::finit() {
    onef.finit_write();
}

void FilerLoad::init(FILE* in) {
    onef.init_read(in);
}

// Save

static UINT64 name2u(const char* name) {
    UINT64 uname ;
    assert(strlen(name) <=8 );
    strncpy((char*)&uname, name, 8);
    return uname;
}

FilerSave::FilerSave(const char* name, bool file_zero) {

    uint fi = m_onef_i = onef.get_findex();
    onef.files[fi].name = name2u(name);
    onef.files[fi].size = 0;
    onef.files[fi].node = 0;
    onef.files[fi].first = file_zero ? 0 : onef.allocate();

    m_node_i = 0;
    m_node_p = 0;

    m_valid = true;
    m_cur = 0;
    m_page_count = 0;
}

FilerSave::~FilerSave() {
    save_page();
    m_valid = false;
    if (m_node_p)
        save_node(0);
}

bool FilerSave::is_valid() const { return m_valid; }

void FilerSave::save_node(UINT32 next_node) {
    assert(m_node_i <= maxi_nodes );
    assert(m_node_p);
    m_node[m_node_i ] = next_node;
    onef.write_page(m_node_p, (UCHAR*) m_node);
    m_node_p = next_node;
    m_node_i = 0;
}

void FilerSave::save_page() {
    if (not m_valid or
        not m_cur)
        return ;

    assert(m_cur <= FILER_PAGE);
    // size_t cnt = fwrite(m_buff, sizeof(m_buff[0]), m_cur, m_out);
    // if (cnt != m_cur) {
    //     fprintf(stderr, "Error saving pager: %s m_cur=%lu cnt=%lu\n", strerror(errno), m_cur, cnt);
    //     exit(1); // remove and handle error at production
    //     m_valid = false;
    // }
    rarely_if (not m_node_p) {
        assert(m_node_i == 0);
        onef.write_page(onef.files[m_onef_i].first, m_buff);
        onef.files[m_onef_i].node = m_node_p = (m_cur == FILER_PAGE) ? onef.allocate() : 0;
    }
    else {
        onef.write_page(m_node[ m_node_i ++ ], m_buff);
        rarely_if (m_node_i == maxi_nodes)
            save_node(onef.allocate());
    }
    m_node[ m_node_i ] = onef.allocate(); 
    onef.files[m_onef_i].size += m_cur;
    m_cur = 0;
    m_page_count++;
}

// Load

FilerLoad::FilerLoad(const char* name, bool* valid_ptr, bool file_zero) {

    if (file_zero)
        m_onef_i = 0;
    else {
        m_onef_i = onef.get_findex(name2u(name));
        if (not m_onef_i) {
            *valid_ptr = m_valid = false;
            return;
        }
    }

    m_node_i = 0;
    m_node_p = 0;

    m_valid_ptr = valid_ptr;
    * valid_ptr = m_valid = true ;
    m_page_count = 0;
    m_cur = m_count = 0;                // happy valgrind 

    load_page();
}

FilerLoad::~FilerLoad() {
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

    if (tell() >= onef.files[m_onef_i].size ){
        *m_valid_ptr = m_valid = false;
        return;                 // EOF
    }
 
    // size_t cnt = fread(m_buff, sizeof(m_buff[0]), FILER_PAGE, m_in);
    // rarely_if (cnt == 0) {
    //     *m_valid_ptr = m_valid = false;
    //     return;
    // }
    rarely_if (m_node_p == 0) {
        assert(m_node_i == 0);
        onef.read_page(onef.files[m_onef_i].first, m_buff);
        m_node_p = onef.files[m_onef_i].node;
        if (m_node_p) 
            onef.read_page(m_node_p, (UCHAR*) m_node);
        m_node_i = 0;
    }
    else {
        rarely_if (m_node_i == maxi_nodes) { // load node page
            m_node_p = m_node[ maxi_nodes ];
            onef.read_page(m_node[m_node_i], (UCHAR*) m_node);
            m_node_i = 0;
        }
        onef.read_page(m_node[m_node_i ++], m_buff);
    }

    m_cur = 0;
    UINT64 size = onef.files[m_onef_i].size;
    m_count = size/FILER_PAGE == m_page_count ? size % FILER_PAGE : FILER_PAGE ;
    m_page_count++;
}

// UINT64 FilerLoad::getN(int N) {
//     UINT64 val = 0;
//     for (int i = N-1; i>=0; i--)
//         val |= (UINT64(get()) << (8*i));
//     return val;
// }

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


