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


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "usrs.hpp"

#include "gens.hpp"
#include "recs.hpp"
#include "qlts.hpp"

UsrSave::UsrSave() {
    BZERO(m_last);
    BZERO(mp);

    m_valid  = true ;
    m_cur = m_end = m_rec_total = m_page_count = 0;
    m_llen = 0;
    m_solid = false;
    x_file = new XFileSave("usr.x");

    m_in   = conf.file_usr();
    load_page();
}

UsrSave::~UsrSave(){
    if (not conf.quiet and x_file)
        fprintf(stderr, "::: USR  num recs: %llu \t| EX size: %lu\n", g_record_count-1, x_file->tell());
    if (x_file) {
        if (x_file->has_file()) {
            x_file->put(10);
            x_file->put_chr(ET_END);
        }
        delete(x_file);
    }
}

void UsrSave::load_page() {

    size_t size  = m_end - m_cur ;
    assert(size <= PLL_STRT);
    size_t start = PLL_STRT - size;
    for (size_t i = 0 ; i < size; i++)
        m_buff[start+i] = m_buff[m_cur+i];

    likely_if (mp.rec != NULL) {
        UCHAR* pl = mp_last;
        for (UCHAR* p = mp.rec; p < mp.rec_end; p++ , pl++)
            *pl = *p;
        mp.rec = mp_last;
        mp.rec_end = pl;
        *pl++ = '\n';           // Just in case
        *pl   = 0;              // clean debug
    }
    
    size_t cnt = fread(&m_buff[PLL_STRT], 1, PLL_SIZE, m_in);
    m_cur  = start;
    m_end = PLL_STRT + cnt  ;

    if (m_cur == m_end) 
        m_valid = false;
    else
        m_page_count++;
}

void UsrSave::update(exception_t type, UINT16 dat) {
    x_file->put(g_record_count - m_last.index);
    x_file->put_chr(type);
    x_file->put(dat);
    m_last.index = g_record_count;

    switch (type) {
    case ET_LLEN     : m_llen = dat; break;
    case ET_SOLPF_GEN: m_last.solid_pf_gen = dat; break;
    case ET_SOLPF_QLT: m_last.solid_pf_qlt = dat; break;
    default : assert(0);
    }
}

void UsrSave::expect(UCHAR chr) {
    likely_if (m_buff[m_cur++] == chr)
        return;
    fprintf(stderr, "fastq file: expecting '%c', got '%c' after record %llu\n", chr, m_buff[m_cur-1], g_record_count);
    exit (1);
}

bool UsrSave::mid_rec_msg() const {
    fprintf(stderr, "fastq file: record seems truncated  after record %llu\n", g_record_count);
    exit (1);
}

void UsrSave::load_check() {
    likely_if (m_cur+PLL_STRT < m_end)
        return;
    for (int i = m_cur, cnt=0; i < m_end; i++)
        if (m_buff[i] == '\n')
            if (++cnt >= 4)
                return;
    load_page();
}

void UsrSave::determine_record() {
    // LLEN
    for (int i = 1;
         i < MAX_GN_LLEN and not m_llen;
         i++)
        if (m_buff[m_cur + i] == '\n')
            m_llen = i;

    if (not m_llen)
        croak("1st id line is too long");

    int q = m_cur + m_llen + 1;

    if (m_buff[q] != '+')
        croak("Missing '+', is it really a fastq format?");

    // 2ND ID
    bool has_2nd_id = false;
    while (m_buff[++ q ] != '\n')
        if (m_buff[ q ] != ' ' )
            has_2nd_id = true;

    // SOLID
    bool d_solid = false;
    for (int i = 1;
         i < m_llen and not d_solid and not m_solid;
         i ++ )
        switch (m_buff[m_cur + i] | 0x20) {
        case '0': case '1': case '2': case '3':
            m_solid = true;
            break;

        case 'A': case 'C' : case 'G' : case 'T':
            d_solid = true;
            break;

        default:
            // TODO: what if first record is all Ns ?
            // - seek in other records
            break;
        }

    if (m_solid) {
        conf.set_info("usr.solid", m_solid);
        m_llen --;
    }
    conf.set_info("llen", m_llen);
    conf.set_info("usr.2id", has_2nd_id); // TODO
}

bool UsrSave::get_record() {
    
    load_check();

#define CHECK_OVERFLOW rarely_if (m_cur >= m_end) return mid_rec_msg()

    if (m_cur >= m_end) return m_valid = false;
    expect('@');
    mp.prev_rec = mp.rec;
    mp.prev_rec_end = mp.rec_end;
    mp.rec = &(m_buff[m_cur]);
    int sanity = MAX_ID_LLEN;
    while (--  sanity and m_buff[m_cur] != '\n')
        m_cur ++ ;
    rarely_if( not sanity)
        croak("record %llu: rec line too long", g_record_count);

    mp.rec_end = &(m_buff[m_cur]);
    CHECK_OVERFLOW ;
    expect('\n');

    rarely_if (not m_llen)      // TODO: call from constructor 
        determine_record();

    if (m_solid) {
        rarely_if (m_last.solid_pf_gen != m_buff[m_cur])
            update(ET_SOLPF_GEN, m_buff[m_cur]);

        m_cur++;
    }

    mp.gen = &(m_buff[m_cur]);
    const int gi = m_cur ;
    sanity = MAX_GN_LLEN;
    while (-- sanity and m_buff[m_cur] != '\n')
        m_cur ++;

    rarely_if( not sanity)
        croak("recrod %llu: gen line too long", g_record_count);

    CHECK_OVERFLOW;
    rarely_if(m_llen != m_cur-gi)
        update(ET_LLEN, m_cur-gi);

    expect('\n');
    expect('+' );
    for (int sanity = MAX_ID_LLEN;
         --  sanity and m_buff[m_cur] != '\n';
         m_cur ++ );
    CHECK_OVERFLOW;
    expect('\n');

    if (m_solid) {
        rarely_if (m_last.solid_pf_qlt != m_buff[m_cur])
            update(ET_SOLPF_QLT, m_buff[m_cur]);

        m_cur++;
    }
 
    mp.qlt = &(m_buff[m_cur]);
    m_cur += m_llen;
    CHECK_OVERFLOW;

    likely_if (m_cur < m_end)
        expect('\n');

    return true;

#undef  CHECK_OVERFLOW
}

// UINT64 UsrSave::estimate_rec_limit() {
//     int  i, cnt;
//     for (i = m_cur, cnt=0;
//          i < m_end and cnt < 4;
//          i ++ )
//         if (m_buff[i] == '\n')
//             cnt ++;
//     
//     if (cnt < 4)
//         croak("This usr file is too small");
// 
//     UINT64 limit = conf.partition.size / (i-m_cur);
//     // if (limit < 500000)
//     if (limit < 5000) // for debuging
//         croak("This partition partition is too small (records limit=%lld)", limit);
// 
//     return limit;
// }

int UsrSave::encode() {

    UINT32  sanity = conf.profiling ? 100000 : 1000000000;

    RecSave rec;
    GenSave gen;
    QltSave qlt;

    switch (conf.level) {

    case 1:
        while( ++ g_record_count < sanity  and get_record() ) {
            gen.save_1(mp.gen, mp.qlt, m_llen);
            rec.save_2(mp.rec, mp.rec_end, mp.prev_rec, mp.prev_rec_end);
            qlt.save_1(mp.qlt, m_llen);
        } break;
    case 2: default:
        while( ++ g_record_count < sanity  and get_record() ) {

            gen.save_2(mp.gen, mp.qlt, m_llen);
            rec.save_2(mp.rec, mp.rec_end, mp.prev_rec, mp.prev_rec_end);
            qlt.save_2(mp.qlt, m_llen);
        } break;
    case 3:
        while( ++ g_record_count < sanity  and get_record() ) {

            gen.save_3(mp.gen, mp.qlt, m_llen);
            rec.save_2(mp.rec, mp.rec_end, mp.prev_rec, mp.prev_rec_end);
            qlt.save_3(mp.qlt, m_llen);
        } break;
    case 4:
        while( ++ g_record_count < sanity  and get_record() ) {
            gen.save_4(mp.gen, mp.qlt, m_llen);
            rec.save_2(mp.rec, mp.rec_end, mp.prev_rec, mp.prev_rec_end);
            qlt.save_3(mp.qlt, m_llen);
        } break;
    }
    conf.set_info("num_records", g_record_count-1);
    return 0;
}

// load

UsrLoad::UsrLoad() {
    BZERO(m_last);
    flip = 0;
    m_out = conf.file_usr();
    m_rec[0] = '@' ;
    m_rep[0] = '@' ;
    m_llen    = conf.get_long("llen");
    m_2nd_rec = conf.get_long("usr.2id");
    m_solid   = conf.get_bool("usr.solid");

    if (not m_2nd_rec) {
        m_gen[m_llen+1] = '\n';
        m_gen[m_llen+2] = '+' ;
    }

    if (m_solid) {
        m_gen_ptr = m_gen;
        m_qlt_ptr = m_qlt;
        m_llen_factor = 1;
    }
    else {
        m_gen_ptr = m_gen + 1;
        m_qlt_ptr = m_qlt + 1;
        m_llen_factor = 0;
    }

    x_file = new XFileLoad("usr.x");
    m_last.index = x_file->get();
    m_rec_total = 0;
}

UsrLoad::~UsrLoad() {
    DELETE(x_file);
}

void UsrLoad::update() {
    int sanity = ET_END;
    while (m_last.index == g_record_count) {

        if (not sanity--)
            croak("UsrLoad: illegal exception list");
        UCHAR type = x_file->get_chr(); 
        UINT16 data = x_file->get();
        rarely_if ( not data and
                    not x_file->is_valid()) break;
        switch (type) {
        case ET_LLEN:
            m_llen = data;
            m_gen[m_llen+1] = '\n';
            m_gen[m_llen+2] = '+';
            break;
        case ET_SOLPF_GEN:
            m_gen[0] = m_last.solid_pf_gen = data;
            break;
        case ET_SOLPF_QLT:
            m_qlt[0] = m_last.solid_pf_qlt = data;
            break;
        case ET_END:
            return ;

        default: assert(0);
        }
        m_last.index += x_file->get(); 
    }    
}

void UsrLoad::save() {

    size_t llen = m_llen + m_llen_factor ;

#define SAVE(X, L) putline(X, L)
    UCHAR* p_rec = flip ? m_rep : m_rec ;
    flip = flip ? 0 : 1 ;

    SAVE(p_rec, m_rec_size+1);
    if (m_2nd_rec) {
        SAVE(m_gen_ptr, llen);
        p_rec[0] = '+';
        SAVE(p_rec, m_rec_size+1);
        p_rec[0] = '@';
    }
    else {
        SAVE(m_gen_ptr, llen + 2);
    }
    SAVE(m_qlt_ptr, llen);
#undef  SAVE

}

void UsrLoad::putline(UCHAR* buf, UINT32 size) {
    buf[size++] = '\n';
    size_t cnt = fwrite(buf, 1, size, m_out);

    rarely_if (cnt != size)
        croak("USR: Error writing output");
}

int UsrLoad::decode() {

    size_t n_recs = conf.get_long("num_records");

    if ( ! n_recs)
        croak("Zero records, what's going on?");

    RecLoad rec;
    GenLoad gen;
    QltLoad qlt;

    UCHAR* b_qlt = m_qlt+1 ;
    UCHAR* b_gen = m_gen+1 ;

    switch (conf.level) {
    case 1:
        while (n_recs --) {
            g_record_count++;
            UCHAR* b_rec = (flip ? m_rep : m_rec)+1 ;
            UCHAR* p_rec = (flip ? m_rec : m_rep)+1 ;

            rarely_if (g_record_count == m_last.index) update();
            m_rec_size = rec.load_2(b_rec, p_rec);
            rarely_if (not m_rec_size)
                croak("premature EOF - %llu records left", n_recs+1);

            qlt.load_1(b_qlt, m_llen);
            gen.load_1(b_gen, b_qlt, m_llen);
            save();
        } break;
    case 2: default:
        while (n_recs --) {
            g_record_count++;
            UCHAR* b_rec = (flip ? m_rep : m_rec)+1 ;
            UCHAR* p_rec = (flip ? m_rec : m_rep)+1 ;
            rarely_if (g_record_count == m_last.index) update();
            m_rec_size = rec.load_2(b_rec, p_rec);
            rarely_if (not m_rec_size)
                croak("premature EOF - %llu records left", n_recs+1);

            qlt.load_2(b_qlt, m_llen);
            gen.load_2(b_gen, b_qlt, m_llen);
            save();
        } break;
    case 3:
        while (n_recs --) {
            g_record_count++;
            UCHAR* b_rec = (flip ? m_rep : m_rec)+1 ;
            UCHAR* p_rec = (flip ? m_rec : m_rep)+1 ;
            rarely_if (g_record_count == m_last.index) update();
            m_rec_size = rec.load_2(b_rec, p_rec);
            rarely_if (not m_rec_size)
                croak("premature EOF - %llu records left", n_recs+1);

            qlt.load_3(b_qlt, m_llen);
            gen.load_3(b_gen, b_qlt, m_llen);
            save();
        } break;
    case 4:
        while (n_recs --) {
            g_record_count++;
            UCHAR* b_rec = (flip ? m_rep : m_rec)+1 ;
            UCHAR* p_rec = (flip ? m_rec : m_rep)+1 ;
            rarely_if (g_record_count == m_last.index) update();
            m_rec_size = rec.load_2(b_rec, p_rec);
            rarely_if (not m_rec_size)
                croak("premature EOF - %llu records left", n_recs+1);

            qlt.load_3(b_qlt, m_llen);
            gen.load_4(b_gen, b_qlt, m_llen);
            save();
        } break;
    }
    // sanity: verify all objects are done (by croak?)
    return 0;
}
