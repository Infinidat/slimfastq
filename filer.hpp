



#ifndef COMMON_FILER_H
#define COMMON_FILER_H

#include "common.hpp"
#include <stdio.h>
#include <assert.h>

#define FILER_PAGE 0x2000

class FilerLoad {
public:
    FilerLoad(FILE* fh, bool *valid_ptr);
    ~FilerLoad();

    bool is_valid() const ;
    size_t    tell() const ;

    inline UCHAR get() {
        rarely_if(m_count <= m_cur) load_page();
        return m_valid ? m_buff[m_cur++] : 0;
    }

    UINT64    get4() { return getN(4);}
    UINT64    get8() { return getN(8);}

private:
    UINT64    getN(int N);
    void load_page();

    bool   m_valid;
    UCHAR  m_buff[ FILER_PAGE+10 ]; 
    size_t m_cur, m_count;
    FILE  *m_in;
    bool  *m_valid_ptr;
    UINT64 m_page_count; 
};

class FilerSave {
public:
    FilerSave(FILE* fh);
    ~FilerSave();
    bool is_valid() const ;
    bool put(UCHAR c) {
        rarely_if(m_cur >= FILER_PAGE)
            save_page();
        m_buff[m_cur++] = c;
        return m_valid;
    }
    bool put4(UINT64 val) { return putN(4, val);}
    bool put8(UINT64 val) { return putN(8, val);}

private:
    bool putN(int N, UINT64 val);
    void save_page();

    bool   m_valid;
    UCHAR  m_buff[ FILER_PAGE+10 ]; 
    size_t m_cur;
    FILE  *m_out;
    UINT64 m_page_count; 
};


#endif  // COMMON_FILER_H