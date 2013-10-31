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

#include "recs.hpp"

#define MAX_LLINE 400           // TODO: assert max is enough at constructor


void RecBase::range_init() {
    BZERO(ranger);
    BZERO(rangerx);
}

RecSave::RecSave() {

    m_valid = true;
    m_type = 0;

    BZERO(m_last);
    BZERO(stats);
    BZERO(m_ids);
    
    range_init();

    filer = new FilerSave(conf.open_w("rec"));
    assert(filer);
    rcoder.init(filer);

    filerx = new FilerSave(conf.open_w("rec.x"));
    assert(filerx);
    rcoderx.init(filerx);
}

RecSave::~RecSave() {
    putx_u(0, 0);
    putx_u(1, ET_END);
    rcoder.done();
    rcoderx.done();
    DELETE(filer);
    DELETE(filerx);
    fprintf(stderr, "::: REC big u/i: %u/%u ex:%u\n",
            stats.big_u, stats.big_i, stats.exceptions);
}

static bool is_digit(UCHAR c) {
    return c >= '0' and c <= '9';
}

static const char* backto(char c, const char* p, const char* start) {

    while (*p != c)
        if(*p == '\n' or --p < start)
            return NULL;

    return ++p ; // is_digit(*(++p)) ? p : NULL ;
}

static const UCHAR* backto(UCHAR c, const UCHAR* p, const UCHAR* start) {
    return (UCHAR*) backto((char)c, (const char*)p, (const char*)start);
}

static const bool be_digits(const char* p, const char* end) {
    // How do I miss perl regex ...
    if (not p)
        return false;
    for (; p < end ; p ++)
        if (not is_digit(*p))
            return false;
    return true;
}

int RecSave::get_rec_type(const char* start) {

#define MUST(X) if (not (X)) break

    const char* end = index(start, 0);

    do {
        const char* p_l = index(start, '.');
        MUST(p_l);
        const char* p_s = index(p_l, ' '  );
        MUST(p_s++);
        MUST(be_digits(p_l+1, p_s-1)); // line

        const char* p = end;
        for (int i = 0; p and i < 4; i++) 
            p = backto(':', p-2, p_s);
        MUST(p);
        p -= 2;

        const char* p1 = index(p, ':');
        MUST(p1);

        const char* p2 = index(p1+1, ':');
        MUST(p2);
        MUST(be_digits(p1+1, p2));

        const char* p3 = index(p2+1, ':');
        MUST(p3);
        MUST(be_digits(p2+1, p3));

        const char* p4 = index(p3+1, ':');
        MUST(p4);
        MUST(be_digits(p3+1, p4));

        do {
            // @ERR042980.2 FCC01NKACXX:8:1101:1727:1997#CGATGTAT/1
            // @ERR243065.1 HS9_09378:1:1101:1060:13075#36/1
            const char* pm = index(p4+1, '#');
            MUST(pm);
            bool is_iseq = *(pm+1) < '0';
            const char* pa = index(pm+1, '/');
            MUST(pa);
            MUST(be_digits(pa+1, end));
            // congratulations
            m_ids[0] = strndup(start, p_l-start);
            m_ids[1] = strndup(p_s  , p1-p_s);
            conf.set_info("rec.in", m_ids[0]);
            conf.set_info("rec.id", m_ids[1]);
            return is_iseq ? 2 : 8;
        } while (0);

        do {
            // @SRR043409.575 61ED1AAXX100429:3:1:1039:1223/1
            const char* pa = index(p4+1, '/');
            MUST(pa);
            MUST(be_digits(pa+1, end));
            // congratulations
            m_ids[0] = strndup(start, p_l-start);
            m_ids[1] = strndup(p_s  , p1-p_s);
            conf.set_info("rec.in", m_ids[0]);
            conf.set_info("rec.id", m_ids[1]);
            return 1;
        } while(0);

        do {
            // @SRR004635.142 HWI-EAS292_30CWN:7:1:1474:1889 length=36
            const char* plen = index(p4+1, ' ');
            MUST(plen);
            MUST(0 == strncmp(plen, " length=", 8));
            MUST(be_digits(plen+8, end));

            m_ids[0] = strndup(start, p_l-start);
            m_ids[1] = strndup(p_s  , p1-p_s);
            conf.set_info("rec.in", m_ids[0]);
            conf.set_info("rec.id", m_ids[1]);
            return 7;
        } while (0);

    } while (0);

    // do {
    //     // @ERR042980.2 FCC01NKACXX:8:1101:1727:1997#CGATGTAT/1
    //     const char* p_l = index(start, '.');
    //     MUST(p_l);
    //     const char* p_s = index(p_l, ' '  );
    //     MUST(p_s++);
    //     MUST(be_digits(p_l+1, p_s-1)); // this part matches qr/\..*?\s/
    // 
    //     const char* p_a = backto('/', end-1, end-3);
    //     MUST(be_digits(p_a, end));
    // 
    //     const char* p_m = backto('#', p_a-2, p_a - 30);
    //     MUST(p_m);
    // 
    //     const char* p_y = backto(':', p_m-2, p_m - 10);
    //     MUST(be_digits(p_y, p_m-1));
    // 
    //     const char* p_x = backto(':', p_y-2, p_y - 10);
    //     MUST(be_digits(p_x, p_y-1));
    // 
    //     const char* p_t = backto(':', p_x-2, p_x - 10);
    //     MUST(be_digits(p_t, p_x-1));
    // 
    //     MUST(p_t > p_s and p_t - p_s > 4);
    // 
    //     // congratulations
    //     m_ids[0] = strndup(start, p_l-start);
    //     m_ids[1] = strndup(p_s  , p_t-p_s-1);
    //     m_ids[2] = strndup(p_m  , p_a-p_m-1);
    //     conf.set_info("rec.2.in", m_ids[0]);
    //     conf.set_info("rec.2.id", m_ids[1]);
    //     conf.set_info("rec.2.is", m_ids[2]);
    //     return 2;
    // } while (0);

    do {
        // @SRR387414.86006 0099_20110922_1_SL_AWG_TG_NA21112_1_1pA_01003468541_1_2_25_167/2
        // @SRR387414.86010 0099_20110922_1_SL_AWG_TG_NA21112_1_1pA_01003468541_1_2_25_229/2
        const char* p_l = index(start, '.');
        MUST(p_l);
        const char* p_s = index(p_l, ' '  );
        MUST(p_s++);
        MUST(be_digits(p_l+1, p_s-1)); // this part matches qr/\..*?\s/

        const char* p_a = backto('/', end-1, end-3);
        MUST(be_digits(p_a, end));

        const char* p_1 = backto('_', p_a-2, p_a - 30);
        MUST(be_digits(p_1, p_a-1));

        const char* p_2 = backto('_', p_1-2, p_1 - 30);
        MUST(be_digits(p_2, p_1-1));

        const char* p_3 = backto('_', p_2-2, p_2 - 30);
        MUST(be_digits(p_3, p_2-1));

        const char* p_4 = backto('_', p_3-2, p_3 - 30);
        MUST(be_digits(p_4, p_3-1));

        m_ids[0] = strndup(start, p_l-start);
        m_ids[1] = strndup(p_s  , p_4-p_s-1);
        conf.set_info("rec.in" , m_ids[0]);
        conf.set_info("rec.id" , m_ids[1]);

        return 3;
    } while (0);

    do {
        // @ERR048944.1 solid0743_20110810_PE_BC_PEP20110805002A_bcSample1_1_2_225/1
        const char* p_l = index(start, '.');
        MUST(p_l);
        const char* p_s = index(p_l, ' '  );
        MUST(p_s++);
        MUST(be_digits(p_l+1, p_s-1)); // this part matches qr/\..*?\s/

        const char* p_a = backto('/', end-1, end-3);
        MUST(be_digits(p_a, end));

        const char* p_1 = backto('_', p_a-2, p_a - 30);
        MUST(be_digits(p_1, p_a-1));

        const char* p_2 = backto('_', p_1-2, p_1 - 30);
        MUST(be_digits(p_2, p_1-1));

        const char* p_3 = backto('_', p_2-2, p_2 - 30);
        MUST(be_digits(p_3, p_2-1));

        m_ids[0] = strndup(start, p_l-start);
        m_ids[1] = strndup(p_s  , p_3-p_s-1);
        m_ids[2] = strndup(p_2  , p_1-p_2-1);
        m_ids[3] = strndup(p_3  , p_2-p_3-1);

        conf.set_info("rec.in" , m_ids[0]);
        conf.set_info("rec.id" , m_ids[1]);

        return 4;
    } while (0);

    do {
        // @SRR395323.1 1/2    (simple)
        // @SRR395323.2 2/2
        // @SRR395323.3 3/2
        const char* p_l = index(start, '.');
        MUST(p_l);
        const char* p_s = index(p_l, ' '  );
        MUST(p_s++);
        MUST(be_digits(p_l+1, p_s-1)); // this part matches qr/\..*?\s/

        const char* p_r = p_s;
        MUST(p_r);
        // while(*(p_r) == ' ') p_r++;  -- assumes exactly one space
        const char* p_a = index(p_r, '/');
        MUST(p_a);
        MUST(be_digits(p_r, p_a));
        MUST(is_digit(*(p_a+1)));
        MUST(   end == (p_a+2));

        m_ids[0] = strndup(start, p_l-start);
        conf.set_info("rec.in", m_ids[0]);
        conf.set_info("rec.id", "none");

        return 5;
    } while (0);

    do {
        // @SRR097722.102 VAB_0097_20110113_2_SL_ANG_MID001032_TG_sA_01003407209_1_BC12_40_461
        const char* p_l = index(start, '.');
        MUST(p_l);
        const char* p_s = index(p_l, ' '  );
        MUST(p_s++);
        MUST(be_digits(p_l+1, p_s-1)); // this part matches qr/\..*?\s/

        const char* p_1 = backto('_', end-1, end - 30);
        MUST(be_digits(p_1, end));

        const char* p_2 = backto('_', p_1-2, p_1 - 30);
        MUST(be_digits(p_2, p_1-1));

        m_ids[0] = strndup(start, p_l-start);
        m_ids[1] = strndup(p_s  , p_2-p_s-1);

        conf.set_info("rec.in" , m_ids[0]);
        conf.set_info("rec.id" , m_ids[1]);

        return 6;
    } while (0);

#undef  MUST
    croak("Cannot determine record type");
    return 0; 
} // get_rec_type

void RecSave::determine_rec_type(const UCHAR* buf, const UCHAR* end) {

    const char* first = strndup((const char*)buf, end-buf);
    conf.set_info("rec.first", first);

    m_type = get_rec_type(first) ;
    if (not m_type)
        croak("Illegal record type: %u\n", m_type);

    conf.set_info("rec.type", m_type);
    conf.save_info();

    // update m_len
    for (int i = 0 ; i < 10 and m_ids[i]; i++)
        m_len[i] = strlen(m_ids[i]);
}

static const UCHAR* n_front(UCHAR c, const UCHAR* p) {
    for (int sanity = MAX_LLINE;
         --  sanity and c != *p;
         ++ p);
    assert(*p == c);
    ++ p;
    assert(*p >= '0' and *p <= '9');
    return p;
}

static const UCHAR* n_backw(UCHAR c, const UCHAR* p) {
    for (int sanity = 300;
         --  sanity and c != *p;
         -- p);
    assert(*p == c);
    ++ p;
    assert(*p >= '0' and *p <= '9');
    return p;
}

static UINT64 is_to_num(const UCHAR* is, int len) {

    assert(len < 16) ;
    UINT64 num = 0;
    for (int i = 0; i < len; i ++) {
        num <<= 2;
        char c = is[i] | 0x20;
        UCHAR n =
            c == 'a' ? 0 :
            c == 'c' ? 1 :
            c == 'g' ? 2 :
            c == 't' ? 3 :
            4;

        rarely_if(n > 3)
            croak("is_to_num: Illegal base: %c", c);

        num += n;
    }
    return num ;
}

static void num_to_is (UINT64 num, char* is) {

    int len = 0;
    while (is[len] == 'A' ||
           is[len] == 'C' ||
           is[len] == 'G' ||
           is[len] == 'T') len ++;

    for (int i = len-1; i >= 0 ; i --) {
        is[i] = "ACGT"[ num & 3];
        num >>= 2;
    }
}

#define ATOI(X) atoll((const char*)X)

void RecSave::save_allele(char a) {
    if (m_last.alal == a)
        return;
    m_last.alal  = a;
    put_i(0, -1);
    put_u(1,  a);
    stats.exceptions++;
}

void RecSave::update(exception_t type, int val) {
    putx_u(0, m_last.rec_count, &m_last.rec_count_tag);
    putx_u(1, type);
    switch(type) {
    case ET_v0: putx_i(2, val, &m_last.n[0]); break;
    case ET_v1: putx_i(3, val, &m_last.n[1]); break;
    case ET_v2: putx_i(4, val, &m_last.n[2]); break;
    case ET_v3: putx_i(5, val, &m_last.n[3]); break;
    case ET_v9: putx_i(9, val, &m_last.n[9]); break;
    case ET_al: putx_i(6, val); m_last.alal = val; break;
    case ET_iseq: putx_u(7, val); break;
    case ET_END: default : assert(0);
    }
}

bool RecSave::save_t_1(const UCHAR* buf, const UCHAR* end) {
    // @SRR043409.575 61ED1AAXX100429:3:1:1039:1223/1
    const UCHAR* p = n_front('.', buf);
    UINT64 line_n = ATOI(p);
    p = (const UCHAR* )index((char*)p, ' ');
    assert(p);
    p += m_len[1];

    UINT64 n[4];
    for (int i = 0; i < 4; i ++) {
        p = n_front(':', p);
        n[i] = ATOI(p);
    }

    rarely_if(n[0] != m_last.n[0])
        update(ET_v0, n[0]);

    rarely_if(n[1] != m_last.n[1])
        update(ET_v1, n[1]);

    likely_if (m_type != 7) {
        p = n_backw('/', end-1);
        if (*p != m_last.alal)
            update(ET_al, *p);
    }
    else {
        p = backto('=', end-1, end-16);
        assert(p);
        int len = ATOI(p);
        rarely_if (len != (int)m_last.n[9])
            update(ET_v9, len);
    }
    if (m_type == 2 or
        m_type == 8) {
        // Also has sequent number
        p = backto('#', end-1, end-16);

        rarely_if (m_type == 2 and
                   strncmp((const char*)p, m_ids[2], m_len[2])) {
            UINT64 iseq_num = is_to_num(p, m_len[2]);
            num_to_is(iseq_num, (char*)m_ids[2]); 
            update(ET_iseq, iseq_num);
        }
        rarely_if(m_type == 8 and
                  m_last.n[9] != (UINT64)ATOI(p)) {
            update(ET_v9, ATOI(p));
        }
    }

    // p = n_backw(':', p-2);
    // UINT64 y = ATOI(p);
    // 
    // p = n_backw(':', p-2);
    // UINT64 x = ATOI(p);
    // 
    // p = n_backw(':', p-2);
    // UINT64 t = ATOI(p);
    // 
    // if (m_last.tile != t) {
    //     m_last.tile  = t;
    //     put_i(0, -3);
    //     put_u(1,  t);
    //     stats.exceptions++;
    // }

    assert(line_n > m_last.line_n);
    put_u(0, line_n, &m_last.line_n);
    put_i(2, n[2], &(m_last.n[2]));
    put_i(3, n[3], &(m_last.n[3]));
    // put_i(2, x, &m_last.x);
    // put_i(3, y, &m_last.y);

    return m_valid;
}

bool RecSave::save_t_3(const UCHAR* buf, const UCHAR* end) {

    const UCHAR* p = n_front('.', buf);
    UINT64 line_n = ATOI(p);

    p = n_backw('/', end-1);
    save_allele(*p);

    // map n1..n4 to n0 .. n3
    UINT64 n[4];
    const int n_last = m_type == 3 ? 3 : 2 ;
    for (int i = 0; i <= n_last; i++) {
        p = n_backw('_', p-2);
        n[i] = ATOI(p);
    }

    for (int i = n_last; i ; i--)
        rarely_if (n[i] != m_last.n[i]) {
            put_i(0, -2-i);
            put_i(1, n[i], &(m_last.n[i]));
            stats.exceptions++;
        }

    put_i(0, line_n, &m_last.line_n);
    put_i(2, n[0], &m_last.n[0]);

    return m_valid;
}

bool RecSave::save_t_6(const UCHAR* buf, const UCHAR* end) {

    const UCHAR* p = n_front('.', buf);
    UINT64 line_n = ATOI(p);

    p = n_backw('_', end-1);
    UINT64 n1 = ATOI(p);

    p = n_backw('_', p-2);
    UINT64 n2 = ATOI(p);

    rarely_if(n2 != m_last.n[2]) {
        put_i(0, -2);
        put_i(2, n2, &(m_last.n[2]));
        stats.exceptions++;
    }
    put_i(0, line_n, &m_last.line_n);
    put_i(1, n1, &m_last.n[1]);

    return m_valid;
}

bool RecSave::save_t_5(const UCHAR* buf, const UCHAR* end) {
    const UCHAR* p = n_front('.', buf);
    UINT64 line_n = ATOI(p);

    p = n_backw('/', end-1);
    save_allele(*p);

    p = n_backw(' ', p-2);
    UINT64 line_2 = ATOI(p);

    rarely_if ((m_last.n[0] == 0) !=
               (line_n == line_2)) {
        m_last.n[0] = (line_n != line_2);
        put_i(0, (line_n == line_2) ? -2 : -3);
    }
    
    put_i(0, line_n, &m_last.line_n);

    rarely_if (m_last.n[0]) {
        put_i(2, line_2, &line_n);
    }

    return m_valid;
}

#undef ATOI

bool RecSave::save_2(const UCHAR* buf, const UCHAR* end) {

    rarely_if(not m_type )
        determine_rec_type(buf, end);

    m_last.rec_count ++;
    switch(m_type) {
    case 6 : return save_t_6(buf, end);
    case 5 : return save_t_5(buf, end);
    case 4 :
    case 3 : return save_t_3(buf, end);
    case 7:
    case 8:
    case 2 :                    // TODO: verify multiplexed sample index number
    case 1 : return save_t_1(buf, end);
    default: ;
    }
    croak("unexpected record type");
    return false;
}

// Load

void RecLoad::determine_ids(int size) {
    for (int i = 0 ; i < size ; i++)
        if (not m_ids[i] or
            not m_ids[i][0]) 
            croak("missing information for format %d", m_type);
        else 
            m_len[i] = strlen(m_ids[i]);
}

RecLoad::RecLoad() {
    m_valid = true;
    range_init();

    filer = new FilerLoad(conf.open_r("rec"), &m_valid);
    assert(filer);
    rcoder.init(filer);

    filerx = new FilerLoad(conf.open_r("rec.x"), &m_valid);
    assert(filerx);
    rcoderx.init(filerx);

    BZERO(m_ids);
    BZERO(m_last);
    m_type = atoi(conf.get_info("rec.type"));
    if (m_type < 1 or m_type > 8)
        croak("error: unknown format type: %i", m_type);

    m_ids[0] = conf.get_info("rec.in");
    m_ids[1] = conf.get_info("rec.id");
    determine_ids(2);


    // switch(m_type) {
    // case 1:
    //     m_ids[0] = conf.get_info("rec.1.in");
    //     m_ids[1] = conf.get_info("rec.1.id");
    //     determine_ids(2);
    //     break;
    // 
    // case 7:
    //     m_ids[0] = conf.get_info("rec.7.in");
    //     m_ids[1] = conf.get_info("rec.7.id");
    //     determine_ids(2);
    //     break;
    // 
    // case 8:
    // case 2:
    //     m_ids[0] = conf.get_info("rec.2.in");
    //     m_ids[1] = conf.get_info("rec.2.id");
    //     m_ids[2] = conf.get_info("rec.2.is"); 
    //     determine_ids(3);
    //     break;
    //     
    // case 3:
    // case 4:
    //     m_ids[0] = conf.get_info("rec.3.in");
    //     m_ids[1] = conf.get_info("rec.3.id");
    //     determine_ids(2);
    //     break;
    // 
    // case 5:
    //     m_ids[0] = conf.get_info("rec.5.in");
    //     determine_ids(1);
    //     break;
    // 
    // case 6:
    // 
    // 
    // default: 
    //     croak("unexpected record type");
    // }

    getx_u(0, &m_last.rec_count_tag);
}

RecLoad::~RecLoad() {
    rcoder.done();
    rcoderx.done();
}

void RecLoad::update() {
    int sanity = ET_END + 10;
    while (m_last.rec_count_tag == m_last.rec_count) {
        if (not -- sanity)
            croak("RecLoad: illegal exceptions file");
        exception_t type = (exception_t) getx_u(1);
        switch (type) {
        case ET_v0: getx_i(2, &m_last.n[0]); break;
        case ET_v1: getx_i(3, &m_last.n[1]); break;
        case ET_v2: getx_i(4, &m_last.n[2]); break;
        case ET_v3: getx_i(5, &m_last.n[3]); break;
        case ET_v9: getx_i(9, &m_last.n[9]); break;
        case ET_al: m_last.alal = getx_i(6); break;
        case ET_iseq: num_to_is(getx_u(7), (char*)m_ids[2]); break;
        case ET_END: m_last.rec_count_tag = 0; return;
        }
        getx_u(0, &m_last.rec_count_tag);        
    }
}

size_t RecLoad::load_t_1(UCHAR* buf) {
    // @SRR043409.575 61ED1AAXX100429:3:1:1039:1223/1        

    // long long gap = get_i(0);
    // while (RARELY(gap <= 0)) {
    //     switch (gap) {
    //     case  0  : return 0;
    //     case -1: m_last.alal = get_u(1); break;
    //     case -120: num_to_is(get_u(9), (char*)m_ids[2], m_len[2]); break;
    //     case -121: get_i(4, &(m_last.n[0])); break;
    //     case -122: get_i(5, &(m_last.n[1])); break;
    //     default: croak("REC: bad line gap value: %lld", gap);
    //     }
    //     gap = get_i(0);
    // }
    // 
    // m_last.line_n += gap;
    update();
    get_u(0, &m_last.line_n);
    get_i(2, &m_last.n[2]);
    get_i(3, &m_last.n[3]);

    size_t count =
        m_type == 1 ? 
        sprintf((char*)buf, "%s.%lld %s:%llu:%llu:%llu:%llu/%c",
                m_ids[0],
                m_last.line_n,
                m_ids[1],
                m_last.n[0],
                m_last.n[1],
                m_last.n[2],
                m_last.n[3],
                m_last.alal
                ) :
        m_type == 2 ?
        sprintf((char*)buf, "%s.%lld %s:%llu:%llu:%llu:%llu#%s/%c",
                m_ids[0],
                m_last.line_n,
                m_ids[1],
                m_last.n[0],
                m_last.n[1],
                m_last.n[2],
                m_last.n[3],
                m_ids[2],
                m_last.alal
                ) :
        m_type == 7 ?
        sprintf((char*)buf, "%s.%lld %s:%llu:%llu:%llu:%llu length=%llu",
                m_ids[0],
                m_last.line_n,
                m_ids[1],
                m_last.n[0],
                m_last.n[1],
                m_last.n[2],
                m_last.n[3],
                m_last.n[9]
                ) :
        m_type == 8 ?
        sprintf((char*)buf, "%s.%lld %s:%llu:%llu:%llu:%llu#%llu/%c",
                m_ids[0],
                m_last.line_n,
                m_ids[1],
                m_last.n[0],
                m_last.n[1],
                m_last.n[2],
                m_last.n[3],
                m_last.n[9],
                m_last.alal
                ) :
        0 ;


    return m_valid ? count : 0;

    // No significant speed difference. Switch back tp sprintf?

    // UCHAR* p = buf;
    // p = (UCHAR*) mempcpy(p, m_ids[0], m_len[0]);
    // *(p++) = '.';
    // p = pnum(p, m_last.index);
    // *(p++) = ' ';
    // p = (UCHAR*) mempcpy(p, m_ids[1], m_len[1]);
    // *(p++) = ':';
    // p = pnum(p, m_last.tile);
    // *(p++) = ':';
    // p = pnum(p, x);
    // *(p++) = ':';
    // p = pnum(p, y);        
    // *(p++) = '/';
    // p = pnum(p, pair_mem);
    // return m_valid ? p-buf : 0;
}

size_t RecLoad::load_t_3(UCHAR* buf) {
    long long gap = get_i(0);
    while (RARELY(gap <= 0)) {
        if      (gap == 0) return 0;
        else if (gap == -1) 
            m_last.alal = get_u(1);
        else if (gap == -2 or gap < -5)
            croak("REC: bad gap value: %d", gap);
        else
            get_i(1, &(m_last.n[0-gap-2]));
        gap = get_i(0);
    }

    m_last.line_n += gap;
    get_i(2, &m_last.n[0]);

    size_t count =
        m_type == 3 ?
        sprintf((char*) buf, "%s.%lld %s_%llu_%llu_%llu_%llu/%c",
                m_ids[0],
                m_last.line_n,
                m_ids[1],
                m_last.n[3],
                m_last.n[2],
                m_last.n[1],
                m_last.n[0],
                m_last.alal
                ) :
        sprintf((char*) buf, "%s.%lld %s_%llu_%llu_%llu/%c",
                m_ids[0],
                m_last.line_n,
                m_ids[1],
                m_last.n[2],
                m_last.n[1],
                m_last.n[0],
                m_last.alal
                ) ;
        
    return m_valid ? count : 0;
}

size_t RecLoad::load_t_5(UCHAR* buf) {

    long long gap = get_i(0);
    while (RARELY(gap <= 0)) {
        switch (gap) {
        case  0: return 0;
        case -1: m_last.alal = get_u(1); break;
        case -2: m_last.n[0] = 0; break;
        case -3: m_last.n[0] = 1; break;
        default: croak("REC: bad line gap value: %lld", gap);
        }
        gap = get_i(0);
    }

    m_last.line_n += gap;
    UINT64 line_2 = m_last.line_n;
    rarely_if (m_last.n[0])
        get_i(2, &line_2);

    size_t count =
        sprintf((char*)buf, "%s.%lld %lld/%c",
                m_ids[0],
                m_last.line_n,
                line_2,
                m_last.alal
                );

    return m_valid ? count : 0;
}

size_t RecLoad::load_t_6(UCHAR* buf) {
    long long gap = get_i(0);
    while (RARELY(gap <= 0)) {
        if      (gap == 0) return 0;
        else if (gap == -2)
            get_i(2, &(m_last.n[2]));
        else
            croak("REC: bad gap value: %d", gap);

        gap = get_i(0);
    }

    m_last.line_n += gap;
    get_i(1, &m_last.n[1]);

    size_t count =
        sprintf((char*) buf, "%s.%lld %s_%llu_%llu",
                m_ids[0],
                m_last.line_n,
                m_ids[1],
                m_last.n[2],
                m_last.n[1]
                );
        
    return m_valid ? count : 0;
}

size_t RecLoad::load_2(UCHAR* buf) {
    // get one record to buf
    if (not m_valid)
        return 0;

    m_last.rec_count ++;
    switch (m_type) {
    case 6:
        return load_t_6(buf);
    case 5 :
        return load_t_5(buf);
    case 4 : case 3 :
        return load_t_3(buf);
    case 1 : case 2 : case 7: case 8:
        return load_t_1(buf);
    default:;
    }
    croak("Illegal type: %d", m_type);
    return 0;
}
