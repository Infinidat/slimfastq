//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#include "gens.hpp"

#include <assert.h>
// ?:
#include <stdlib.h>
#include <errno.h>
#include <string.h>


void GenBase::range_init() {
    for (int i = 0 ; i < BRANGER_SIZE; i++)
        ranger[i].init();
    // bzero(ranger, sizeof(ranger[0])*BRANGER_SIZE);
    // memset(ranger, 1, sizeof(ranger[0])*BRANGER_SIZE);
}

GenSave::GenSave() {
    // m_conf = conf;
    m_lossless = true;
    conf.set_info("gen.lossless", m_lossless);

    pagerNs = pagerNn = NULL;
    BZERO(m_stats);
    BZERO(m_last);
    m_N_byte = 0;

    m_faster = conf.level < 3;
    conf.set_info("gen.faster", m_faster);

    if (m_faster) {
        filer = NULL;
        pager = new PagerSave02(conf.open_w("gen"));
    }
    else {
        pager = NULL;
        filer = new FilerSave(conf.open_w("gen"));
        assert(filer);
        rcoder.init(filer);
        ranger = new Base2Ranger[BRANGER_SIZE];
        range_init();
    }
}

GenSave::~GenSave() {
    if (not m_faster)
        rcoder.done();
    DELETE(filer);

    delete pager;
    delete pagerNs;
    delete pagerNn;
    // fprintf stats
}

// void GenSave::pager_init() {
//     DELETE( pager   );
//     DELETE( pagerNs );
//     DELETE( pagerNn );
//     pager = new PagerSave02(m_conf->open_w("gen"));
// 
//     BZERO(m_last);
// }

void GenSave::putgapNs(UINT64 gap) {
    // pagerNs ||= new ... if only life could have been like perl .. 
    rarely_if(not pagerNs)
        pagerNs = new PagerSave16(conf.open_w("gen.Ns"));

    if (pagerNs->putgap(gap))
        m_stats.big_gaps ++;
}

void GenSave::putgapNn(UINT64 gap) {
    rarely_if(not pagerNn) 
        pagerNn = new PagerSave16(conf.open_w("gen.Nn")) ;

    if (pagerNn->putgap(gap))
        m_stats.big_gaps ++;
}

inline UCHAR GenSave::normalize_gen(UCHAR gen, UCHAR &qlt) {
        
    bool bad_n = false;
    bool bad_q = qlt == '!';

    // switch(gen[i] | 0x20) { - support lowercase, only if we ever see it in fastq
    UCHAR n;
    switch(gen) {
    case '0': case 'A': n = 0; break;
    case '1': case 'C': n = 1; break;
    case '2': case 'G': n = 2; break;
    case '3': case 'T': n = 3; break;
    case '.': case 'N': n = 0;
        bad_n = true;
        break;
    default: croak("unexpected genome char: %c", gen);
    }
        
    if (// m_lossless - always is
        true ) {
        rarely_if(bad_n) {
            rarely_if(not m_N_byte) {
                // TODO: make a single m_last - exceptions file
                m_N_byte = gen;
                if ('.' == gen) {
                    conf.set_info("gen.N_byte", gen);
                    conf.save_info();
                }
            }
            // TODO: eliminate this temp sanity
            rarely_if (gen != m_N_byte)
                croak("switched N_byte: %c", gen);

            if (not bad_q) {
                putgapNs(m_last.count - m_last.Ns_index);
                m_last.Ns_index = m_last.count ;
            }
        }
        else rarely_if (bad_q) {
                putgapNn(m_last.count - m_last.Nn_index);
                m_last.Nn_index = m_last.count;
            }
    }
    else {
        rarely_if (bad_n)
            qlt = '!';
    }
    return n;
}

void GenSave::save(const UCHAR* gen, UCHAR* qlt, size_t size) {

    // rarely_if(not pager)
    //     pager_init();

    // *is_raw = 1; // TODO: allow indexing

    UINT32 last = 0x007616c7 & BRANGER_MASK;


    if (m_faster)
        for (size_t i = 0; i < size; i ++) {
            m_last.count ++;
            UCHAR n = normalize_gen(gen[i], qlt[i]);

            pager->put02(n);
        }
    else 
        for (size_t i = 0; i < size; i ++) {
            m_last.count ++;
            UCHAR n = normalize_gen(gen[i], qlt[i]);

            PREFETCH(ranger + last); // I have no idea why it's faster when located here
            ranger[last].put(&rcoder, n);
            last = ((last<<2) + n) & BRANGER_MASK;

            // PREFETCH(ranger + last);
        }
}

// load

UINT64 GenLoad::getgapNs() {
    return
        pagerNs ?
        pagerNs -> getgap() :
        0;
}

UINT64 GenLoad::getgapNn() {
    return
        pagerNn ?
        pagerNn -> getgap() :
        0;
}

GenLoad::GenLoad() {
    BZERO(m_last);
    // m_conf = conf;
    m_valid = true;
    pagerNs = pagerNn = NULL;
    m_lossless = *conf.get_info("gen.lossless") == '0' ? false : true ;
    if (m_lossless) {
        FILE* fNs = conf.open_r("gen.Ns", false);
        if (  fNs  ) {
            pagerNs = new PagerLoad16( fNs, &m_validNs);
            m_last.Ns_index = getgapNs();
        }
        FILE* fNn = conf.open_r("gen.Nn", false);
        if (  fNn ) {
            pagerNn = new PagerLoad16( fNn , &m_validNn);
            m_last.Nn_index = getgapNn();
        }
    }

    m_N_byte          = conf.get_long("gen.N_byte", 'N') ;
    bool is_solid     = conf.get_bool("usr.solid");
    bool is_lowercase = false;

    m_gencode =
        is_solid ?
        "0123" :
        is_lowercase ?
        "acgt" :
        "ACGT" ;


    m_faster = *conf.get_info("gen.faster") == '1' ? true : false ;
    if (m_faster) {
        pager = new PagerLoad02(conf.open_r("gen"), &m_valid);
        filer = NULL;
    }
    else {
        pager = NULL;
        filer = new FilerLoad(conf.open_r("gen"), &m_valid);
        assert(filer);
        rcoder.init(filer);
        ranger = new Base2Ranger[BRANGER_SIZE];
        range_init();
    }
}

GenLoad::~GenLoad() {
    if (not m_faster)
        rcoder.done();
    DELETE(filer);

    delete pager;
    delete pagerNs;
    delete pagerNn;
}

UINT32 GenLoad::load(UCHAR* gen, const UCHAR* qlt, size_t size) {
    // TODO: if lowercase
    UINT32 last = 0x007616c7 & BRANGER_MASK;

    for (size_t i = 0; i < size; i ++ ) {
        m_last.count ++ ;

        if (m_faster)
            gen[i] = m_gencode [ pager->get02() ];
        else {
            PREFETCH(ranger + last);
            UCHAR b = ranger[last].get(&rcoder);
            gen[i] = m_gencode [ b ];
            last = ((last<<2) + b) & BRANGER_MASK;
            // PREFETCH(ranger + last);
        }

        rarely_if (m_last.Nn_index and
                   m_last.Nn_index == m_last.count) {
            m_last.Nn_index += getgapNn();
        }
        else rarely_if (qlt[i] == '!')
            gen[i] = m_N_byte;
        else rarely_if (m_last.Ns_index and
                 m_last.Ns_index == m_last.count) {
            gen[i] = m_N_byte;
            m_last.Ns_index += getgapNs();
        }
    }

    return m_valid ? size : 0;
}


/* Solid map: 
  0 : ACGT => ACGT
  1 : ACGT => CATG
  2 : ACGT => GTAC
  3 : ACGT => TGCA

  last = first dummy
  str .= (last = map[ cur ] [ last ])

*/
