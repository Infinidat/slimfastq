//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#include "fq_qlts.hpp"
#include <assert.h>
// ?:
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <algorithm>

void QltBase::range_init() {
    bzero(ranger, sizeof(ranger[0])*RANGER_SIZE);
}

enum FQQ_STMP {
    FQQ_STMP_TREE = 0x7ababa00,
    FQQ_STMP_DATA = 0x7adada00, // last byte used to store algorithm
};

QltSave::QltSave(const Config* conf )  { // : FQQBase() {
    m_conf = conf;
    filer  = new FilerSave(m_conf->open_w("qlt"));
    ranger = new Log64Ranger[RANGER_SIZE];
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

// #define KILLER_BEE 1
void QltSave::save(const UCHAR* buf, size_t size) {
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
        last = calc_last(last, b); 
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
}

QltLoad::QltLoad(const Config* conf) { // : FQQBase() {
    filer = new FilerLoad(conf->open_r("qlt"), &m_valid);
    ranger = new Log64Ranger[RANGER_SIZE];
    // bzero(&bucket, sizeof(bucket));

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

// #define CHECK_VALID if (not m_valid) return
// void QltLoad::load_tree() {
//     UINT32 hist[MAX_CHARS] ;
//     bzero(hist, sizeof(hist));
// 
//     // UINT64 n = pager->get();
//     // CHECK_VALID;
//     // UINT32 stmp = (n>>32) & MASK_STAMP;
//     UINT32 stmp = filer->get4();
//     assert(stmp == FQQ_STMP_TREE);
//     // bucket.tree_size = n & MASK_32;
//     // assert(bucket.tree_size <= MAX_CHARS);
//     bucket.tree_size = filer->get();
// 
//     for (UCHAR i = 0; i < bucket.tree_size; i++)
//         hist[filer->get()] = filer->get4();
// 
// // {
// //         n = pager->get();
// //         hist[(n>>32) & MASK_08] = n & MASK_32;
// //     }
//         // hist[pager->get()] = pager->get32();
// 
//     CHECK_VALID;
//     int num_verify;
//     set_tree(new_tree(hist, &num_verify));
//     if (DEBUG) {
//         BZERO(m_cbits);
//         fill_cbits(m_tree);
//     }
//     assert (num_verify == bucket.tree_size);
// }
 
// void QltLoad::load_bucket() {
// 
//     bzero(&bucket, sizeof(bucket));
// 
//     // load_tree();
//     // UINT64 n = pager->get();
//     // CHECK_VALID;
//     // 
//     // UINT32 stamp = (n>>32) & MASK_STAMP ;
//     // assert(stamp == FQQ_STMP_DATA);
//     // bucket.algo = (FQQ_ALGO) ((n>>32) & MASK_08);
//     // bucket.size = n & MASK_32;
//     // 
//     // n = pager->get();
//     // bucket.hist_first = (n>>32) & MASK_32;
//     // bucket.hist_range = n & MASK_32;
//     UINT32 stmp = filer->get4();
//     assert(stmp == FQQ_STMP_DATA);
//     bucket.algo       = filer->get();
//     bucket.hist_first = filer->get();
//     bucket.hist_range = filer->get();
//     bucket.hist_lastc = bucket.hist_first + bucket.hist_range;
//     bucket.size = filer->get4();
// 
//     rcoder.init(filer);
//     range_init();
// 
//     // m_wrd = 0;
//     // m_bit = 0;
// }

// void QltLoad::get_w() {
//     m_wrd = pager->get();
//     m_bit = 64;
// }
// 
// bool QltLoad::get_b() {
//     if (m_bit == 0)
//         get_w();
// 
//     return !! (m_wrd & (1ULL << (64 - (m_bit--))));
// }

// void QltLoad::range_init() {
// 
//     bzero(ranger, sizeof(ranger[0])*RANGER_SIZE);
//     // for (int i = 0; i < RCARR_SIZE; i++)
//     //     // TODO: faster init (bzero?)
//     //     rcarr[i].init();
//     // rcarr_last = 0;
// }

// UCHAR QltLoad::get_char() {
//     UCHAR b = rcarr[rcarr_last].decodeSymbol(&rcoder);
//     rcarr_last = ((rcarr_last <<6) + b) & RCARR_MASK;
//     bucket.index++;
// 
//     // HFNode* node = m_tree;
//     // UINT32   sanity = 64;
//     // while (--sanity) {
//     //     if (node->is_leaf) {
//     //         bucket.index++;
//     //         return node->symbol;
//     //     }
//     //     node =
//     //         get_b() ?
//     //         node->one :
//     //         node->zero;
//     // }
//     // assert(!is_valid());
//     return b;                   // happy compiler
// }

// UCHAR QltLoad::algo_self(UCHAR o) const {
//     return UCHAR(o + '!');
// }
// 
// UCHAR QltLoad::algo_prev(UCHAR o, UCHAR p) const {
// 
//     UINT32 n = p + o;           // avoid UCHAR wrapp around
//     return
//         n < bucket.hist_lastc ?
//         (UCHAR) n             :
//         n - bucket.hist_lastc +
//             bucket.hist_first ;
// }
// 
// UCHAR QltLoad::algo_most(UCHAR o, UCHAR p1, UCHAR p2, UCHAR p3) const {
//     return
//         algo_prev(o,
//                   bucket.index >= 3  ?
//                   p2 == p3 ? p2 : p1 :
//                   bucket.index       ?
//                   p1                 :
//                   bucket.hist_first  );
// }

UINT32 QltLoad::load(UCHAR* buf, const size_t size) {

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
        // last = ((last <<6) + b) & RANGER_MASK;
        // calc_last(last, b, q1, q2, i);
        // last = calc_last(last, b); 
        // last = calc_last(last, buf, i); 
        last = calc_last(last, b);
        // last = calc_last(delta, buf, i);
        // PREFETCH(ranger + last);
    }
    return m_valid ? size : 0;

    // assert(size > 3);           // I'm too lazy to handle all the special cases
    // assert(filer->is_valid());
    // 
    // size_t offset = 0;
    // while (offset < size) {
    // 
    //     if (bucket.index >= bucket.size)
    //         load_bucket();
    // 
    //     CHECK_VALID offset;
    // 
    //     size_t limit =
    //         bucket.size - bucket.index + offset < size ?
    //         bucket.size - bucket.index + offset : size ;
    // 
    //     switch(bucket.algo) {
    // 
    //     case FQQ_ALGO_SELF: {
    //         for (;offset < limit; offset++)
    //             buf[offset] = algo_self(get_char());
    //     } break;
    // 
    //     case FQQ_ALGO_PREV: {
    //         rarely_if(bucket.index == 0) {
    //             // new bucket, oh bother
    //             buf[offset++] = algo_prev(get_char(), bucket.hist_first );
    //             if (offset >= limit)
    //                 break;      // case
    //         }
    //         if (offset == 0)
    //             buf[offset++] = algo_prev(get_char(), bucket.prev[0]);
    // 
    //         for (;offset < limit; offset++)
    //             buf[offset] = algo_prev(get_char(), buf[offset-1]); 
    // 
    //     } break;
    // 
    //     case FQQ_ALGO_MOST: {
    //         rarely_if(bucket.index < 3) {
    //             for (; bucket.index < 3 and offset < size; offset++) {
    //                 buf[offset] = algo_prev(get_char(),
    //                                         bucket.index ?
    //                                         offset ?
    //                                         buf[offset-1] :
    //                                         bucket.prev[0] :
    //                                         bucket.hist_first);
    //             }
    //             if(offset >= limit)
    //                 break;      // case
    //         }
    // 
    //         if (offset == 0)
    //             buf[offset++] = algo_most(get_char(),
    //                                       bucket.prev[0],
    //                                       bucket.prev[1],
    //                                       bucket.prev[2]);
    //         if (offset == 1)
    //             buf[offset++] = algo_most(get_char(),
    //                                       buf[0],
    //                                       bucket.prev[0],
    //                                       bucket.prev[1]);
    //         if (offset == 2)
    //             buf[offset++] = algo_most(get_char(),
    //                                       buf[1],
    //                                       buf[0],
    //                                       bucket.prev[0]);
    //         
    //         for (; offset < limit; offset++)
    //             buf[offset] = algo_most(get_char(),
    //                                     buf[offset-1],
    //                                     buf[offset-2],
    //                                     buf[offset-3] );
    //     } break;
    // 
    //     case FQQ_ALGO_LAST_DUMMY:
    //     default: assert(0);
    //     }
    // }
    // for (int i = 0; i < 3; i++)
    //     bucket.prev[i] = buf[offset-1-i];
    // 
    // return offset;
}

