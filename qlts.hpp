
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

class QltBase {
protected:
    Log64Ranger* ranger;
    RCoder rcoder;
    bool m_valid;
    // const Config* m_conf;

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
    inline UINT32 min_val(UINT32 a, UINT32 b) { return a>b?b:a; }
    inline UCHAR  max_val(UCHAR  a, UCHAR  b) { return a>b?a:b; }
    inline UINT32 calc_last(UINT32 &delta, const UCHAR* q, size_t i) {
    
        return 
            ( RARELY(i < 2) ?
              0 : 
              (q[i]-'!')
              | 
              ((max_val(q[i-1], q[i-2])-'!')<<6 )
              |
              ((q[i-1] == q[i-2]) << 12)
              |
              ((q[i-1]>q[0]) ? 
               (min_val(7, (delta += q[i-1]-q[i])/8)<<13)
              :
              0 )
              ) & RANGER_MASK;
    }
};

class QltSave : private QltBase {
public:
    QltSave();
    ~QltSave();

    void save(const UCHAR* buf, size_t size);
    // void filer_init();
    bool is_valid();
private:
    FilerSave* filer;
};

class QltLoad : private QltBase {
public:
    QltLoad();
    ~QltLoad();

    UINT32 load (UCHAR* buffer, const size_t size);
    bool is_valid();
private:
    FilerLoad* filer;
};

#endif  // FQ_QLTS_H
