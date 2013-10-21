//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#ifndef FQ_RECS_H
#define FQ_RECS_H

#include "common.hpp"
#include "config.hpp"
// #include "pager.hpp"
#include <stdio.h>

#include "filer.hpp"
#include "power_ranger.hpp"

class RecBase {
protected: 

    RecBase()  {}
    ~RecBase() {rcoder.done();}

    PowerRanger ranger[10];
    RCoder rcoder;

    const char* m_ids[10];
    int m_len[10];

    struct {
        UINT64 line_n;
        UINT64 tile;
        UINT64 x;
        UINT64 y;
        UINT64 n[4];
        UINT64 rec_count;
        UINT64 rec_count_tag;
        UCHAR alal;
    } m_last;

    struct {
        UINT32 big_u;
        UINT32 big_i;
        UINT32 exceptions;
    } stats;

    int    m_type;
    bool   m_valid;

    // const Config* m_conf;

    void range_init();

    // Yeh yeh, I know it should be getter and putter but I'll keep it aligned
    void put_i(int i, long long num, UINT64* old=NULL) {
        if (ranger[i].put_i(&rcoder, num, old))
            stats.big_i ++;
    }
    void put_u(int i, UINT64    num, UINT64* old=NULL) {
        if (ranger[i].put_u(&rcoder, num, old))
            stats.big_u ++;
    }
    long long get_i(int i, UINT64* old=NULL) {
        return ranger[i].get_i(&rcoder, old);
    }
    UINT64    get_u(int i, UINT64* old=NULL) {
        return ranger[i].get_u(&rcoder, old);
    };
};

class RecSave : private RecBase {
public:
     RecSave();
    ~RecSave();

    bool save_1(const UCHAR* buf, const UCHAR* end) { return save_2(buf, end); }
    bool save_2(const UCHAR* buf, const UCHAR* end);
    bool save_3(const UCHAR* buf, const UCHAR* end) { return save_2(buf, end); }
    // void pager_init();
private:
    // enum { UT_ALLELE,
    //        UT_BASES,
    //        UT_END } UpdateType;
    // void update(UpdateType type, UINT64 val);
    // bool is_allele(char a);
    void save_allele(char a);
    bool save_t_1(const UCHAR* buf, const UCHAR* end);
    bool save_t_3(const UCHAR* buf, const UCHAR* end);
    bool save_t_5(const UCHAR* buf, const UCHAR* end);

    void determine_rec_type(const UCHAR* buf, const UCHAR* end);
    int  get_rec_type(const char* start);
    // void putgap(UINT64 gap);
    // void putgap(UINT64 num, UINT64& old);
    // void putgap(UINT64 gap, bool flag);

    // PagerSave16* pager;
    // PagerSave02* pager2;
    FilerSave* filer;
};

class RecLoad : private RecBase {
public:
     RecLoad();
    ~RecLoad();

    inline bool is_valid() {return m_valid;}
    size_t load_1(UCHAR* buf) { return load_2(buf) ;}
    size_t load_2(UCHAR* buf);
    size_t load_3(UCHAR* buf) { return load_2(buf) ;}

private:
    // update();
    size_t load_t_1(UCHAR* buf);
    size_t load_t_3(UCHAR* buf);
    size_t load_t_5(UCHAR* buf);

    // UINT16 get16();
    // UINT64 getgap();
    // UINT64 getgap(UINT64& old);
    // UINT64 getgap(bool * flag);
    void determine_ids(int size) ;

    // PagerLoad16* pager;
    // PagerLoad02* pager2;

    FilerLoad* filer;
};



#endif
