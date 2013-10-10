//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "fq_recs.hpp"

#define MAX_LLINE 400           // TODO: assert max is enough at constructor

RecSave::RecSave(const Config* conf) {

    m_valid = true;
    pager  = NULL;
    pager2 = NULL;

    m_type = 0;
    m_conf = conf;

    BZERO(m_last);
    BZERO(stats);
    BZERO(m_ids);
}

void RecSave::pager_init() {
    DELETE( pager);
    DELETE( pager2);
    BZERO(m_last);
    pager  = new PagerSave16(m_conf->open_w("rec"));
    pager2 = new PagerSave02(m_conf->open_w("rec2"));
}

RecSave::~RecSave() {
    // flush();
    delete pager;
    delete pager2;

    fprintf(stderr, "::: REC %u oversized line gaps\n", stats.big_gaps);
}

void RecSave::putgap(UINT64 num, UINT64& old) {
    // for X Y values, we must negative gaps
    // I'll keep it local because it's hairy
    long long gap = num - old ;
    old = num ;

    // gap:
    // from negative 0xffe (0xf002 to  0xf000: save short
    // negative 0xfff (0xf001) means save int (32 bits) gap
    // read int:
    // negative 1 (0xffffffff) means save long (64 bits) gap
    likely_if (gap >  -0xffe and
               gap <= 0xf000) {
        pager->put16((UINT16) gap);
        return;
    }
    pager->put16(0xf001);
    if (gap >  0x7fffffff or
        gap < -0x80000000 ) {
        UINT32 gap32 = (UINT32) gap;
        pager->put16(0xffff & (gap32 >> 16));
        pager->put16(0xffff & (gap32));
        return;
    }
    pager->put16(0xffff);
    pager->put16(0xffff);
    croak("TODO: support oversized gap: %lld: ", gap);
}

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
        m_conf->set_info("rec.1.in", m_ids[0]);
        m_conf->set_info("rec.1.id", m_ids[1]);
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
        m_conf->set_info("rec.2.in", m_ids[0]);
        m_conf->set_info("rec.2.id", m_ids[1]);
        m_conf->set_info("rec.2.is", m_ids[2]);
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

        m_conf->set_info("rec.3.in" , m_ids[0]);
        m_conf->set_info("rec.3.id" , m_ids[1]);
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

        m_conf->set_info("rec.3.in" , m_ids[0]);
        m_conf->set_info("rec.3.id" , m_ids[1]);
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
        m_conf->set_info("rec.5.in", m_ids[0]);

        return 5;
    } while (0);
#undef  MUST
    croak("Cannot determine record type");
    return 0; 
} // get_rec_type

void RecSave::determine_rec_type(const UCHAR* buf, const UCHAR* end) {

    const char* first = strndup((const char*)buf, end-buf);
    m_conf->set_info("rec.first", first);

    m_type = get_rec_type(first) ;
    if (not m_type)
        croak("Illegal record type: %u\n", m_type);

    m_conf->set_info("rec.type", m_type);
    m_conf->save_info();

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

#define ATOI(X) atoll((const char*)X)

bool RecSave::is_allele(char a) {

    if (a == '1')
        return false;

    if (m_last.alal != a) {
        m_last.alal  = a;
        pager->put16(0xfff0);
        pager->put16(a);
    }   return true;
}

bool RecSave::save_1(const UCHAR* buf, const UCHAR* end, bool gentype) {
    // @SRR043409.575 61ED1AAXX100429:3:1:1039:1223/1
    // @SRR043409.578 61ED1AAXX100429:3:1:1040:4755/1
    const UCHAR* p = n_front('.', buf);
    UINT64 line_n = ATOI(p);

    p = n_backw('/', end-1);
    bool is_alal = is_allele(*p);

    if (m_type == 2) {
        // Also has sequent number
        const UCHAR* end = p-1;
        p = backto('#', end-1, end-16);
        assert(end-p == m_len[2]);
        if (strncmp((const char*)p, m_ids[2], m_len[2])) {
            pager->put16(0xfffc);
            UINT64 iseq_num = is_to_num(p, m_len[2]);
            pager->putgap(iseq_num );
            num_to_is(iseq_num, (char*)m_ids[2], m_len[2]);
        }
    }

    p = n_backw(':', p-2);
    UINT64 y = ATOI(p);

    p = n_backw(':', p-2);
    UINT64 x = ATOI(p);

    p = n_backw(':', p-2);
    UINT64 t = ATOI(p);

    if (t != m_last.tile) {
        pager->put16(0xfffe);
        putgap(t, m_last.tile);
    }

    // UINT64 line_g = line_n- m_last.index;
    // m_last.index = line_n;
    // putgap(line_g);
    if (line_n < m_last.index)
        croak("Sanity failure - decrising line number: %llu", line_n);
    pager->putgap(line_n -  m_last.index); // first item
    m_last.index = line_n ;

    putgap(x, m_last.x);
    putgap(y, m_last.y);

    pager2->put02((gentype ? 2 : 0) +
                  (is_alal ? 1 : 0));

    return m_valid;
}

bool RecSave::save_3(const UCHAR* buf, const UCHAR* end, bool gentype) {

    const UCHAR* p = n_front('.', buf);
    UINT64 line_n = ATOI(p);

    p = n_backw('/', end-1);
    bool is_alal = is_allele(*p);

    // map n1..n4 to n0 .. n3
    UINT64 n[4];
    const int n_last = m_type == 3 ? 3 : 2 ;
    for (int i = 0; i <= n_last; i++) {
        p = n_backw('_', p-2);
        n[i] = ATOI(p);
    }

    for (int i = n_last; i ; i--)
        if (n[i] != m_last.n[i]) {
            pager->put16(0xffff-i);
            pager->putgap(n[i]);
            m_last.n[i] = n[i];
        }

    rarely_if (line_n < m_last.index or
               n[0] < m_last.n[0])
        croak("Sanity failure - decrising line/n[0] number: %llu", line_n);

    pager->putgap(line_n - m_last.index); // first item
    m_last.index = line_n ;

    pager->putgap(n[0]);

    pager2->put02((gentype ? 2 : 0) +
                  (is_alal ? 1 : 0));
    
    return m_valid;
}

bool RecSave::save_5(const UCHAR* buf, const UCHAR* end, bool gentype) {
    const UCHAR* p = n_front('.', buf);
    UINT64 line_n = ATOI(p);

    p = n_backw('/', end-1);
    bool is_alal = is_allele(*p);

    p = n_backw(' ', p-2);
    UINT64 line_2 = ATOI(p);

    rarely_if ((m_last.n[0] == 0) !=
               (line_n == line_2)) {
        m_last.n[0] = (line_n != line_2);
        pager->put16(0xfffd + m_last.n[0]);
    }
    
    pager->putgap(line_n - m_last.index); // first item
    m_last.index = line_n ;

    rarely_if (m_last.n[0]) {
        putgap(line_2, line_n);
    }

    pager2->put02((gentype ? 2 : 0) +
                  (is_alal ? 1 : 0));

    return m_valid;
}

#undef ATOI

bool RecSave::save(const UCHAR* buf, const UCHAR* end, bool gentype) {

    rarely_if(not m_type )
        determine_rec_type(buf, end);
    rarely_if(not pager)
        pager_init();

    switch(m_type) {
    case 5:  return save_5(buf, end, gentype);
    case 4 :
    case 3 : return save_3(buf, end, gentype);
    case 2 :                    // TODO: verify multiplexed sample index number
    case 1 : return save_1(buf, end, gentype);
    default: ;
    }
    croak("unexpected record type");
    return false;
}

// Load

void RecLoad::determine_ids(int size) {
    for (int i = 0 ; i < size ; i++)
        if (not (m_ids[i] and
                 m_ids[i][0])) 
            croak("missing information for format %d", m_type);
        else 
            m_len[i] = strlen(m_ids[i]);
}

RecLoad::RecLoad(const Config* conf) {
    m_valid = true;
    pager  = new PagerLoad16(conf->open_r("rec") , &m_valid);
    pager2 = new PagerLoad02(conf->open_r("rec2"), &m_valid);

    BZERO(m_ids);
    BZERO(m_last);
    m_type = atoi(conf->get_info("rec.type"));

    switch(m_type) {
    case 1:
        m_ids[0] = conf->get_info("rec.1.in");
        m_ids[1] = conf->get_info("rec.1.id");
        determine_ids(2);
        break;

    case 2:
        m_ids[0] = conf->get_info("rec.2.in");
        m_ids[1] = conf->get_info("rec.2.id");
        m_ids[2] = conf->get_info("rec.2.is"); 
        determine_ids(3);
        break;
        
    case 3:
    case 4:
        m_ids[0] = conf->get_info("rec.3.in");
        m_ids[1] = conf->get_info("rec.3.id");
        determine_ids(2);
        break;

    case 5:
        m_ids[0] = conf->get_info("rec.5.in");
        determine_ids(1);
        break;

    default: 
        croak("unexpected record type");
    }
}

RecLoad::~RecLoad() {
    delete pager;
    delete pager2;
}

UINT64 RecLoad::getgap(UINT64& old) {

    UINT64 gap = pager->get16();
    if (gap != 0xf001) {
        old += (gap <= 0xf000) ? gap : (short int) gap;
        return old;
    }
    gap  = pager->get16();
    gap <<= 16;
    gap |= pager->get16();
    if (gap != 0xffffffff) {
        old += (int) gap;
        return old;
    }

    croak("TODO: support oversized gap: %lld: ", gap);

    old += gap;
    return old;
}

size_t RecLoad::load_1(UCHAR* buf, bool *gentype) {
    // @SRR043409.575 61ED1AAXX100429:3:1:1039:1223/1        

    UINT16 predict = pager->predict();
    rarely_if( predict == 0)
        return 0;
    rarely_if (predict >= 0xfff0 and predict != 0xffff)
        while (predict >= 0xfff0 and predict != 0xffff) {
            pager->get16();
            switch(predict) {
            case 0xfffc:
                num_to_is(pager->getgap(), (char*)m_ids[2], m_len[2]);
                break;
            case 0xfffe:
                getgap(m_last.tile);
                break;
            case 0xfff0:
                m_last.alal = 0xff & pager->get16();
                break;
            default:
                croak("unexpected exception: %u", predict);
            }
            predict = pager->predict();
        }

    m_last.index += pager->getgap();
    getgap(m_last.x);
    getgap(m_last.y);

    int ang = pager2->get02();
    bool is_alal = !! (ang & 1);
        *gentype = !! (ang & 2);

    size_t count =
        m_type == 1 ? 
        sprintf((char*)buf, "%s.%lld %s:%llu:%llu:%llu/%c",
                m_ids[0],
                m_last.index,
                m_ids[1],
                m_last.tile,
                m_last.x,
                m_last.y,
                (is_alal ? m_last.alal : '1')
                ) :

        sprintf((char*)buf, "%s.%lld %s:%llu:%llu:%llu#%s/%c",
                m_ids[0],
                m_last.index,
                m_ids[1],
                m_last.tile,
                m_last.x,
                m_last.y,
                m_ids[2],
                (is_alal ? m_last.alal : '1')
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

size_t RecLoad::load_3(UCHAR* buf, bool *gentype) {

    UINT16 predict = pager->predict();
    rarely_if( predict == 0)
        return 0;
    rarely_if (predict >= 0xfff0 and predict != 0xffff)
        while (predict >= 0xfff0 and predict != 0xffff) {
            pager->get16();
            switch(predict) {
            case 0xfffe:
            case 0xfffd:
            case 0xfffc: 
                m_last.n[0xffff-predict] = pager->getgap();
                break;

            case 0xfff0:
                m_last.alal = 0xff & pager->get16();
                break;
            default:
                croak("unexpected exception: %u", predict);
            }
            predict = pager->predict();
        }

    m_last.index += pager->getgap();
    m_last.n[0]   = pager->getgap();

    int ang = pager2->get02();
    bool is_alal = !! (ang & 1);
    /**/*gentype = !! (ang & 2);
    size_t count =
        m_type == 3 ?
        sprintf((char*) buf, "%s.%lld %s_%llu_%llu_%llu_%llu/%c",
                m_ids[0],
                m_last.index,
                m_ids[1],
                m_last.n[3],
                m_last.n[2],
                m_last.n[1],
                m_last.n[0],
                (is_alal ? m_last.alal : '1')
                ) :
        sprintf((char*) buf, "%s.%lld %s_%llu_%llu_%llu/%c",
                m_ids[0],
                m_last.index,
                m_ids[1],
                m_last.n[2],
                m_last.n[1],
                m_last.n[0],
                (is_alal ? m_last.alal : '1')
                ) ;
        
    return m_valid ? count : 0;
}

size_t RecLoad::load_5(UCHAR* buf, bool *gentype) {

    UINT16 predict = pager->predict();
    rarely_if( predict == 0)
        return 0;
    rarely_if (predict >= 0xfff0 and predict != 0xffff)
        while (predict >= 0xfff0 and predict != 0xffff) {
            pager->get16();
            switch(predict) {
            case 0xfffe:
            case 0xfffd:
                m_last.n[0] = (predict == 0xfffe);
                break;

            case 0xfff0:
                m_last.alal = 0xff & pager->get16();
                break;
            default:
                croak("unexpected exception: %u", predict);
            }
            predict = pager->predict();
        }

    m_last.index += pager->getgap();
    UINT64 line_2 = m_last.index;
    rarely_if(m_last.n[0])
        getgap(line_2);

    int ang = pager2->get02();
    bool is_alal = !! (ang & 1);
        *gentype = !! (ang & 2);

    size_t count =
        sprintf((char*)buf, "%s.%lld %lld/%c",
                m_ids[0],
                m_last.index,
                line_2,
                (is_alal ? m_last.alal : '1')
                );

    return m_valid ? count : 0;
}

size_t RecLoad::load(UCHAR* buf, bool *gentype) {
    // get one record to buf
    // size_t count = 0;
    if (not m_valid)
        return 0;

    switch (m_type) {
    case 5 : return load_5(buf, gentype);
    case 4 :
    case 3 : return load_3(buf, gentype);
    case 2 : 
    case 1 : return load_1(buf, gentype);
    default:;
    }
    croak("Illegal type: %d", m_type);
    return 0;
}