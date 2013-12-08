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
#include "common.hpp"
#include "xfile.hpp"

// static void croak(const char* fmt, long long num) __attribute__ ((noreturn, cold));
// static void croak(const char* fmt, long long num) {
//     fprintf(stderr, fmt, num, errno ? strerror(errno) : "");
//     fprintf(stderr, "\n");
//     exit(1);
// }

struct OneFile {
    struct file_attr_t {

        UINT64 name;
        UINT64 size ; // in bytes
        UINT32 first; // page index
        UINT32 node ; // page index
    } PACKED;
    enum {
        max_root_files = FILER_PAGE / sizeof(file_attr_t),
        max_node_files = FILER_PAGE / 4,
    };
    file_attr_t files[max_root_files+1]; // pad to fit one page
    UINT32 next_findex;                  // offset in files table
    UINT32 num_pages;                    // free alloc index

    // UINT32 files_index;  - currently limitted to single root dir (341 files, including zero).

    FILE* m_out;
    FILE* m_in ;

    UINT32 get_findex() {
        assert(m_out);
        rarely_if(next_findex >= max_root_files)
            croak("Internal error: Too many open files: %d %s", next_findex);
        return next_findex ++;
    }
    UINT32 get_findex(UINT64 name) {
        assert(m_in);
        for(UINT32 i = 1; i < next_findex; i++)
            if (files[i].name == name)
                return i;
        return 0;
    }
    UINT32 allocate() {
        return num_pages ++;
    }
    void read_page(UINT32 offset, UCHAR* page) {
        // Note: -D_FILE_OFFSET_BITS=64 is required
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
        m_in  = in;
        m_out = NULL;

        read_page(1, (UCHAR*)files);
        next_findex = files[0].first;
        files[0].first = 0;
    }
    void do_confess() const {
        fprintf(stderr, "i: 'name'\t: size\t: first\t: node\n");
        for (UINT32 i = 0;
             i < next_findex;
             i++) {
            char name[10];
            strncpy(name, (char*)&files[i].name, 8);
            fprintf(stderr, "%d: '%s'\t: %lld\t: %d:\t: %d\n",
                    i, name, files[i].size, files[i].first, files[i].node);
        }
    }
    void init_write(FILE* out) {
        next_findex = 1;
        m_in  = NULL;
        m_out = out;

        files[0].name = 0;
        files[0].first = 0;
        num_pages = 2;
    }
    void finit_write() {
        if (m_out) {
            files[0].first = next_findex;
            write_page(1, (UCHAR*)files);
            fclose(m_out);
            m_out = NULL;
        }
    }
    ~OneFile() { finit_write() ; } // call explicitly from config?
} onef ;

// Static 

void FilerSave::init(FILE* out) { onef.init_write(out); }
void FilerSave::finit()         { onef.finit_write()  ; }
void FilerLoad::init(FILE* in)  { onef.init_read(in)  ; }
void FilerLoad::confess()       { onef.do_confess()   ; }

// Base
static UINT64 name2u(const char* name) {
    UINT64 uname ;
    assert(strlen(name) <=8 );
    strncpy((char*)&uname, name, 8);
    return uname;
}

FilerBase::FilerBase() {
    m_node_i = 0;
    m_node_p = 0;

    m_valid = true;
    m_cur = m_count = 0;
    m_page_count = 0;
}

// Save

bool FilerSave::is_valid() const { return m_valid; }

FilerSave::FilerSave(const char* name) {
    UINT32 fi = m_onef_i = onef.get_findex();
    onef.files[fi].name = name2u(name);
    onef.files[fi].size = 0;
    onef.files[fi].node = 0;
    onef.files[fi].first = onef.allocate();
}

FilerSave::FilerSave(int forty_two) {
    assert(forty_two == 42);    // Verify this version wasn't called by mistake

    BZERO(onef.files[0]);
    m_onef_i = 0;
}

FilerSave::~FilerSave() {
    save_page();
    m_valid = false;
    if (m_node_p)
        save_node(0);
}

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

    rarely_if (not m_node_p) {
        assert(m_node_i == 0);
        onef.write_page(onef.files[m_onef_i].first, m_buff);
        if (m_cur == FILER_PAGE) // Don't allocate at EOF (harmless in the rare case of exact one page file)
            onef.files[m_onef_i].node = m_node_p =  onef.allocate();
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

FilerLoad::FilerLoad(const char* name, bool* valid_ptr) {

    m_onef_i = onef.get_findex(name2u(name));
    if (not m_onef_i) {
        *valid_ptr = m_valid = false;
        return;
    }
    m_valid_ptr = valid_ptr;
    * valid_ptr = m_valid = true ;
    load_page();
}

FilerLoad::FilerLoad(int forty_two, bool* valid_ptr) {
    m_onef_i = 0;
    m_valid_ptr = valid_ptr;
    * valid_ptr = m_valid = true ;
    load_page();
}

FilerLoad::~FilerLoad() {
    if ( m_valid_ptr )
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

    rarely_if (tell() >= onef.files[m_onef_i].size ) {
        *m_valid_ptr = m_valid = false;
        return;                 // EOF
    }
 
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
            m_node_p = m_node[ maxi_nodes ]; // keep it for debugging
            onef.read_page(m_node_p, (UCHAR*) m_node);
            m_node_i = 0;
        }
        onef.read_page(m_node[m_node_i ++], m_buff);
    }

    m_cur = 0;
    UINT64 size = onef.files[m_onef_i].size;
    m_count = size/FILER_PAGE == m_page_count ? size % FILER_PAGE : FILER_PAGE ;
    m_page_count++;
}

void FilerSave::save_bookmark(BookMark & bmk) const {
    UINT16 i = bmk.mark.nfiles ++ ;
    bmk.file[i].onef_i = m_onef_i ;
    bmk.file[i].node_p = m_node_p ;
    bmk.file[i].node_i = m_node_i ;
    bmk.file[i].page_cnt = m_page_count;
    bmk.file[i].page_cur = m_cur ;
}

void FilerLoad::load_bookmark() {

}
