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

#include "filer.hpp"
#include "power_ranger.hpp"

class RecBase {

    typedef PowerRanger<8> Ranger;

public:                         // for utest
    RecBase()  {}
    ~RecBase() {rcoder.done();}
    const char* m_ids[10];
    int m_len[10];

    struct {
        UINT64 tile;
        UINT64 index;
        UINT64 x;
        UINT64 y;
        UINT64 n[4];
        UCHAR alal;
    } m_last;

    int    m_type;
    bool   m_valid;

    const Config* m_conf;

    RangeCoder rcoder;
    Ranger ranger[10][4];
    void range_init();

    // Yeh yeh, I know it should be getter and putter but I'll keep it aligned
    void puter(int i, int j, UCHAR c);
    void put_i(int i, long long num, long long* old=NULL);
    bool put_u(int i, UINT64    num, UINT64   * old=NULL);

    UCHAR     geter(int i, int j);
    long long get_i(int i, long long* old=NULL);
    UINT64    get_u(int i, UINT64   * old=NULL);
};

class RecSave : public RecBase {
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
    FilerSave* filer;

    struct {
        UINT32 big_gaps;
        UINT32 big_gaps8;
    } stats;
};

class RecLoad : public RecBase {
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

    FilerLoad* filer;
};



#endif
