//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

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
    UsrSave(const Config* conf);
    ~UsrSave();

    int encode();

private:
    bool get_record(UCHAR** rec, UCHAR** rec_end, UCHAR** gen, UCHAR** qlt);
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
    const Config* m_conf;
};

class UsrLoad {
public:
    UsrLoad(const Config* conf);
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

    UCHAR m_rec[MAX_ID_LLEN + 1 ];
    UCHAR m_qlt[MAX_GN_LLEN + 2 ];
    UCHAR m_gen[MAX_GN_LLEN + 4 ];
    UCHAR * m_qlt_ptr;
    UCHAR * m_gen_ptr;

    PagerLoad16* pager_x;
    bool  m_x_valid;
    const Config* m_conf;
};

#endif
