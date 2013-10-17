
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

#define RANGER_SIZE (1<<16)
#define RANGER_MASK (RANGER_SIZE-1)
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

class QltBase {
protected:
    Log64Ranger* ranger;
    RangeCoder rcoder;
    bool    m_valid;
    const Config* m_conf;

    void range_init();
    inline void calc_last(UINT32 &last, UCHAR q, UCHAR &q1, UCHAR &q2, size_t i) {
        last += (((MAX(q1, q2)<<6) + q) +
                 (MIN(i+15,127)&(15<<3))
                 );
        
        last &= RANGER_MASK;
        q2 = q1;
        q1 = q;
    }
    inline void calc_last (UINT32& last, UCHAR q) {
        last = ((last <<6) + q) & RANGER_MASK;
    }
};

class QltSave : private QltBase {
public:
    QltSave(const Config* conf);
    ~QltSave();

    void save(const UCHAR* buf, size_t size);
    // void filer_init();
    bool is_valid();
private:
    FilerSave* filer;
};

class QltLoad : private QltBase {
public:
    QltLoad(const Config* conf);
    ~QltLoad();

    UINT32 load (UCHAR* buffer, const size_t size);
    bool is_valid();
private:
    FilerLoad* filer;
};

#endif  // FQ_QLTS_H
