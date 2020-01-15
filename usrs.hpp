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
#include "xfile.hpp"

// NOTE: soon there would be new machines, providing longer lines.
#define MAX_ID_LLEN  0x2000
#define MAX_GN_LLEN  0x10000
#define MAX_REC_LEN  (2*(MAX_GN_LLEN + MAX_ID_LLEN))

class UsrBase {

public:
enum exception_t {
    ET_LLEN,
    ET_SOLPF_GEN,
    ET_SOLPF_QLT,

    ET_END
} ;

#define PLL_SIZE 0x100000
#define PLL_STRT MAX_REC_LEN
    //  PLL_STRT must be more than record size
#define PLL_LAST (PLL_SIZE + PLL_STRT)

    struct {
        UINT64 i_llen;
        UINT64 i_sgen;
        UINT64 i_sqlt;

        UINT64 i_long;

        UCHAR solid_pf_gen;
        UCHAR solid_pf_qlt;
    } m_last;
};

class UsrSave : public UsrBase {


public:
    UsrSave();
    ~UsrSave();

    int encode();

private:
    bool get_record();
    bool get_oversized_record(int cur, bool from_get=true);
    void load_page();
    void update(exception_t type, UINT16 dat);

    inline void load_check();
    inline UCHAR load_char();
    inline void expect(UCHAR chr);
    bool mid_rec_msg() const ;
    void determine_record();
    UINT64 estimate_rec_limit();

    bool   m_valid;
    UCHAR  m_buff[PLL_LAST+10];
    size_t m_page_count;
    int m_cur, m_end;
    FILE  *m_in;
    bool  first_cycle;
    int   m_llen;
    bool  m_solid;
    XFileSave* x_llen;
    // XFileSave* gen_len;
    // XFileSave* qlt_len;
    XFileSave* x_sgen;
    XFileSave* x_sqlt;
    XFileSave* x_lgen;
    XFileSave* x_lqlt;
    XFileSave* x_lrec;
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

class UsrLoad : public UsrBase {
public:
    UsrLoad();
    ~UsrLoad();
    int decode();

private:
    void save();
    void update();
    void putline(UCHAR* buf, UINT32 size);

    FILE *m_out;

    size_t m_llen, m_llen_factor, m_qlen;
    // UINT64 m_rec_total;
    bool   m_2nd_rec, m_solid;
    UINT32 m_rec_size;
    long   comp_version;

    bool  flip;
    UCHAR m_rep[MAX_ID_LLEN + 1 ];
    UCHAR m_rec[MAX_ID_LLEN + 1 ];
    UCHAR m_qlt[MAX_GN_LLEN + 2 ];
    UCHAR m_gen[MAX_GN_LLEN + 4 ];
    UCHAR * m_qlt_ptr;
    UCHAR * m_gen_ptr;

    XFileLoad* x_llen;
    XFileLoad* x_sgen;
    XFileLoad* x_sqlt;
    XFileLoad* x_lgen;
    XFileLoad* x_lqlt;
    XFileLoad* x_lrec;
};

#endif
