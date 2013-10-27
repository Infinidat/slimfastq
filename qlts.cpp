//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#include "qlts.hpp"
#include <assert.h>
// ?:
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <algorithm>

size_t QltBase::ranger_cnt() {
    return
        conf.level == 1 ? RANGER_SIZE_1 :
        // conf.level == 3 ? RANGER_SIZE_3 :
                          RANGER_SIZE_2 ;
}

void QltBase::range_init() {
    bzero(ranger, sizeof(ranger[0]) * ranger_cnt());
}

// enum FQQ_STMP {
//     FQQ_STMP_TREE = 0x7ababa00,
//     FQQ_STMP_DATA = 0x7adada00, // last byte used to store algorithm
// };

QltSave::QltSave()  {
    filer  = new FilerSave(conf.open_w("qlt"));
    ranger = new Log64Ranger[ranger_cnt()];
    assert(filer);
    assert(ranger);
    rcoder.init(filer);
    range_init();
}

QltSave::~QltSave() {
    rcoder.done();
    DELETE(filer);
    DELETE(ranger);
}

bool QltSave::is_valid() {
    return
        m_valid and
        filer and
        filer->is_valid();
}

void QltSave::save_1(const UCHAR* buf, size_t size) {
    UINT32 last = 0;
    for (size_t i = 0; i < size; i++) {
        UCHAR b = UCHAR(buf[i] - '!');
        rarely_if(b >= 63)
            croak("Illegal quality value 0x%x. Aborting\n", buf[i]);

        ranger[last].put(&rcoder, b);
        last = calc_last_1(last, b); 
    }
}

void QltSave::save_2(const UCHAR* buf, size_t size) {
    UINT32 last = 0;
    for (size_t i = 0; i < size; i++) {
        UCHAR b = UCHAR(buf[i] - '!');
        rarely_if(b >= 63)
            croak("Illegal quality value 0x%x. Aborting\n", buf[i]);

        PREFETCH(ranger + last);
        ranger[last].put(&rcoder, b);
        last = calc_last_2(last, b); 
    }
}

void QltSave::save_3(const UCHAR* buf, size_t size) {
    UINT32 last = 0;
    UINT32 delta = 5;
    UCHAR q1 = 0, q2 = 0;
    UINT32 di = 0;

// #define KILLER_BEE
#ifdef KILLER_BEE
    size_t len = size;
    while (len and buf[len-1] == '#')
        -- len;

#define SIZE len
#else
#define SIZE size
#endif

    for (size_t i = 0;;) {
        UCHAR b = UCHAR(buf[i++] - '!');

        rarely_if(b >= 63)
            croak("Illegal quality value 0x%x. Aborting\n", buf[i-1]);

        PREFETCH(ranger + last);
        ranger[last].put(&rcoder, b);

        rarely_if(i >= SIZE)
            break;

        if (++ di & 1) {
            last = calc_last_delta(delta, b, q1, q2);
            q2 = b;
        }
        else {
            last = calc_last_delta(delta, b, q2, q1);
            q1 = b;
        }
    }

#ifdef KILLER_BEE
    if (len != size)
        ranger[last].put(&rcoder, 63);
#endif

}

//////////
// load //
//////////

QltLoad::QltLoad() {
    filer = new FilerLoad(conf.open_r("qlt"), &m_valid);
    ranger = new Log64Ranger[ranger_cnt()];

    rcoder.init(filer);
    range_init();
}

QltLoad::~QltLoad() {
    rcoder.done();
    DELETE(filer);
    DELETE(ranger);
}

bool QltLoad::is_valid() {
    return
        m_valid and
        filer and
        filer->is_valid();
}

UINT32 QltLoad::load_1(UCHAR* buf, const size_t size) {

    UINT32 last = 0 ;
    for (size_t i = 0; i < size ; i++) {

        UCHAR b = ranger[last].get(&rcoder);
        buf[i] = UCHAR('!' + b);

        last = calc_last_1(last, b);
    }
    return m_valid ? size : 0;
}

UINT32 QltLoad::load_2(UCHAR* buf, const size_t size) {

    UINT32 last = 0 ;
    for (size_t i = 0; i < size ; i++) {

        PREFETCH(ranger + last);
        UCHAR b = ranger[last].get(&rcoder);
        buf[i] = UCHAR('!' + b);

        last = calc_last_2(last, b);
    }
    return m_valid ? size : 0;
}

UINT32 QltLoad::load_3(UCHAR* buf, const size_t size) {

    UINT32 last = 0 ; 
    UINT32 delta = 5;
    UCHAR  q1 = 0, q2 = 0;
    UINT32 di = 0;
    
    for (size_t i = 0;;) {

        PREFETCH(ranger + last);
        UCHAR b = ranger[last].get(&rcoder);

#ifdef KILLER_BEE
        rarely_if(b == 63) {
            for (; i < size; i++)
                buf[i] = '#';
        }
#endif

        buf[i++] = UCHAR('!' + b);

        rarely_if(i >= size)
            break;

        if (++di & 1) {
            last = calc_last_delta(delta, b, q1, q2);
            q2 = b;
        }
        else {
            last = calc_last_delta(delta, b, q2, q1);
            q1 = b;
        }
    }
    return m_valid ? size : 0;
}

