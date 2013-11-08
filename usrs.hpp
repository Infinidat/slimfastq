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

#ifndef FQ_USRS_H
#define FQ_USRS_H

#include "common.hpp"
#include "config.hpp"
#include <stdio.h>
#include "pager.hpp"

enum exception_t {
    ET_LLEN,
    ET_SOLPF_GEN,
    ET_SOLPF_QLT,

    ET_END
} ;

class UsrSave {
    // to read fastq file
    // NOTE: Line limitted to 400 bytes

#define PLL_SIZE 0x4000
#define PLL_STRT MAX_REC_LEN
    //  PLL_STRT must always be more than record size
#define PLL_LAST (PLL_SIZE + PLL_STRT)

public:
    UsrSave();
    ~UsrSave();

    int encode();

private:
    // bool get_record(UCHAR** rec, UCHAR** rec_end, UCHAR** gen, UCHAR** qlt);
    bool get_record();
    void load_page();
    void update(exception_t type, UCHAR dat);
    void pager_init();

    inline void load_check();
    inline void expect(UCHAR chr);
    bool mid_rec_msg() const ;
    void determine_record();
    UINT64 estimate_rec_limit();

    struct {
        UCHAR solid_pf_gen;
        UCHAR solid_pf_qlt;
        UINT64 rec_count;
    } m_last;

    bool   m_valid;
    UCHAR  m_buff[PLL_LAST+10]; 
    size_t m_page_count, m_rec_total;
    int m_cur, m_end;
    FILE  *m_in;
    bool  first_cycle;
    int   m_llen;
    bool  m_solid;
    PagerSave16* pager_x;
    // const Config* m_conf;
    struct {
        UCHAR* rec;
        UCHAR* rec_end;
        UCHAR* prev_rec;
        UCHAR* prev_rec_end;
        UCHAR* gen;
        UCHAR* qlt;
    } mp;
    UCHAR mp_last[MAX_REC_LEN];
};

class UsrLoad {
public:
    UsrLoad();
    ~UsrLoad();
    int decode();

private:
    void save();
    void update();
    void putline(UCHAR* buf, UINT32 size);
    UINT64 set_partition();

    FILE *m_out;

    struct {
        UCHAR solid_pf_gen;
        UCHAR solid_pf_qlt;
        UINT64 index;
        UINT64 rec_count;
    } m_last;

    size_t m_llen, m_llen_factor, m_rec_total;
    bool   m_2nd_rec, m_solid;
    UINT32 m_rec_size;

    bool  flip;
    UCHAR m_rep[MAX_ID_LLEN + 1 ];
    UCHAR m_rec[MAX_ID_LLEN + 1 ];
    UCHAR m_qlt[MAX_GN_LLEN + 2 ];
    UCHAR m_gen[MAX_GN_LLEN + 4 ];
    UCHAR * m_qlt_ptr;
    UCHAR * m_gen_ptr;

    PagerLoad16* pager_x;
    bool  m_x_valid;
    // const Config* m_conf;
};

#endif
