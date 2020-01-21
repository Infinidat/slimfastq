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




#include "gens.hpp"
#include "config.hpp"
#include <assert.h>
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
    // bzero(ranger, sizeof(ranger[0])* cnt);
    // memset(ranger, 2, sizeof(ranger[0])*BRANGER_SIZE); - how could it be slower than nantive constructor? I would expect 'memset' implementation to use pipes
}

//////////
// save //
//////////

static char gencodes[256];

GenSave::GenSave() {
    m_lossless = true;
    conf.set_info("gen.lossless", m_lossless);

    BZERO(m_stats);
    BZERO(m_last);
    m_N_byte = 0;

    filer = new FilerSave("gen");
    assert(filer);
    rcoder.init(filer);
    ranger = new Base2Ranger[ranger_cnt()];
    range_init();

    x_Ns = new XFileSave("gen.Ns");
    x_Nn = new XFileSave("gen.Nn");

   memset(gencodes, 0x10, sizeof(gencodes));
   gencodes[(UINT8)'0'] = gencodes[(UINT8)'A'] = gencodes[(UINT8)'a'] = 0;
   gencodes[(UINT8)'1'] = gencodes[(UINT8)'C'] = gencodes[(UINT8)'c'] = 1;
   gencodes[(UINT8)'2'] = gencodes[(UINT8)'G'] = gencodes[(UINT8)'g'] = 2;
   gencodes[(UINT8)'3'] = gencodes[(UINT8)'T'] = gencodes[(UINT8)'t'] = 3;
   gencodes[(UINT8)'.'] = gencodes[(UINT8)'N'] = gencodes[(UINT8)'n'] = 4;
}

GenSave::~GenSave() {
    rcoder.done();
    if (not conf.quiet and filer and x_Ns and x_Nn)
        fprintf(stderr, "::: GEN comp size: %lu \t| Ns: %lu, Nn: %lu\n",
                filer->tell(), x_Ns->tell(), x_Nn->tell());
    delete []ranger;
    delete filer;

    DELETE(x_Ns);
    DELETE(x_Nn);
}

// inline UCHAR GenSave::normalize_gen(UCHAR gen, UCHAR &qlt) {
inline UCHAR GenSave::normalize_gen(UCHAR gen, UCHAR qlt) {

    bool bad_n; //  = false;
    const bool bad_q = qlt == '!';

    // switch(gen[i] | 0x20) { - support lowercase, only if we ever see it in fastq
    // UCHAR n;
    // switch(gen) {
    //     // TODO: optimized with a char table
    // case '0': case 'A': n = 0; break;
    // case '1': case 'C': n = 1; break;
    // case '2': case 'G': n = 2; break;
    // case '3': case 'T': n = 3; break;
    // case '.': case 'N': n = 0; bad_n = true; break;
    // default: croak("unexpected genome char: %c", gen);
    // }
    UCHAR n = gencodes[gen];
    likely_if (n <= 3)
        bad_n = false;
    else {
        rarely_if( n > 4)
            croak("unexpected genome char: %c", gen);
        bad_n = true;
        n = 0;
    }

    g_genofs_count ++;
    // if (// m_lossless - always is
    //     true ) {
        rarely_if(bad_n) {
            rarely_if(not m_N_byte) {
                // TODO: make a single m_last - exceptions file
                m_N_byte = gen;
                if ('N' != gen)
                    conf.set_info("gen.N_byte", gen);
            }
            // TODO: eliminate this temp sanity
            rarely_if (gen != m_N_byte)
                croak("switched N_byte: %c", gen);

            if (not bad_q) {
                x_Ns->put(g_genofs_count - m_last.Ns_index);
                m_last.Ns_index = g_genofs_count ;
            }
        }
        else rarely_if (bad_q) {
                x_Nn->put(g_genofs_count - m_last.Nn_index);
                m_last.Nn_index = g_genofs_count;
            }
    // }
    // else {
    //     rarely_if (bad_n)
    //         qlt = '!';
    // }
    return n;
}

void GenSave::save_x(const UCHAR* gen, const UCHAR* qlt, UINT64 size, const UINT64 mask) {
    UINT32 last = 0x007616c7;
    const UCHAR* g = gen; const UCHAR* q = qlt;
    for (; g < gen + size ; g++, q++) {
        UCHAR n = normalize_gen(*g, *q);
        last &= mask;
        PREFETCH(ranger + last);
        ranger[last].put(&rcoder, n);
        last = ((last<<2) | n);
    }
}

void GenSave::save_x(const UCHAR* gen, const UCHAR* qlt, UINT64 llen, UINT64 qlen, const UINT64 mask) {
    UINT32 last = 0x007616c7;
    for (uint i = 0; i < llen; i++) {
        UCHAR n = normalize_gen(gen[i], i < qlen ? qlt[i] : 40);
        last &= mask;
        PREFETCH(ranger + last);
        ranger[last].put(&rcoder, n);
        last = ((last<<2) | n);
    }
}

 //////////
 // load //
 //////////

GenLoad::GenLoad() {
    BZERO(m_last);
    m_valid = true;
    m_lossless = true; // *conf.get_info("gen.lossless") == '0' ? false : true ;
    m_N_byte          = conf.get_long("gen.N_byte", 'N') ;
    bool is_solid     = conf.get_bool("usr.solid");
    bool is_lowercase = false;

    m_gencode =
        is_solid ?
        "0123" :
        is_lowercase ?
        "acgt" :
        "ACGT" ;

    filer = new FilerLoad("gen", &m_valid);
    assert(filer);
    rcoder.init(filer);
    ranger = new Base2Ranger[ranger_cnt()];
    range_init();

    x_Ns = new XFileLoad("gen.Ns");
    x_Nn = new XFileLoad("gen.Nn");
    m_last.Ns_index = x_Ns->get();
    m_last.Nn_index = x_Nn->get();
}

GenLoad::~GenLoad() {
    rcoder.done();
    delete []ranger;
    delete   filer;
    DELETE(x_Ns);
    DELETE(x_Nn);
}

void GenLoad::normalize_gen(UCHAR & gen, UCHAR qlt) {
    g_genofs_count ++ ;

    rarely_if (m_last.Nn_index == g_genofs_count)
        m_last.Nn_index += x_Nn->get();

    else rarely_if (qlt == '!')
        gen = m_N_byte;

    else rarely_if (m_last.Ns_index == g_genofs_count) {
        gen = m_N_byte;
        m_last.Ns_index += x_Ns->get();
   }
}

UINT32 GenLoad::load_x(UCHAR* gen, const UCHAR* qlt, UINT64 size, const UINT64 mask) {

    UINT32 last = 0x007616c7;

    UCHAR* g = gen; const UCHAR* q = qlt;
    for (; g < gen + size ; g++, q++) {
        last &= mask ;
        PREFETCH(ranger + last);
        UCHAR b = ranger[last].get(&rcoder);

        *g = m_gencode [ b ];
        last = ((last<<2) + b);
        normalize_gen(*g, *q);
    }

    return m_valid ? size : 0;
}

UINT32 GenLoad::load_x(UCHAR* gen, const UCHAR* qlt, UINT64 llen, UINT64 qlen, const UINT64 mask) {
    // rare: mismatch qlt/gen line sizes. This typically happens when padding the gen line. In this
    // case, assume valid base.
    UINT32 last = 0x007616c7;

    for (uint i = 0; i < llen; i++) {
        last &= mask ;
        PREFETCH(ranger + last);
        UCHAR b = ranger[last].get(&rcoder);

        gen[i] = m_gencode[ b ];
        last = ((last<<2) + b);
        normalize_gen(gen[i], i < qlen ? qlt[i] : 40);
    }

    return m_valid ? llen : 0;
}
