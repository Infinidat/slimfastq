
//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#ifndef FQ_QLTS_H
#define FQ_QLTS_H

#include "common.hpp"
#include "config.hpp"
#include "filer.hpp"
#include "power_ranger.hpp"

#define RANGER_SIZE (1<<16)
#define RANGER_MASK (RANGER_SIZE-1)

class QltBase {
protected:
    PowerRanger<6>* ranger;
    RangeCoder rcoder;
    bool    m_valid;
    const Config* m_conf;

    void range_init();
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
