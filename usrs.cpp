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
    m_cur = m_end = m_page_count = 0;
    m_llen = 0;
    m_solid = false;
    x_llen = new XFileSave("usr.x");
    x_sgen = new XFileSave("usr.pfg");
    x_sqlt = new XFileSave("usr.pfq");

    x_lgen = new XFileSave("usr.lgen");
    x_lqlt = new XFileSave("usr.lqlt");
    x_lrec = new XFileSave("usr.lrec");

    m_in   = conf.file_usr();
    load_page();
    if (m_valid)
        determine_record();
}

UsrSave::~UsrSave(){
    if (not conf.quiet) {
        fprintf(stderr, "::: USR  num recs: %llu \t| EX size len/sgen/sqlt: %lu/%lu/%lu\n", g_record_count-1,
                x_llen->tell(), x_sgen->tell(), x_sqlt->tell());
        if (x_lrec and
            x_lrec->tell())
            fprintf(stderr, "::: USR  oversized records: rec/gen/qlt %lu/%lu/%lu\n",
                    x_lrec->tell(), x_lgen->tell(), x_lqlt->tell());
    }
    DELETE(x_llen);
    DELETE(x_sgen);
    DELETE(x_sqlt);

    DELETE(x_lrec);
    DELETE(x_lgen);
    DELETE(x_lqlt);
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
    switch (type) {
    case ET_LLEN:
        x_llen -> put ( g_record_count - m_last.i_llen );
        x_llen -> put ( dat );
        m_last.i_llen = g_record_count;
        m_llen = dat;
        break;

    case ET_SOLPF_GEN:
        x_sgen->put( g_record_count - m_last.i_sgen);
        x_sgen->put_chr( dat );
        m_last.i_sgen = g_record_count;
        m_last.solid_pf_gen = dat;
        break;

    case ET_SOLPF_QLT:
        x_sqlt->put( g_record_count - m_last.i_sqlt);
        x_sqlt->put_chr( dat );
        m_last.i_sqlt = g_record_count;
        m_last.solid_pf_qlt = dat;
        break;

    default : assert(0);
    }
}

void UsrSave::expect(UCHAR chr) {
    likely_if (m_buff[m_cur++] == chr)
        return;

    croak("fastq file: expecting '%c', got '%c' after record %llu", chr, m_buff[m_cur-1], g_record_count);
}

bool UsrSave::mid_rec_msg() const {

    croak("fastq file: record seems truncated  after record %llu", g_record_count);
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

UCHAR UsrSave::load_char() {
    if (m_cur >= m_end)
        load_page();
    return m_buff[m_cur++];
}

void UsrSave::determine_record() {

    int q = m_cur;

    rarely_if(m_cur >= m_end) {
        if (not conf.quiet) {
            if (x_lrec->tell())
                fprintf(stderr, "::: HEY all records were oversized\n");
            else
                fprintf(stderr, "::: HEY no records were found");
        }
        return;
    }

    rarely_if(m_buff[q++] != '@')
        croak("first record: Missing prefix '@', is it really a fastq format?");

    int sanity = MAX_ID_LLEN;
    while (--  sanity and m_buff[q] != '\n')
        q ++ ;
    rarely_if( not sanity) {
        // croak("first record: REC is too long");
        g_record_count++;
        get_oversized_record(m_cur, false);
        return determine_record();
    }

    if (m_buff[q++] != '\n')
        croak("first record: Expected newline, got '%c'", m_buff[q-1]);

    int qg = q;
    // LLEN
    for (int i = 1;
         i < MAX_GN_LLEN and not m_llen;
         i++)
        if (m_buff[q + i] == '\n')
            m_llen = i;

    if (not m_llen) {
        // croak("first record: GEN is too long");
        g_record_count++;
        get_oversized_record(m_cur, false);
        return determine_record();
    }

    q += m_llen + 1;
    if (m_buff[q] != '+')
        croak("first record: Missing 2nd prefix '+', is it really a fastq format?");

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
        switch (m_buff[qg + i] | 0x20) {
        case '0': case '1': case '2': case '3':
            m_solid = true;
            break;

        case 'a': case 'c' : case 'g' : case 't':
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

bool UsrSave::get_oversized_record(int cur, bool from_get) {
    // collect this record
    x_lrec->put( g_record_count - m_last.i_long);
    m_last.i_long = g_record_count ;

    m_cur = cur ;               // zap back
    UCHAR c = load_char();
    if ('@' != c)
        croak("record %llu: bad (long) record", g_record_count);

#define CHK_VALID if (!m_valid) croak("record %llu: seems truncated", g_record_count)
#define PUT_LINE(X) do { c = load_char(); X->put_chr(c); } while (c != '\n' and m_valid)

    PUT_LINE(x_lrec);
    CHK_VALID;

    PUT_LINE(x_lgen);
    CHK_VALID;

    PUT_LINE(x_lrec);           // 2nd rec
    CHK_VALID;

    PUT_LINE(x_lqlt);
    CHK_VALID;

#undef PUT_LINE
#undef CHK_VALID
    if (from_get) {
        g_record_count ++ ;
        return get_record();
    }
    return true;
}

bool UsrSave::get_record() {

    load_check();

#define CHECK_OVERFLOW rarely_if (m_cur >= m_end) return mid_rec_msg()

    if (m_cur >= m_end) return m_valid = false;

    int currec = m_cur;
    expect('@');
    int sanity = MAX_ID_LLEN;
    while (--  sanity and m_buff[m_cur] != '\n')
        m_cur ++ ;
    rarely_if( not sanity)
        // croak("record %llu: rec line too long", g_record_count);
        return get_oversized_record(currec);

    mp.rec_end = &(m_buff[m_cur]);
    CHECK_OVERFLOW ;
    expect('\n');

    UCHAR update_solid_pf = 0;
    if (m_solid) {
        rarely_if (m_last.solid_pf_gen != m_buff[m_cur])
            update_solid_pf = m_buff[m_cur];
        m_cur++;
    }

    mp.gen = &(m_buff[m_cur]);
    const int gi = m_cur ;
    sanity = MAX_GN_LLEN;
    while (-- sanity and m_buff[m_cur] != '\n')
        m_cur ++;

    rarely_if( not sanity)
        // croak("recrod %llu: gen line too long", g_record_count);
        return get_oversized_record(currec);

    rarely_if(update_solid_pf)  // update only after last potential get_oversized_record
        update(ET_SOLPF_GEN, update_solid_pf);

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

    mp.prev_rec = mp.rec;
    mp.prev_rec_end = mp.rec_end;
    mp.rec = &(m_buff[currec+1]);

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

    UINT32  sanity = conf.profiling ? 100000 : 3000000000;

    RecSave rec;
    GenSave gen;
    QltSave qlt;

    while( ++ g_record_count < sanity  and get_record() ) {
        gen.save(mp.gen, mp.qlt, m_llen);
        rec.save(mp.rec, mp.rec_end, mp.prev_rec, mp.prev_rec_end);
        qlt.save(mp.qlt, m_llen);
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
    m_2nd_rec = conf.get_long("usr.2id");
    m_solid   = conf.get_bool("usr.solid");
    if (comp_version > 5) {
        assert(0);              // TODO
    }
    else {
        m_llen = conf.get_long("llen");
        m_qlen = m_llen;
    }

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

    x_llen = new XFileLoad("usr.x");
    x_sgen = new XFileLoad("usr.pfg");
    x_sqlt = new XFileLoad("usr.pfq");

    m_last.i_llen = x_llen->get();
    m_last.i_sgen = x_sgen->get();
    m_last.i_sqlt = x_sqlt->get();

    x_lrec = new XFileLoad("usr.lrec");
    m_last.i_long = x_lrec->get();
    if (m_last.i_long) {
        x_lgen = new XFileLoad("usr.lgen");
        x_lqlt = new XFileLoad("usr.lqlt");
    }
    else {
        DELETE(x_lrec);
        x_lgen = x_lqlt = NULL;
    }
}

UsrLoad::~UsrLoad() {
    DELETE(x_llen);
    DELETE(x_sgen);
    DELETE(x_sqlt);

    DELETE(x_lgen);
    DELETE(x_lqlt);
    DELETE(x_lrec);
}

void UsrLoad::update() {

    rarely_if(m_last.i_long == g_record_count) {
        UCHAR c = '@';
        fputc(c, m_out);
#define PUT_LINE(X) do { c = X->get_chr(); fputc(c, m_out); } while (c != '\n')
        PUT_LINE(x_lrec);
        PUT_LINE(x_lgen);
        PUT_LINE(x_lrec);
        PUT_LINE(x_lqlt);
#undef PUT_LINE
        m_last.i_long += x_lrec->get();
        g_record_count++;
        return update();
    }
    if (comp_version > 5) {
        assert(0);              // TODO
    }
    else {
        rarely_if(m_last.i_llen == g_record_count) {
            m_llen = x_llen -> get();
            m_qlen = m_llen;
            m_last.i_llen += x_llen->get();
            m_gen[m_llen+1] = '\n';
            m_gen[m_llen+2] = '+';
        }
    }
    rarely_if(m_solid and
              m_last.i_sgen == g_record_count) {
        m_last.solid_pf_gen = m_gen[0] = x_sgen->get_chr();
        m_last.i_sgen += x_sgen->get();
    }
    rarely_if(m_solid and
              m_last.i_sqlt == g_record_count) {
        m_last.solid_pf_qlt = m_qlt[0] = x_sqlt->get_chr();
        m_last.i_sqlt += x_sqlt->get();
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
    SAVE(m_qlt_ptr, m_qlen + m_llen_factor);
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

    rarely_if( ! n_recs and not m_last.i_long)
        croak("Zero records, what's going on?");

    comp_version = conf.decoder_version;

    RecLoad rec;
    GenLoad gen;
    QltLoad qlt;

    UCHAR* b_qlt = m_qlt+1 ;
    UCHAR* b_gen = m_gen+1 ;

    while (1) {
        g_record_count++;
        update();
        if (g_record_count > n_recs)
            break;

        UCHAR* b_rec = (flip ? m_rep : m_rec)+1 ;
        UCHAR* p_rec = (flip ? m_rec : m_rep)+1 ;

        m_rec_size = rec.load(b_rec, p_rec);
        rarely_if (not m_rec_size)
            croak("premature EOF - %llu records left", n_recs+1);

        qlt.load(b_qlt, m_qlen);
        gen.load(b_gen, b_qlt, m_llen);
        save();
    }
    // sanity: verify all objects are done (by croak?)
    return 0;
}
