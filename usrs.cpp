//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

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

    m_valid  = true ;
    m_cur = m_end = m_rec_total = m_page_count = 0;
    m_llen = 0;
    m_solid = false;
    pager_x = NULL;

    m_in   = conf.file_usr();
    load_page();
}

UsrSave::~UsrSave(){
    delete pager_x;
    fprintf(stderr, "::: USR read %lu fastq records\n", m_rec_total);
}

void UsrSave::load_page() {

    size_t size  = m_end - m_cur ;
    assert(size <= PLL_STRT);
    size_t start = PLL_STRT - size;
    for (size_t i = 0 ; i < size; i++)
        m_buff[start+i] = m_buff[m_cur+i];
    
    size_t cnt = fread(&m_buff[PLL_STRT], 1, PLL_SIZE, m_in);
    m_cur  = start;
    m_end = PLL_STRT + cnt  ;

    if (m_cur == m_end) 
        m_valid = false;
    else
        m_page_count++;
}

void UsrSave::pager_init() {
    BZERO(m_last);
    DELETE(pager_x);
}

void UsrSave::update(exception_t type, UCHAR dat) {
    rarely_if(pager_x == NULL)
        pager_x = new PagerSave16(conf.open_w("usr.update"));
    assert(pager_x);
    pager_x->putgap(m_last.rec_count);
    pager_x->put16( (type << 8) | dat );
    switch (type) {
    case ET_LLEN     : m_llen = dat; break;
    case ET_SOLPF_GEN: m_last.solid_pf_gen = dat; break;
    case ET_SOLPF_QLT: m_last.solid_pf_qlt = dat; break;
    case ET_END: default : assert(0);
    }
}

void UsrSave::expect(UCHAR chr) {
    likely_if (m_buff[m_cur++] == chr)
        return;
    fprintf(stderr, "fastq file: expecting '%c', got '%c' after record %lu+%llu\n", chr, m_buff[m_cur-1], m_rec_total, m_last.rec_count);
    exit (1);
}

bool UsrSave::mid_rec_msg() const {
    fprintf(stderr, "fastq file: record seems truncated  after record %lu+%llu\n", m_rec_total, m_last.rec_count);
    exit (1);
}

void UsrSave::load_check() {
    if (m_cur+PLL_STRT < m_end)
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

bool UsrSave::get_record(UCHAR** rec, UCHAR** rec_end, UCHAR** gen, UCHAR** qlt) {
    
    load_check();

#define CHECK_OVERFLOW rarely_if (m_cur >= m_end) return mid_rec_msg()

    if (m_cur >= m_end) return m_valid = false;
    expect('@');
    (*rec) = &(m_buff[m_cur]);
    for (int sanity = MAX_ID_LLEN;
         --  sanity and m_buff[m_cur] != '\n';
         m_cur ++ );
    (*rec_end) = &(m_buff[m_cur]);

    CHECK_OVERFLOW ;

    expect('\n');

    rarely_if (not m_llen)      // TODO: call from constructor 
        determine_record();

    if (m_solid) {
        rarely_if (m_last.solid_pf_gen != m_buff[m_cur])
            update(ET_SOLPF_GEN, m_buff[m_cur]);

        m_cur++;
    }

    (*gen) = &(m_buff[m_cur]);
    const int gi = m_cur ;
    while (m_buff[m_cur] != '\n' and (m_cur-gi) < MAX_GN_LLEN)
        m_cur ++;
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
 
    (*qlt) = &(m_buff[m_cur]);
    m_cur += m_llen;
    CHECK_OVERFLOW;

    likely_if (m_cur < m_end)
        expect('\n');

    m_last.rec_count++;

    return true;

#undef  CHECK_OVERFLOW
}

UINT64 UsrSave::estimate_rec_limit() {
    int  i, cnt;
    for (i = m_cur, cnt=0;
         i < m_end and cnt < 4;
         i ++ )
        if (m_buff[i] == '\n')
            cnt ++;
    
    if (cnt < 4)
        croak("This usr file is too small");

    UINT64 limit = conf.partition.size / (i-m_cur);
    // if (limit < 500000)
    if (limit < 5000) // for debuging
        croak("This partition partition is too small (records limit=%lld)", limit);

    return limit;
}

int UsrSave::encode() {

    UINT32  sanity = conf.profiling ? 100000 : 1000000000;
    UCHAR *p_rec, *p_rec_end, *p_gen, *p_qlt;
    // bool gentype = 0;

    RecSave rec;
    GenSave gen;
    QltSave qlt;

    if (conf.partition.size) {
        assert(0);              // TODO
        // // TODO: unite cases
        // size_t recs_l = estimate_rec_limit();
        // m_conf->set_info("partition.size", m_conf->partition.size);
        // m_conf->set_info("partition.n_rec", recs_l);
        // PagerSave ppart(m_conf->open_w("part"));
        // ppart.put(0);          // version
        // bool valid = true;
        // assert(m_page_count);
        // while (valid) {
        //     UINT64 offs = ((m_page_count-1)*PLL_SIZE) + m_cur - PLL_STRT;
        //     m_conf->set_part_offs(offs);
        //     pager_init();
        //     rec.pager_init();
        //     gen.pager_init();
        //     qlt.filer_init();
        // 
        //     while (m_last.rec_count < recs_l and
        //            (valid = get_record(&p_rec, &p_rec_end, &p_gen, &p_qlt)) and
        //             ++ m_rec_total < sanity ) {
        //         gen.save(p_gen, p_qlt, m_llen);
        //         rec.save(p_rec, p_rec_end);
        //         qlt.save(p_qlt, m_llen);
        //     }
        //     ppart.put(offs);
        //     ppart.put(m_last.rec_count);
        // }
    }
    else {

        switch (conf.level) {

        case 1:
            while(get_record(&p_rec, &p_rec_end, &p_gen, &p_qlt) and
                  ++ m_rec_total < sanity ) {
                gen.save_1(p_gen, p_qlt, m_llen);
                rec.save_1(p_rec, p_rec_end);
                qlt.save_1(p_qlt, m_llen);
            } break;
        case 2: default:
            while(get_record(&p_rec, &p_rec_end, &p_gen, &p_qlt) and
                  ++ m_rec_total < sanity ) {

                gen.save_2(p_gen, p_qlt, m_llen);
                rec.save_2(p_rec, p_rec_end);
                qlt.save_2(p_qlt, m_llen);
            } break;
        case 3:
            while(get_record(&p_rec, &p_rec_end, &p_gen, &p_qlt) and
                  ++ m_rec_total < sanity ) {

                gen.save_3(p_gen, p_qlt, m_llen);
                rec.save_3(p_rec, p_rec_end);
                qlt.save_3(p_qlt, m_llen);
            } break;
        case 4:
            while(get_record(&p_rec, &p_rec_end, &p_gen, &p_qlt) and
                  ++ m_rec_total < sanity ) {

                gen.save_4(p_gen, p_qlt, m_llen);
                rec.save_3(p_rec, p_rec_end);
                qlt.save_3(p_qlt, m_llen);
            } break;
        }
    }
    conf.set_info("num_records", m_rec_total);
    return 0;
}

// load

UsrLoad::UsrLoad() {
    BZERO(m_last);
    m_out = conf.file_usr();
    m_rec[0] = '@' ;
    m_llen    = conf.get_long("llen");
    m_2nd_rec = conf.get_long("usr.2id");
    m_solid   = conf.get_bool("usr.solid");

    if (not m_2nd_rec) {
        m_gen[m_llen+1] = '\n';
        m_gen[m_llen+2] = '+' ;
    }

    FILE* fh = conf.open_r("usr.update", false);
    if (  fh  ) {
        pager_x = new PagerLoad16(fh, &m_x_valid);
        m_last.index = pager_x->getgap();
    }
    else {
        pager_x = NULL;
        m_last.index = -1ULL;
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
}

UsrLoad::~UsrLoad() {
    if (pager_x)
        delete pager_x;
}

void UsrLoad::update() {
    int sanity = ET_END;
    while (m_last.index == m_last.rec_count) {

        if (not sanity--)
            croak("UsrLoad: illegal exception list");
        UINT16 tnd = pager_x->get16();
        UCHAR type = 0xff & (tnd >> 8);
        UCHAR data = 0xff & (tnd);
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
        case ET_END: default: assert(0);
        }
        m_last.index = pager_x->getgap();
        if (not m_x_valid)
            m_last.index = -1ULL;
    }    
}

void UsrLoad::save() {

    size_t llen = m_llen + m_llen_factor ;

#define SAVE(X, L) putline(X, L)
    SAVE(m_rec, m_rec_size+1);
    if (m_2nd_rec) {
        SAVE(m_gen_ptr, llen);
        m_rec[0] = '+';
        SAVE(m_rec, m_rec_size+1);
        m_rec[0] = '@';
    }
    else {
        SAVE(m_gen_ptr, llen + 2);
    }
    SAVE(m_qlt_ptr, llen);
#undef  SAVE

    m_last.rec_count ++;
}

void UsrLoad::putline(UCHAR* buf, UINT32 size) {
    buf[size++] = '\n';
    size_t cnt = fwrite(buf, 1, size, m_out);

    rarely_if (cnt != size)
        croak("USR: Error writing output");
}

UINT64 UsrLoad::set_partition() {
    if (conf.partition.param < 0) // TODO: also check orig file size
        croak("illegal partition param %lld", conf.partition.param);

    const UINT64 num = conf.partition.param; // alias
    bool valid;
    PagerLoad ppart(conf.open_r("part"), &valid);
    /* UINT64 version = */ ppart.get(); 
    for (unsigned i = 0 ; valid ; i++) {

        UINT64 offs = ppart.get();
        UINT64 nrec = ppart.get();
        if (valid and
            (num == i or
             num == offs)) {
            conf.set_part_offs(offs);
            return nrec;
        }
    }
    croak ("can't find matching partition for '%lld'", num);
}

int UsrLoad::decode() {

    size_t n_recs =
        conf.partition.size ?
        set_partition() :
        conf.get_long("num_records");

    if ( ! n_recs)
        croak("Zero records, what's going on?");

    RecLoad rec;
    GenLoad gen;
    QltLoad qlt;

    UCHAR* b_qlt = m_qlt+1 ;
    UCHAR* b_gen = m_gen+1 ;
    UCHAR* b_rec = m_rec+1 ;

    // bool gentype = false;

    switch (conf.level) {
    case 1:
        while (n_recs --) {
            rarely_if (m_last.rec_count == m_last.index) update();
            m_rec_size = rec.load_1(b_rec);
            rarely_if (not m_rec_size)
                croak("premature EOF - %llu records left", n_recs+1);

            qlt.load_1(b_qlt, m_llen);
            gen.load_1(b_gen, b_qlt, m_llen);
            save();
        } break;
    case 2: default:
        while (n_recs --) {
            rarely_if (m_last.rec_count == m_last.index) update();
            m_rec_size = rec.load_2(b_rec);
            rarely_if (not m_rec_size)
                croak("premature EOF - %llu records left", n_recs+1);

            qlt.load_2(b_qlt, m_llen);
            gen.load_2(b_gen, b_qlt, m_llen);
            save();
        } break;
    case 3:
        while (n_recs --) {
            rarely_if (m_last.rec_count == m_last.index) update();
            m_rec_size = rec.load_3(b_rec);
            rarely_if (not m_rec_size)
                croak("premature EOF - %llu records left", n_recs+1);

            qlt.load_3(b_qlt, m_llen);
            gen.load_3(b_gen, b_qlt, m_llen);
            save();
        } break;
    case 4:
        while (n_recs --) {
            rarely_if (m_last.rec_count == m_last.index) update();
            m_rec_size = rec.load_3(b_rec);
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
