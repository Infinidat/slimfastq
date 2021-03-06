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



#ifndef FQ_GENS_H
#define FQ_GENS_H

#include "common.hpp"
#include "config.hpp"
#include <stdio.h>

#include "filer.hpp"
#include "base2_ranger.hpp"
#include "xfile.hpp"

class GenBase {
protected:
    GenBase() {}
    ~GenBase(){}

#define BRANGER_SIZE_1 (1<<18)
#define BRANGER_MASK_1 (BRANGER_SIZE_1-1)

#define BRANGER_SIZE_2 (1<<22)
#define BRANGER_MASK_2 (BRANGER_SIZE_2-1)

#define BRANGER_SIZE_3 (1<<24)
#define BRANGER_MASK_3 (BRANGER_SIZE_3-1)

#define BRANGER_SIZE_4 (1<<26)
#define BRANGER_MASK_4 (BRANGER_SIZE_4-1)

    Base2Ranger* ranger;
    RCoder rcoder;

    struct {
        // UINT64 count;
        UINT64 Ns_index;
        UINT64 Nn_index;
    } m_last;

    struct {
        UINT32 big_gaps;
    } m_stats ;

    bool   m_valid;
    UCHAR  m_N_byte;

    void range_init();
    size_t ranger_cnt();

    inline UINT64 get_mask() const {
        switch (conf.level) {
        default:
        case 1: return BRANGER_MASK_1;
        case 2: return BRANGER_MASK_2;
        case 3: return BRANGER_MASK_3;
        case 4: return BRANGER_MASK_4;
        }
    }
};

class GenSave : private GenBase {
public:
    GenSave() ;
    ~GenSave();
    void save(const UCHAR* gen, UCHAR* qlt, UINT64 llen, UINT64 qlen)
    {return llen == qlen ?
            save_x(gen, qlt, llen , get_mask()) :
            save_x(gen, qlt, llen , qlen, get_mask());
    }

private:
    void bad_q_or_bad_n(UCHAR gen, UCHAR qlt, bool bad_n, bool bad_q);
    inline UCHAR normalize_gen(UCHAR gen, UCHAR qlt);
    void save_x(const UCHAR* gen, const UCHAR* qlt, UINT64 size, const UINT64 mask);
    void save_x(const UCHAR* gen, const UCHAR* qlt, UINT64 llen, UINT64 qlen, const UINT64 mask);

    FilerSave* filer;
    XFileSave* x_Ns;
    XFileSave* x_Nn;
};

class GenLoad : private GenBase {
public:
    GenLoad();
    ~GenLoad();

    UINT32 load(UCHAR* gen, const UCHAR* qlt, UINT64 llen, UINT64 qlen)
    {return llen == qlen ?
            load_x(gen, qlt, llen, get_mask()) :
            load_x(gen, qlt, llen, qlen, get_mask());
    }

private:
    UINT32 load_x(UCHAR* gen, const UCHAR* qlt, UINT64 llen, const UINT64 mask);
    UINT32 load_x(UCHAR* gen, const UCHAR* qlt, UINT64 llen, UINT64 qlen, const UINT64 mask);
    inline void normalize_gen(UCHAR &gen, UCHAR qlt);

    // bool   m_validNs, m_validNn;
    const char* m_gencode;

    FilerLoad* filer;
    XFileLoad* x_Ns;
    XFileLoad* x_Nn;
};


#endif //  FQ_GENS_H
