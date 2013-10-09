//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#ifndef FQ_QLTS_H
#define FQ_QLTS_H

#include "common.hpp"
#include "config.hpp"
#include "pager.hpp"


#define MAX_CHARS 256
#define BUCKET_SIZE 0x100000

enum FQQ_ALGO {
    FQQ_ALGO_SELF,
    FQQ_ALGO_PREV,
    FQQ_ALGO_MOST,
    // FQQ_ALGO_PREV_PREV,

    FQQ_ALGO_LAST_DUMMY
};

struct HFNode;
struct HFBits;

struct HFBits {
    UINT32 nbits;
    UINT64 vbits;
};



class FQQBase {

public:
    FQQBase ();
    ~FQQBase();

protected:
    void fill_cbits(HFNode* leaf);
    void dump_cbits();
    //    HFNode* build_tree(UINT32* hist, int* p_num=NULL);
    void set_tree(HFNode* tree);

    HFNode* m_tree;
    HFBits  m_cbits[MAX_CHARS];

    UINT64  m_wrd;
    UCHAR   m_bit; // up to 64

    bool    m_valid;
};

class QltSave : public FQQBase {
public:
    QltSave(const Config* conf);
    ~QltSave();

    void save(const UCHAR* buf, size_t size);
    bool is_valid();
    void pager_init();
private:
    void save_tree(const UINT32* hist, int num);
    void save_bucket ();
    void clear_bucket();
    void determine_algo_n_tree(FQQ_ALGO* p_algo, int* p_num);
    // FQQ_ALGO calc_algo();
    // UINT64   calc_variance(FQQ_ALGO algo) const ;

    bool pager_put(UINT64 word);
    inline void put_char(UCHAR b);
    inline void put_w();
    inline void flush();
    inline UCHAR algo_self(size_t i) const ;
    inline UCHAR algo_prev(size_t i) const ;
    inline UCHAR algo_most(size_t i) const ;
    // inline UCHAR algo_prev_prev(size_t i, UCHAR prev) const;

    PagerSave* pager;
    const Config* m_conf;

    struct {
        UINT32 algo_hist[FQQ_ALGO_LAST_DUMMY];
    } stats;
    struct {
        // big buffer
        UCHAR  buf [BUCKET_SIZE];
        size_t index ;
        UINT32 hist[FQQ_ALGO_LAST_DUMMY][MAX_CHARS];
        UCHAR  hist_range;
        UCHAR  hist_first;
    } bucket;
};

class QltLoad : public FQQBase {
public:
    QltLoad(const Config* conf);
    ~QltLoad();

    UINT32 load (UCHAR* buffer, const size_t size);
    bool is_valid();
private:
    void load_tree();
    void load_bucket();
    inline void get_w();
    inline bool get_b();
    UCHAR get_char();
    UCHAR algo_prev(UCHAR o, UCHAR p) const ;
    UCHAR algo_most(UCHAR o, UCHAR p1, UCHAR p2, UCHAR p3) const ;

    PagerLoad* pager;
    struct {
        size_t index;
        size_t size;

        UCHAR  tree_size;

        UCHAR  algo;
        UCHAR  hist_range;
        UCHAR  hist_first;
        UCHAR  hist_lastc;

        UCHAR  prev[3];
    } bucket;
};

#endif  // FQ_QLTS_H
