
//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#ifndef FQ_QLTS_H
#define FQ_QLTS_H

#include "common.hpp"
#include "config.hpp"
#include "filer.hpp"
#include "log64_ranger.hpp"

#define RANGER_SIZE_2 (1<<16)
#define RANGER_MASK_2 (RANGER_SIZE_2-1)

#define RANGER_SIZE_1 (1<<12)
#define RANGER_MASK_1 (RANGER_SIZE_1-1)

class QltBase {
protected:

    Log64Ranger* ranger;
    RCoder rcoder;
    bool m_valid;

    size_t ranger_cnt();
    void   range_init();

    inline static UINT32 calc_last_1 (UINT32 last, UCHAR b) {
        return ( b | (last << 6) ) & RANGER_MASK_1;
    }
    inline static UINT32 calc_last_2 (UINT32 last, UCHAR b) {
        return
            ( b | 
              (last << 6)
              ) & RANGER_MASK_2;
    }
    // inline static UINT32 calc_last_3 (UINT32 last, UCHAR b) {
    //     return ( b | (last << 6) ) & RANGER_MASK_3;
    //     // TODO: use delta
    // }
    inline static UINT32 min_val(UINT32 a, UINT32 b) { return a>b?b:a; }
    inline static UCHAR  max_val(UCHAR  a, UCHAR  b) { return a>b?a:b; }
    inline static UINT32 calc_last_delta(UINT32 &delta, UCHAR q, UCHAR q1, UCHAR q2) {

        // This brilliant code could only be of James Bonfield
        if (q1>q)
            delta += (q1-q);

        return
            ( q
              | (max_val(q1, q2) << 6)
              | ((q1 == q2)      << 12)
              | (min_val(7, (delta>>3)) << 13)
              ) & RANGER_MASK_2 ;
    }
};

class QltSave : private QltBase {
public:
    QltSave();
    ~QltSave();

    void save_1(const UCHAR* buf, size_t size);
    void save_2(const UCHAR* buf, size_t size);
    void save_3(const UCHAR* buf, size_t size);
    // void filer_init();
    bool is_valid();
private:
    FilerSave* filer;
};

class QltLoad : private QltBase {
public:
    QltLoad();
    ~QltLoad();

    bool is_valid();
    UINT32 load_1 (UCHAR* buffer, const size_t size);
    UINT32 load_2 (UCHAR* buffer, const size_t size);
    UINT32 load_3 (UCHAR* buffer, const size_t size);
private:
    FilerLoad* filer;
};

#endif  // FQ_QLTS_H
