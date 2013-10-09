//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#ifndef FQ_RECS_H
#define FQ_RECS_H

#include "common.hpp"
#include "config.hpp"
#include "pager.hpp"
#include <stdio.h>

class RecSave {
public:
    RecSave(const Config* conf);
    ~RecSave();

    bool save(const UCHAR* buf, const UCHAR* end, bool gentype);
    void pager_init();
private:
    bool is_allele(char a);
    bool save_1(const UCHAR* buf, const UCHAR* end, bool gentype);
    bool save_3(const UCHAR* buf, const UCHAR* end, bool gentype);
    bool save_5(const UCHAR* buf, const UCHAR* end, bool gentype);

    void determine_rec_type(const UCHAR* buf, const UCHAR* end);
    int  get_rec_type(const char* start);
    void putgap(UINT64 gap);
    void putgap(UINT64 num, UINT64& old);
    // void putgap(UINT64 gap, bool flag);

    PagerSave16* pager;
    PagerSave02* pager2;

    int    m_type;
    bool   m_valid;
    const Config* m_conf;

    const char* m_ids[10];
    int m_len[10];
    struct {
        UINT64 index;
        UINT64 tile;
        UINT64 x;
        UINT64 y;
        UINT64 n[4];
        UCHAR alal;
    } m_last;

    struct {
        UINT32 big_gaps;
    } stats;
};


class RecLoad {
public:
    RecLoad(const Config* conf);
    ~RecLoad();

    inline bool is_valid() {return m_valid;}
    size_t load(UCHAR* buf, bool *gentype);

private:
    size_t load_1(UCHAR* buf, bool *gentype);
    size_t load_3(UCHAR* buf, bool *gentype);
    size_t load_5(UCHAR* buf, bool *gentype);

    UINT16 get16();
    UINT64 getgap();
    UINT64 getgap(UINT64& old);
    // UINT64 getgap(bool * flag);
    void determine_ids(int size) ;

    PagerLoad16* pager;
    PagerLoad02* pager2;

    int    m_type;
    bool   m_valid;
    struct {
        UINT64 tile;
        UINT64 index;
        UINT64 x;
        UINT64 y;
        UINT64 n[4];
        UCHAR alal;
    } m_last;

    const char* m_ids[10];
    int m_len[10];
};



#endif
