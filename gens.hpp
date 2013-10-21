//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#ifndef FQ_GENS_H
#define FQ_GENS_H

#include "common.hpp"
#include "config.hpp"
#include "pager.hpp"
#include <stdio.h>

#include "filer.hpp"
#include "base2_ranger.hpp"

class GenBase {
protected:
    GenBase(){}
    ~GenBase(){}
#define BRANGER_SIZE (1<<21)
#define BRANGER_MASK (BRANGER_SIZE-1)

    Base2Ranger* ranger; //[BRANGER_SIZE];
    RCoder rcoder;

    struct {
        UINT64 count;
        UINT64 Ns_index;
        UINT64 Nn_index;
    } m_last;
        
    // const Config* m_conf;

    struct {
        UINT32 big_gaps;
    } m_stats ;

    bool   m_lossless, m_valid, m_faster;
    UCHAR  m_N_byte;

    void range_init();
};

class GenSave : private GenBase {
public:
    GenSave();
    ~GenSave();

    void save_1(const UCHAR* gen, UCHAR* qlt, size_t size){ return save_2(gen, qlt, size); } ;
    void save_2(const UCHAR* gen, UCHAR* qlt, size_t size);
    void save_3(const UCHAR* gen, UCHAR* qlt, size_t size);
    // void pager_init();

private:
    inline void putgapNs(UINT64 gap);
    inline void putgapNn(UINT64 gap);
    inline UCHAR normalize_gen(UCHAR gen, UCHAR &qlt);

    FilerSave* filer;
    PagerSave02* pager;
    PagerSave16* pagerNs;
    PagerSave16* pagerNn;

};

class GenLoad : private GenBase {
public:
    GenLoad();
    ~GenLoad();

    UINT32 load_1(UCHAR* gen, const UCHAR* qlt, size_t size) { return load_2(gen, qlt, size); };
    UINT32 load_2(UCHAR* gen, const UCHAR* qlt, size_t size);
    UINT32 load_3(UCHAR* gen, const UCHAR* qlt, size_t size);

private:
    UCHAR get_2();
    UINT64 getgapNs();
    UINT64 getgapNn();
    inline void normalize_gen(UCHAR &gen, UCHAR qlt);

    bool   m_validNs, m_validNn;
    const char* m_gencode;

    FilerLoad* filer;
    PagerLoad02* pager;
    PagerLoad16* pagerNs;
    PagerLoad16* pagerNn;
};


#endif //  FQ_GENS_H
