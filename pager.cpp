//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#include "pager.hpp"
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

PagerLoad::PagerLoad(FILE* fh, bool* valid_ptr) {
    m_in = fh;
    m_valid_ptr = valid_ptr;
    * valid_ptr = true;
    m_valid = true ;
    m_page_count = 0;
    load_page();

    m_cur_long = m_cur_indx = 0;
}

PagerLoad::~PagerLoad() {
    fclose(m_in);
}

bool PagerLoad::is_valid() { return m_valid; }

UINT64 PagerLoad::tell() const {
    return
        ( m_page_count )
        ? 
        ((m_page_count-1) * PAGER_PAGE) +
        ( m_cur * sizeof(m_buff[0])) -
        ( m_cur_indx )
        :
        0;
}

void PagerLoad::load_page() {
    rarely_if(not m_valid)
        return;
    size_t cnt = fread(m_buff, sizeof(m_buff[0]), PAGER_PAGE, m_in);
    rarely_if (cnt == 0) {
        *m_valid_ptr = m_valid = false;
        return;
    }
    m_cur = 0;
    m_count = cnt;
    m_page_count++;
}

UINT64 PagerLoad::get() {
    if (m_count <= m_cur)
        load_page();
    return
        m_valid ? m_buff[m_cur++] : 0;     // caller should check is_valid()
}

PagerSave::PagerSave(FILE* fh) {
    m_out = fh;
    m_valid = true;
    m_cur = 0;
    m_page_count = 0;

    m_cur_long = m_cur_indx = 0;
}

PagerSave::~PagerSave() {
    if (m_cur_indx)
        put(m_cur_long);

    save_page();
    fclose(m_out);
}

bool PagerSave::is_valid() { return m_valid; }

void PagerSave::save_page() {
    if (not m_valid or
        not m_cur)
        return ;

    if (m_buff[PAGER_PAGE] == -1ULL) 
        fprintf(stderr, "breakpoint");


    assert(m_cur <= PAGER_PAGE);
    size_t cnt = fwrite(m_buff, sizeof(m_buff[0]), m_cur, m_out);
    if (cnt != m_cur) {
        fprintf(stderr, "Error saving pager: %s m_cur=%lu cnt=%lu\n", strerror(errno), m_cur, cnt);
        exit(1); // remove and handle error at production
        m_valid = false;
    }
    m_cur = 0;
    m_page_count++;
}

bool PagerSave::put(UINT64 n) {
    if (m_cur >= PAGER_PAGE)
        save_page();
    m_buff[m_cur++] = n;
    return m_valid;
}

