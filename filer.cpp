
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "filer.hpp"

FilerLoad::FilerLoad(FILE* fh, bool* valid_ptr) {
    m_in = fh;
    m_valid_ptr = valid_ptr;
    * valid_ptr = m_valid = true ;
    m_page_count = 0;
    m_cur = m_count = 0;                // happy valgrind 
    load_page();
}

FilerLoad::~FilerLoad() {
    fclose(m_in);
}

bool FilerLoad::is_valid() const { return m_valid ; }

size_t FilerLoad::tell() const {
    return
        ( m_page_count  ) ?
        ((m_page_count-1) * FILER_PAGE) +
        ( m_cur ) :
        0;
}

void FilerLoad::load_page() {
    rarely_if(not m_valid)
        return;
    size_t cnt = fread(m_buff, sizeof(m_buff[0]), FILER_PAGE, m_in);
    rarely_if (cnt == 0) {
        *m_valid_ptr = m_valid = false;
        return;
    }
    m_cur = 0;
    m_count = cnt;
    m_page_count++;
}

// UINT64 FilerLoad::getN(int N) {
//     UINT64 val = 0;
//     for (int i = N-1; i>=0; i--)
//         val |= (UINT64(get()) << (8*i));
//     return val;
// }

FilerSave::FilerSave(FILE* fh) {
    m_out = fh;
    m_valid = true;
    m_cur = 0;
    m_page_count = 0;
}

FilerSave::~FilerSave() {
    save_page();
    fclose(m_out);
}

bool FilerSave::is_valid() const { return m_valid; }

void FilerSave::save_page() {
    if (not m_valid or
        not m_cur)
        return ;

    assert(m_cur <= FILER_PAGE);
    size_t cnt = fwrite(m_buff, sizeof(m_buff[0]), m_cur, m_out);
    if (cnt != m_cur) {
        fprintf(stderr, "Error saving pager: %s m_cur=%lu cnt=%lu\n", strerror(errno), m_cur, cnt);
        exit(1); // remove and handle error at production
        m_valid = false;
    }
    m_cur = 0;
    m_page_count++;
}


// bool FilerSave::putN(int N, UINT64 val) {
//     for (int i = N-1; i>=0 ; i--)
//         put( (val >> (8*i)) & 0xff );
//     return m_valid;
// }

// use it later
// UINT64 FilerLoad::getUL() {
//     UINT64 val = get();
//     likely_if (val < 128)
//         return val;
//     val <<= 8;
//     val  |= get();
//     likely_if (val < 0xfffe)
//         return (val & 0x7fff);
// 
//     int nb = (val == 0xfffe) ? 4 : 8;
//     val = 0;
//     for (int i = nb ; i ; i--)
//         val |= (get() << (i-1));
//     return val;
// }


