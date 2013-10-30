/***********************************************************************************************************************/
/* This program was written by Josef Ezra  <jezra@infinidat.com>                                                       */
/* Copyright (c) 2013, infinidat                                                                                       */
/* All rights reserved.                                                                                                */
/*                                                                                                                     */
/* Redistribution and use in source and binary forms, with or without modification, are permitted provided that        */
/* the following conditions are met:                                                                                   */
/*                                                                                                                     */
/* Redistributions of source code must retain the above copyright notice, this list of conditions and the following    */
/* disclaimer.                                                                                                         */
/* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following */
/* disclaimer in the documentation and/or other materials provided with the distribution.                              */
/* Neither the name of the <ORGANIZATION> nor the names of its contributors may be used to endorse or promote products */
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




#include "gens.hpp"

#include <assert.h>
// ?:
#include <stdlib.h>
#include <errno.h>
#include <string.h>


size_t GenBase::ranger_cnt() {
    switch(conf.level) {
    case 1: return BRANGER_SIZE_1;
    default:
    case 2: return BRANGER_SIZE_2;
    case 3: return BRANGER_SIZE_3;
    case 4: return BRANGER_SIZE_4;
    }
}

void GenBase::range_init() {
    const int cnt = ranger_cnt();
    for (int i = 0 ; i < cnt; i++)
        ranger[i].init();
    // bzero(ranger, sizeof(ranger[0])* cnt);
    // memset(ranger, 1, sizeof(ranger[0])*BRANGER_SIZE);
}

//////////
// save //
//////////

GenSave::GenSave() {
    // m_conf = conf;
    m_lossless = true;
    conf.set_info("gen.lossless", m_lossless);

    pagerNs = pagerNn = NULL;
    BZERO(m_stats);
    BZERO(m_last);
    m_N_byte = 0;

    // if (conf.level == 1) {
    //     filer = NULL;
    //     pager = new PagerSave02(conf.open_w("gen"));
    // }
    // else {
        // pager = NULL;
        filer = new FilerSave(conf.open_w("gen"));
        assert(filer);
        rcoder.init(filer);
        ranger = new Base2Ranger[ranger_cnt()];
        range_init();
    // }
}

GenSave::~GenSave() {
    // if (conf.level > 1)
        rcoder.done();
    DELETE(filer);

    // delete pager;
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

// void GenSave::save_1(const UCHAR* gen, UCHAR* qlt, size_t size) {
// 
//     // rarely_if(not pager)
//     //     pager_init();
// 
//     for (size_t i = 0; i < size; i ++) {
//         m_last.count ++;
//         UCHAR n = normalize_gen(gen[i], qlt[i]);
// 
//         pager->put02(n);
//     }
// }

void GenSave::save_1(const UCHAR* gen, UCHAR* qlt, size_t size) {
    UINT32 last = 0;
    for (size_t i = 0; i < size; i ++) {
        m_last.count ++;
        UCHAR n = normalize_gen(gen[i], qlt[i]);

        last &= BRANGER_MASK_1;
        ranger[last].put(&rcoder, n);
        last = ((last<<2) + n) ;
    }
}

void GenSave::save_2(const UCHAR* gen, UCHAR* qlt, size_t size) {
    UINT32 last = 0x007616c7;
    for (size_t i = 0; i < size; i ++) {
        m_last.count ++;
        UCHAR n = normalize_gen(gen[i], qlt[i]);

        last &= BRANGER_MASK_2;
        ranger[last].put(&rcoder, n);
        last = ((last<<2) + n);
    }
}

void GenSave::save_3(const UCHAR* gen, UCHAR* qlt, size_t size) {
    UINT32 last = 0x007616c7;
    for (size_t i = 0; i < size; i ++) {
        m_last.count ++;
        UCHAR n = normalize_gen(gen[i], qlt[i]);

        last &= BRANGER_MASK_3;
        PREFETCH(ranger + last); 
        ranger[last].put(&rcoder, n);
        last = ((last<<2) + n);
    }
}

void GenSave::save_4(const UCHAR* gen, UCHAR* qlt, size_t size) {
    UINT32 last = 0x007616c7;
    for (size_t i = 0; i < size; i ++) {
        m_last.count ++;
        UCHAR n = normalize_gen(gen[i], qlt[i]);

        last &= BRANGER_MASK_4;
        PREFETCH(ranger + last); 
        ranger[last].put(&rcoder, n);
        last = ((last<<2) + n);
    }
}

 //////////
 // load //
 //////////

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


    // if (conf.level == 1) {
    //     pager = new PagerLoad02(conf.open_r("gen"), &m_valid);
    //     filer = NULL;
    // }
    // else {
    //     pager = NULL;
        filer = new FilerLoad(conf.open_r("gen"), &m_valid);
        assert(filer);
        rcoder.init(filer);
        ranger = new Base2Ranger[ranger_cnt()];
        range_init();
    // }
}

GenLoad::~GenLoad() {
    // if (conf.level > 1)
        rcoder.done();
    DELETE(filer);

    // delete pager;
    delete pagerNs;
    delete pagerNn;
}

void GenLoad::normalize_gen(UCHAR & gen, UCHAR qlt) {
    rarely_if (m_last.Nn_index and
               m_last.Nn_index == m_last.count) {
        m_last.Nn_index += getgapNn();
    }

    else rarely_if (qlt == '!')
        gen = m_N_byte;
    else rarely_if (m_last.Ns_index and
                    m_last.Ns_index == m_last.count) {
        gen = m_N_byte;
        m_last.Ns_index += getgapNs();
   }
}

// UINT32 GenLoad::load_1(UCHAR* gen, const UCHAR* qlt, size_t size) {
//     for (size_t i = 0; i < size; i ++ ) {
//         m_last.count ++ ;
//         gen[i] = m_gencode [ pager->get02() ];
//         // if lossless
//         normalize_gen(gen[i], qlt[i]); 
//    }
// 
//     return m_valid ? size : 0;
// }

UINT32 GenLoad::load_1(UCHAR* gen, const UCHAR* qlt, size_t size) {

    UINT32 last = 0;

    for (size_t i = 0; i < size; i ++ ) {
        m_last.count ++ ;
        last &= BRANGER_MASK_1;
        UCHAR b = ranger[last].get(&rcoder);
        gen[i] = m_gencode [ b ];
        last = ((last<<2) + b);
        normalize_gen(gen[i], qlt[i]);
    }

    return m_valid ? size : 0;
}

UINT32 GenLoad::load_2(UCHAR* gen, const UCHAR* qlt, size_t size) {

    UINT32 last = 0x007616c7;

    for (size_t i = 0; i < size; i ++ ) {
        m_last.count ++ ;
        last &= BRANGER_MASK_2;
        UCHAR b = ranger[last].get(&rcoder);
        gen[i] = m_gencode [ b ];
        last = ((last<<2) + b);
        normalize_gen(gen[i], qlt[i]);
    }

    return m_valid ? size : 0;
}

UINT32 GenLoad::load_3(UCHAR* gen, const UCHAR* qlt, size_t size) {

    UINT32 last = 0x007616c7;

    for (size_t i = 0; i < size; i ++ ) {
        m_last.count ++ ;

        last &= BRANGER_MASK_3;

        PREFETCH(ranger + last);
        UCHAR b = ranger[last].get(&rcoder);
        gen[i] = m_gencode [ b ];

        last = ((last<<2) + b);
        normalize_gen(gen[i], qlt[i]);
    }

    return m_valid ? size : 0;
}

UINT32 GenLoad::load_4(UCHAR* gen, const UCHAR* qlt, size_t size) {

    UINT32 last = 0x007616c7;

    for (size_t i = 0; i < size; i ++ ) {
        m_last.count ++ ;

        last &= BRANGER_MASK_4;

        PREFETCH(ranger + last);
        UCHAR b = ranger[last].get(&rcoder);
        gen[i] = m_gencode [ b ];

        last = ((last<<2) + b);
        normalize_gen(gen[i], qlt[i]);
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
