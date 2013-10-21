//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "recs.hpp"

#define MAX_LLINE 400           // TODO: assert max is enough at constructor


void RecBase::range_init() {
    BZERO(ranger);
}
void RecBase::puter(int i, int j, UCHAR c) {
    ranger[i][j].put(&rcoder, c);
}

UINT64 RecBase::geter(int i, int j) {
    return ranger[i][j].get(&rcoder);
}

void RecBase::put_i(int i, long long num, UINT64 * old) {
    if (old) {
        // assert(num >= *old); what's worse?
        long long t = num - *old;
        *old = num;
        num = t;
    }
    likely_if( num >= -0x80+3 and
               num <= 0x7f) {
        puter(i, 0, num & 0xff);
    }
    else if (num >= -0x8000 and
             num <=  0x7fff) {
        puter(i, 0, 0x80);
        puter(i, 1, 0xff & num);
        puter(i, 1, 0xff & (num>>8));
    }
    else if (num >= -0x80000000LL and
             num <=  0x7fffffffLL ) {
        puter(i, 0, 0x81);
        for (int shift = 0; shift < 32; shift+=8)
            puter(i, 2, 0xff&(num>>shift));
        stats.big_i++;
    }
    else {
        puter(i, 0, 0x82);
        for (int shift = 0; shift < 64; shift+=8)
            puter(i, 3, 0xff&(num>>shift));
        stats.big_ill ++;
    }
}

long long RecBase::get_i(int i, UINT64* old) {
    long long num = geter(i, 0);
    rarely_if(num == 0x80 ) {
        num  = geter(i, 1);
        num |= geter(i, 1) << 8;
        num  = (short) num;
    }
    else rarely_if(num == 0x81) {
        num  = geter(i, 2);
        num |= geter(i, 2) << 8;
        num |= geter(i, 2) << 16;
        num |= geter(i, 2) << 24;
        num  = (int) num;
    }
    else rarely_if(num == 0x82) {
        num = 0;
        for (int shift = 0; shift < 64; shift+=8)
            num |= geter(i, 3) << shift;
    }
    else if (0x80&num)
        num = (char)num;

    return
        ( old) ?
        (*old += num) :
        num ;
}

void RecBase::put_u(int i, UINT64 num, UINT64* old) {
    if (old) {
        // assert(num >= *old); what's worse?
        UINT64 t = num - *old;
        *old = num;
        num = t;
    }
    likely_if(num <= 0x7f) {
        puter(i, 0, 0xff & num);
        return;
    }
    likely_if (num < 0x7ffe) {
        puter(i, 0, 0xff & (0x80 | (num>>8)));
        puter(i, 1, 0xff & num);
        return;
    }
    puter(i, 0, 0xff);
    if (num < 1ULL<<32) {
        puter(i, 1, 0xfe);
        for (int shift=0; shift < 32; shift+=8)
            puter(i, 2, 0xff & (num>>shift));
        stats.big_u ++;
    }
    else {
        puter(i, 1, 0xff);
        for (int shift=0; shift < 64; shift+=8)
            puter(i, 3, 0xff & (num>>shift));
        stats.big_ull ++;
    }
}

UINT64 RecBase::get_u(int i, UINT64* old) {

    UINT64 num = geter(i, 0);
    rarely_if(num > 0x7f) {

        num <<= 8;
        num  |= geter(i, 1);
        likely_if(num < 0xfffe)
            num &= 0x7fff;
        else {
            bool _4 = num == 0xfffe;
            num = 0;
            for (int shift=0; shift < (_4 ? 32 : 64); shift+=8) {
                UINT64 c = geter(i, _4 ? 2 : 3);
                num  |= (c<<shift);
            }
        }
    }
    return
        ( old) ?
        (*old += num) :
        num ;
}


RecSave::RecSave() {

    m_valid = true;
    // pager  = NULL;
    // pager2 = NULL;

    m_type = 0;
    // m_conf = conf;

    BZERO(m_last);
    BZERO(stats);
    BZERO(m_ids);
    
    filer = new FilerSave(conf.open_w("rec"));
    assert(filer);
    rcoder.init(filer);
    range_init();
}

// void RecSave::pager_init() {
//     assert(0);                  // TODO
//     // DELETE( pager);
//     // DELETE( pager2);
//     // BZERO(m_last);
//     // pager  = new PagerSave16(m_conf->open_w("rec"));
//     // pager2 = new PagerSave02(m_conf->open_w("rec2"));
// }

RecSave::~RecSave() {
    // flush();
    // delete pager;
    // delete pager2;
    // udpate(UT_END, -1ULL);
    rcoder.done();
    DELETE(filer);
    fprintf(stderr, "::: REC big u/ull/i/ill: %u/%u/%u/%u ex:%u\n",
            stats.big_u, stats.big_ull, stats.big_i, stats.big_ill, stats.exceptions);
}

// void RecSave::putgap(UINT64 num, UINT64& old) {
//     // for X Y values, we must negative gaps
//     // I'll keep it local because it's hairy
//     long long gap = num - old ;
//     old = num ;
// 
//     // gap:
//     // from negative 0xffe (0xf002 to  0xf000: save short
//     // negative 0xfff (0xf001) means save int (32 bits) gap
//     // read int:
//     // negative 1 (0xffffffff) means save long (64 bits) gap
//     likely_if (gap >  -0xffe and
//                gap <= 0xf000) {
//         pager->put16((UINT16) gap);
//         return;
//     }
//     pager->put16(0xf001);
//     if (gap >  0x7fffffff or
//         gap < -0x80000000 ) {
//         UINT32 gap32 = (UINT32) gap;
//         pager->put16(0xffff & (gap32 >> 16));
//         pager->put16(0xffff & (gap32));
//         return;
//     }
//     pager->put16(0xffff);
//     pager->put16(0xffff);
//     croak("TODO: support oversized gap: %lld: ", gap);
// }

// void RecSave::putgap(UINT64 gap) {
// 
//     likely_if (gap < 0xfff0) {
//         pager->put16(gap);
//         return;
//     }
// 
//     if(gap > 0xffffffff)     // TODO: support this case (easy implementation)
//         croak ("RECS : gap bigger than 4G - sanity failure");
// 
//     stats.big_gaps ++;
//     pager->put16(0xffff);
//     pager->put16(0xffff & (gap >> 16));
//     pager->put16(0xffff & (gap));
// }

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
        // @SRR043409.575 61ED1AAXX100429:3:1:1039:1223/1
        const char* p_l = index(start, '.');
        MUST(p_l);
        const char* p_s = index(p_l, ' '  );
        MUST(p_s++);
        MUST(be_digits(p_l+1, p_s-1)); // this part matches qr/\..*?\s/

        const char* p_a = backto('/', end-1, end-3);
        MUST(be_digits(p_a, end));

        const char* p_y = backto(':', p_a-2, p_a - 10);
        MUST(be_digits(p_y, p_a-1));

        const char* p_x = backto(':', p_y-2, p_y - 10);
        MUST(be_digits(p_x, p_y-1));

        const char* p_t = backto(':', p_x-2, p_x - 10);
        MUST(be_digits(p_t, p_x-1));

        MUST(p_t > p_s and p_t - p_s > 4);

        // congratulations
        m_ids[0] = strndup(start, p_l-start);
        m_ids[1] = strndup(p_s  , p_t-p_s-1);
        conf.set_info("rec.1.in", m_ids[0]);
        conf.set_info("rec.1.id", m_ids[1]);
        return 1;
    } while (0);

    do {
        // @ERR042980.2 FCC01NKACXX:8:1101:1727:1997#CGATGTAT/1
        const char* p_l = index(start, '.');
        MUST(p_l);
        const char* p_s = index(p_l, ' '  );
        MUST(p_s++);
        MUST(be_digits(p_l+1, p_s-1)); // this part matches qr/\..*?\s/

        const char* p_a = backto('/', end-1, end-3);
        MUST(be_digits(p_a, end));

        const char* p_m = backto('#', p_a-2, p_a - 30);
        MUST(p_m);

        const char* p_y = backto(':', p_m-2, p_m - 10);
        MUST(be_digits(p_y, p_m-1));

        const char* p_x = backto(':', p_y-2, p_y - 10);
        MUST(be_digits(p_x, p_y-1));

        const char* p_t = backto(':', p_x-2, p_x - 10);
        MUST(be_digits(p_t, p_x-1));

        MUST(p_t > p_s and p_t - p_s > 4);

        // congratulations
        m_ids[0] = strndup(start, p_l-start);
        m_ids[1] = strndup(p_s  , p_t-p_s-1);
        m_ids[2] = strndup(p_m  , p_a-p_m-1);
        conf.set_info("rec.2.in", m_ids[0]);
        conf.set_info("rec.2.id", m_ids[1]);
        conf.set_info("rec.2.is", m_ids[2]);
        return 2;
    } while (0);

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
        // m_ids[2] = strndup(p_2  , p_1-p_2-1);
        // m_ids[3] = strndup(p_3  , p_2-p_3-1);
        // m_ids[4] = strndup(p_4  , p_3-p_4-1);

        conf.set_info("rec.3.in" , m_ids[0]);
        conf.set_info("rec.3.id" , m_ids[1]);
        // m_conf->set_info("rec.3.f.2", m_ids[2]);
        // m_conf->set_info("rec.3.f.3", m_ids[3]);
        // m_conf->set_info("rec.3.f.4", m_ids[4]);

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

        conf.set_info("rec.3.in" , m_ids[0]);
        conf.set_info("rec.3.id" , m_ids[1]);
        // m_conf->set_info("rec.3.f.2", m_ids[2]);
        // m_conf->set_info("rec.3.f.3", m_ids[3]);

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
        // while(*(p_r) == ' ') p_r++;  -- assumes exactly one space
        const char* p_a = index(p_r, '/');
        MUST(p_r);
        MUST(be_digits(p_r, p_a));
        MUST(is_digit(*(p_a+1)));
        MUST(   end == (p_a+2));

        m_ids[0] = strndup(start, p_l-start);
        conf.set_info("rec.5.in", m_ids[0]);

        return 5;
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
    for (int i = 0 ; m_ids[i] and i < 10; i++)
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

static void num_to_is (UINT64 num, char* is, int len) {

    for (int i = len-1; i >= 0 ; i --) {
        is[i] = "ACGT"[ num & 3];
        num >>= 2;
    }
}

// void RecSave::update(UpdateType type, UINT64 val) {
//     // 0 is for updates
//     put_u(0, rec_count, &rec_count_tag);
//     put_u(0, type);
//     put_u(0, val);
//     switch (type) {
//     case UT_ALLELE: m_last.alal = val; break;
//     case UT_BASES : num_to_is(val, (char*)m_ids[2], m_len[2]); break;
//     case UT_TYLE  : m_last.tile = val; break;
//     case UT_END: break;
//     }
// }

#define ATOI(X) atoll((const char*)X)

// bool RecSave::is_allele(char a) {
// 
//     if (a == '1')
//         return false;
// 
//     if (m_last.alal != a) {
//         m_last.alal  = a;
//         put_i(0, -1);
//         put_u(1,  a);
//         // pager->put16(0xfff0);
//         // pager->put16(a);
//     }   return true;
// }

void RecSave::save_allele(char a) {
    if (m_last.alal == a)
        return;
    m_last.alal  = a;
    put_i(0, -1);
    put_u(1,  a);
    stats.exceptions++;
}

bool RecSave::save_t_1(const UCHAR* buf, const UCHAR* end) {
    // @SRR043409.575 61ED1AAXX100429:3:1:1039:1223/1
    // @SRR043409.578 61ED1AAXX100429:3:1:1040:4755/1
    const UCHAR* p = n_front('.', buf);
    UINT64 line_n = ATOI(p);

    p = n_backw('/', end-1);
    // bool is_alal = is_allele(*p);
    save_allele(*p);

    if (m_type == 2) {
        // Also has sequent number
        const UCHAR* end = p-1;
        p = backto('#', end-1, end-16);
        assert(end-p == m_len[2]);
        if (strncmp((const char*)p, m_ids[2], m_len[2])) {
            UINT64 iseq_num = is_to_num(p, m_len[2]);
            num_to_is(iseq_num, (char*)m_ids[2], m_len[2]); 
            put_i(0, -2);
            put_u(1, iseq_num);
            stats.exceptions++;
            // pager->put16(0xfffc);
            // pager->putgap(iseq_num );
        }
    }

    p = n_backw(':', p-2);
    UINT64 y = ATOI(p);

    p = n_backw(':', p-2);
    UINT64 x = ATOI(p);

    p = n_backw(':', p-2);
    UINT64 t = ATOI(p);

    if (m_last.tile != t) {
        m_last.tile  = t;
        put_i(0, -3);
        put_u(1,  t);
        stats.exceptions++;
        // pager->put16(0xfffe);
        // putgap(t, m_last.tile);
    }

    // UINT64 line_g = line_n- m_last.index;

    // if (line_n < m_last.index)
    //     croak("Sanity failure - decrising line number: %llu", line_n);
    // pager->putgap(line_n -  m_last.index); // first item
    // m_last.index = line_n ;

    put_i(0, line_n, &m_last.line_n); // limited to 2G gaps (TODO?)

    // putgap(x, m_last.x);
    // putgap(y, m_last.y);
    put_i(2, x, &m_last.x);
    put_i(3, y, &m_last.y);

    // pager2->put02((gentype ? 2 : 0) +
    //               (is_alal ? 1 : 0));

    // put_i(4, (gentype ? 2 : 0) + (is_alal ? 1 : 0));
    return m_valid;
}

bool RecSave::save_t_3(const UCHAR* buf, const UCHAR* end) {

    const UCHAR* p = n_front('.', buf);
    UINT64 line_n = ATOI(p);

    p = n_backw('/', end-1);
    // bool is_alal = is_allele(*p);
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
            // pager->put16(0xffff-i);
            // pager->putgap(n[i]);
            // m_last.n[i] = n[i];
        }

    // rarely_if (line_n < m_last.index or
    //            n[0] < m_last.n[0])
    //     croak("Sanity failure - decrising line/n[0] number: %llu", line_n);
    // 
    // pager->putgap(line_n - m_last.index); // first item
    // m_last.index = line_n ;
    // pager->putgap(n[0]);
    put_i(0, line_n, &m_last.line_n);
    put_i(2, n[0], &m_last.n[0]);

    // pager2->put02((gentype ? 2 : 0) +
    //               (is_alal ? 1 : 0));
    
    return m_valid;
}

bool RecSave::save_t_5(const UCHAR* buf, const UCHAR* end) {
    const UCHAR* p = n_front('.', buf);
    UINT64 line_n = ATOI(p);

    p = n_backw('/', end-1);
    // bool is_alal = is_allele(*p);
    save_allele(*p);

    p = n_backw(' ', p-2);
    UINT64 line_2 = ATOI(p);

    rarely_if ((m_last.n[0] == 0) !=
               (line_n == line_2)) {
        m_last.n[0] = (line_n != line_2);
        // pager->put16(0xfffd + m_last.n[0]);
        put_i(0, (line_n == line_2) ? -2 : -3);
    }
    
    // pager->putgap(line_n - m_last.index); // first item
    // m_last.index = line_n ;
    put_i(0, line_n, &m_last.line_n);

    rarely_if (m_last.n[0]) {
        // putgap(line_2, line_n);
        put_i(2, line_2, &line_n);
    }

    // pager2->put02((gentype ? 2 : 0) +
    //               (is_alal ? 1 : 0));

    return m_valid;
}

#undef ATOI

bool RecSave::save_2(const UCHAR* buf, const UCHAR* end) {

    rarely_if(not m_type )
        determine_rec_type(buf, end);
    // rarely_if(not pager)
    //     pager_init();

    m_last.rec_count ++;
    switch(m_type) {
    case 5:  return save_t_5(buf, end);
    case 4 :
    case 3 : return save_t_3(buf, end);
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
    // pager  = new PagerLoad16(conf->open_r("rec") , &m_valid);
    // pager2 = new PagerLoad02(conf->open_r("rec2"), &m_valid);
    filer = new FilerLoad(conf.open_r("rec"), &m_valid);
    assert(filer);
    rcoder.init(filer);
    range_init();

    BZERO(m_ids);
    BZERO(m_last);
    m_type = atoi(conf.get_info("rec.type"));

    switch(m_type) {
    case 1:
        m_ids[0] = conf.get_info("rec.1.in");
        m_ids[1] = conf.get_info("rec.1.id");
        determine_ids(2);
        break;

    case 2:
        m_ids[0] = conf.get_info("rec.2.in");
        m_ids[1] = conf.get_info("rec.2.id");
        m_ids[2] = conf.get_info("rec.2.is"); 
        determine_ids(3);
        break;
        
    case 3:
    case 4:
        m_ids[0] = conf.get_info("rec.3.in");
        m_ids[1] = conf.get_info("rec.3.id");
        determine_ids(2);
        break;

    case 5:
        m_ids[0] = conf.get_info("rec.5.in");
        determine_ids(1);
        break;

    default: 
        croak("unexpected record type");
    }
}

RecLoad::~RecLoad() {
    rcoder.done();
    // delete pager;
    // delete pager2;
}

// void RecLoad::update() {
//     int sanity = UT_END;
//     while (m_last.rec_count == m_last.rec_count_tag) {
//         if (not sanity --)
//             croak("RecLoad: illegal update item");
// 
//         UpdateType type = get_u(0);
//         switch(type) {
//         case UT_ALLELE:
//         }
//         get_u(0, &m_last.rec_count_tag);
//     }
// }

// UINT64 RecLoad::getgap(UINT64& old) {
// 
//     UINT64 gap = pager->get16();
//     if (gap != 0xf001) {
//         old += (gap <= 0xf000) ? gap : (short int) gap;
//         return old;
//     }
//     gap  = pager->get16();
//     gap <<= 16;
//     gap |= pager->get16();
//     if (gap != 0xffffffff) {
//         old += (int) gap;
//         return old;
//     }
// 
//     croak("TODO: support oversized gap: %lld: ", gap);
// 
//     old += gap;
//     return old;
// }

size_t RecLoad::load_t_1(UCHAR* buf) {
    // @SRR043409.575 61ED1AAXX100429:3:1:1039:1223/1        

    long long gap = get_i(0);
    while (RARELY(gap <= 0)) {
        switch (gap) {
        case  0: return 0;
        case -1: m_last.alal = get_u(1); break;
        case -2: num_to_is(get_u(1), (char*)m_ids[2], m_len[2]); break;
        case -3: m_last.tile = get_u(1); break;
        default: croak("REC: bad line gap value: %lld", gap);
        }
        gap = get_i(0);
    }

    // UINT16 predict = pager->predict();
    // rarely_if( predict == 0)
    //     return 0;
    // rarely_if (predict >= 0xfff0 and predict != 0xffff)
    //     while (predict >= 0xfff0 and predict != 0xffff) {
    //         pager->get16();
    //         switch(predict) {
    //         case 0xfffc:
    //             num_to_is(pager->getgap(), (char*)m_ids[2], m_len[2]);
    //             break;
    //         case 0xfffe:
    //             getgap(m_last.tile);
    //             break;
    //         case 0xfff0:
    //             m_last.alal = 0xff & pager->get16();
    //             break;
    //         default:
    //             croak("unexpected exception: %u", predict);
    //         }
    //         predict = pager->predict();
    //     }

    m_last.line_n += gap;
    // m_last.index += pager->getgap();
    // getgap(m_last.x);
    // getgap(m_last.y);
    get_i(2, &m_last.x);
    get_i(3, &m_last.y);

    // int ang = pager2->get02();
    // bool is_alal = !! (ang & 1);
    //     *gentype = !! (ang & 2);

    size_t count =
        m_type == 1 ? 
        sprintf((char*)buf, "%s.%lld %s:%llu:%llu:%llu/%c",
                m_ids[0],
                m_last.line_n,
                m_ids[1],
                m_last.tile,
                m_last.x,
                m_last.y,
                m_last.alal
                ) :

        sprintf((char*)buf, "%s.%lld %s:%llu:%llu:%llu#%s/%c",
                m_ids[0],
                m_last.line_n,
                m_ids[1],
                m_last.tile,
                m_last.x,
                m_last.y,
                m_ids[2],
                m_last.alal
                );


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

    // UINT16 predict = pager->predict();
    // rarely_if( predict == 0)
    //     return 0;
    // rarely_if (predict >= 0xfff0 and predict != 0xffff)
    //     while (predict >= 0xfff0 and predict != 0xffff) {
    //         pager->get16();
    //         switch(predict) {
    //         case 0xfffe:
    //         case 0xfffd:
    //         case 0xfffc: 
    //             m_last.n[0xffff-predict] = pager->getgap();
    //             break;
    // 
    //         case 0xfff0:
    //             m_last.alal = 0xff & pager->get16();
    //             break;
    //         default:
    //             croak("unexpected exception: %u", predict);
    //         }
    //         predict = pager->predict();
    //     }

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

    // m_last.index += pager->getgap();
    // m_last.n[0]   = pager->getgap();
    m_last.line_n += gap;
    get_i(2, &m_last.n[0]);

    // int ang = pager2->get02();
    // bool is_alal = !! (ang & 1);
    // /**/*gentype = !! (ang & 2);
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

    // UINT16 predict = pager->predict();
    // rarely_if( predict == 0)
    //     return 0;
    // rarely_if (predict >= 0xfff0 and predict != 0xffff)
    //     while (predict >= 0xfff0 and predict != 0xffff) {
    //         pager->get16();
    //         switch(predict) {
    //         case 0xfffe:
    //         case 0xfffd:
    //             m_last.n[0] = (predict == 0xfffe);
    //             break;
    // 
    //         case 0xfff0:
    //             m_last.alal = 0xff & pager->get16();
    //             break;
    //         default:
    //             croak("unexpected exception: %u", predict);
    //         }
    //         predict = pager->predict();
    //     }

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


    // m_last.index += pager->getgap();
    // UINT64 line_2 = m_last.index;
    // rarely_if(m_last.n[0])
    //     getgap(line_2);
    m_last.line_n += gap;
    UINT64 line_2 = m_last.line_n;
    rarely_if (m_last.n[0])
        get_i(2, &line_2);

    // int ang = pager2->get02();
    // bool is_alal = !! (ang & 1);
    //     *gentype = !! (ang & 2);

    size_t count =
        sprintf((char*)buf, "%s.%lld %lld/%c",
                m_ids[0],
                m_last.line_n,
                line_2,
                m_last.alal
                );

    return m_valid ? count : 0;
}

size_t RecLoad::load_2(UCHAR* buf) {
    // get one record to buf
    // size_t count = 0;
    if (not m_valid)
        return 0;

    m_last.rec_count ++;
    switch (m_type) {
    case 5 : return load_t_5(buf);
    case 4 :
    case 3 : return load_t_3(buf);
    case 2 : 
    case 1 : return load_t_1(buf);
    default:;
    }
    croak("Illegal type: %d", m_type);
    return 0;
}
