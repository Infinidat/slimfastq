
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
    // inline UINT32 calc_last(UINT32 last, const UCHAR* q, size_t i) {
    //     return
    //         ( RARELY(i < 2) ?
    //           (last << 6) + q[i] - '!' :
    //           (( q[i] )-'!') |
    //           (((q[i-2] == q[i-3] ? q[i-2] : q[i-1])-'!') << 6) |
    //           (((i>>4) << 12) & 0x8f00) |
    //           ((q[i] == q[i-1]) <<15 )
    //           ) & RANGER_MASK;
    //         
    // }
    inline UINT32 calc_last (UINT32 last, UCHAR b) {
        return
            ( b | 
              (last << 6)
              ) & RANGER_MASK;
        // UCHAR q  = b & 0x3f;
        // UCHAR q1 = last & 0x3f;
        // UCHAR q2 = (last >> 6) & 0x3f;
        // return ( q
        //          |
        //          q1 << 6
        //          |
        //          (((q2>q1? 64 : 0)+q1-q2)/4) << 12
        //         ) & RANGER_MASK;
    }
    // inline UINT32 calc_last(UINT32 &delta, const UCHAR* q, size_t i) {
    // 
    //     return 
    //         ( RARELY(i < 2) ?
    //           0 : 
    //           (q[i]-'!')
    //           | 
    //           ((MAX(q[i-1], q[i-2])-'!')<<6 )
    //           |
    //           ((q[i-1] == q[i-2]) << 12)
    //           |
    //           (( MIN(7*8, (delta += (q[i-1]>q[0])*(q[i-1]-q[i])))&0xf8)<<13)
    //           ) & RANGER_MASK;
    // }
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
