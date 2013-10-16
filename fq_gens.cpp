//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#include "fq_gens.hpp"

#include <assert.h>
// ?:
#include <stdlib.h>
#include <errno.h>
#include <string.h>


GenSave::GenSave(const Config* conf) {
    m_conf = conf;
    pager = NULL;
    m_lossless = true;
    conf->set_info("gen.lossless", m_lossless);

    pagerNs = pagerNn = NULL;
    BZERO(m_stats);
    BZERO(m_last);
    m_N_byte = 0;
}

GenSave::~GenSave() {
    delete pager;
    delete pagerNs;
    delete pagerNn;

    // fprintf stats
}

void GenSave::pager_init() {
    DELETE( pager   );
    DELETE( pagerNs );
    DELETE( pagerNn );
    pager = new PagerSave02(m_conf->open_w("gen"));

    BZERO(m_last);
}

void GenSave::putgapNs(UINT64 gap) {
    // pagerNs ||= new ... if only life could have been like perl .. 
    rarely_if(not pagerNs)
        pagerNs = new PagerSave16(m_conf->open_w("gen.Ns"));

    if (pagerNs->putgap(gap))
        m_stats.big_gaps ++;
}

void GenSave::putgapNn(UINT64 gap) {
    rarely_if(not pagerNn) 
        pagerNn = new PagerSave16(m_conf->open_w("gen.Nn")) ;

    if (pagerNn->putgap(gap))
        m_stats.big_gaps ++;
}

void GenSave::save(const UCHAR* gen, UCHAR* qlt, size_t size) {

    rarely_if(not pager)
        pager_init();

    // *is_raw = 1; // TODO: allow indexing

    for (size_t i = 0; i < size; i ++) {
        UCHAR n=0;
        bool bad_n = false;
        bool bad_q = qlt[i] == '!';
        m_last.count ++;

        // switch(gen[i] | 0x20) { - support lowercase, only if we ever see it in fastq

        switch(gen[i]) {
        case '0': case 'A': n = 0; break;
        case '1': case 'C': n = 1; break;
        case '2': case 'G': n = 2; break;
        case '3': case 'T': n = 3; break;
        case '.': case 'N': n = 0;
            bad_n = true;
            break;

        default: croak("unexpected genome char: %c", gen[i]);
        }
        
        if (m_lossless) {
            if (bad_n) {
                rarely_if(not m_N_byte) {
                    m_N_byte = gen[i];
                    if ('.' == gen[i]) {
                        m_conf->set_info("gen.N_byte", gen[i]);
                        m_conf->save_info();
                    }
                }
                // TODO: eliminate this temp sanity
                rarely_if (gen[i] != m_N_byte)
                    croak("switched N_byte: %c", gen[i]);

                if (not bad_q) {
                    putgapNs(m_last.count - m_last.Ns_index);
                    m_last.Ns_index = m_last.count ;
                }
            }
            else if (bad_q) {
                putgapNn(m_last.count - m_last.Nn_index);
                m_last.Nn_index = m_last.count;
            }
        }
        else {
            if (bad_n)
            qlt[i] = '!';
        }
        
        pager->put02(n);
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

GenLoad::GenLoad(const Config* conf) {
    BZERO(m_last);
    m_valid = true;
    pager = new PagerLoad02(conf->open_r("gen"), &m_valid);
    pagerNs = pagerNn = NULL;

    m_lossless = *conf->get_info("gen.lossless") == '1' ? true : false ;
    if (m_lossless) {
        FILE* fNs = conf->open_r("gen.Ns", false);
        if (  fNs  ) {
            pagerNs = new PagerLoad16( fNs, &m_validNs);
            m_last.Ns_index = getgapNs();
        }
        FILE* fNn = conf->open_r("gen.Nn", false);
        if (  fNn ) {
            pagerNn = new PagerLoad16( fNn , &m_validNn);
            m_last.Nn_index = getgapNn();
        }
    }

    m_N_byte          = conf->get_long("gen.N_byte", 'N') ;
    bool is_solid     = conf->get_bool("usr.solid");
    bool is_lowercase = false;

    m_gencode =
        is_solid ?
        "0123" :
        is_lowercase ?
        "acgt" :
        "ACGT" ;
}

GenLoad::~GenLoad() {
    delete pager;
    delete pagerNs;
    delete pagerNn;
}

UINT32 GenLoad::load(UCHAR* gen, const UCHAR* qlt, size_t size) {
    // TODO: if lowercase
    for (size_t i = 0; i < size; i ++ ) {
        m_last.count ++ ;
        gen[i] = m_gencode [ pager->get02() ];

        if (m_last.Nn_index and
            m_last.Nn_index == m_last.count) {
            m_last.Nn_index += getgapNn();
        }
        else if (qlt[i] == '!')
            gen[i] = m_N_byte;
        else if (m_last.Ns_index and
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
