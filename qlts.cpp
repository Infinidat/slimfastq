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
        conf.level == 3 ? RANGER_SIZE_3 :
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
// #define KILLER_BEE
    UINT32 last = 0;
    // UCHAR q1 = 0, q2 = 0;
#ifdef KILLER_BEE
    size_t len = size;
    while (len and buf[len-1] == '#')
        -- len;

#define SIZE len
#else
#define SIZE size
#endif

    // UINT32 delta = 5;
    size_t i = 0;
    // for (; i < SIZE and i < 8; i++)
    // {
    //     UCHAR b = UCHAR(buf[i] - '!');
    // 
    //     rarely_if(b >= 63)
    //         croak("Illegal quality value 0x%x. Aborting\n", buf[i]);
    // 
    //     PREFETCH(ranger + last);
    //     ranger[last].put(&rcoder, b);
    //     //        last = ((last <<6) + b) & RANGER_MASK;
    //     // calc_last(last, b, q1, q2, i);
    //     last = calc_last(last, b); 
    //     // PREFETCH(ranger + last);
    // }

    for (; i < SIZE; i++) {
        UCHAR b = UCHAR(buf[i] - '!');

        rarely_if(b >= 63)
            croak("Illegal quality value 0x%x. Aborting\n", buf[i]);

        PREFETCH(ranger + last);
        ranger[last].put(&rcoder, b);
        //        last = ((last <<6) + b) & RANGER_MASK;
        // calc_last(last, b, q1, q2, i);
        // last = calc_last(last, buf, i);
        last = calc_last_3(last, b); 
        // last = calc_last(delta, buf, i);
        // PREFETCH(ranger + last);
    }

#ifdef KILLER_BEE
    if (len != size)
        ranger[last].put(&rcoder, 63);
#endif

    // while (size) {
    //     rarely_if (bucket.index >= BUCKET_SIZE)
    //         save_bucket();
    // 
    //     size_t  num = std::min(size, (BUCKET_SIZE - bucket.index));
    //     size -= num;
    //     for (size_t i = 0 ; i < num; i++) {
    //         UCHAR c = buf[i];
    //         bucket.buf [bucket.index++] = c ;
    //         bucket.hist[FQQ_ALGO_SELF ] [ c ] ++;
    //     }
    //     buf += num;
    // }
#undef KILLER_BEE
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
// #define KILLER_BEE
    UINT32 last = 0 ;// , delta=5;
    // UCHAR q1 = 0, q2 = 0;
    for (size_t i = 0; i < size ; i++) {

        PREFETCH(ranger + last);
        UCHAR b = ranger[last].get(&rcoder);

#ifdef KILLER_BEE
        likely_if(b < 63)
            buf[i] = UCHAR('!' + b);
        else 
            for (; i < size; i++)
                buf[i] = '#';
#else
        buf[i] = UCHAR('!' + b);
#endif
        // last = calc_last(last, b); 
        // last = calc_last(last, buf, i); 
        last = calc_last_3(last, b);
        // last = calc_last(delta, buf, i);
    }
    return m_valid ? size : 0;
}
