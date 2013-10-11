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

// local definitions
#define MASK_STAMP 0xffffff00
#define MASK_32    0xffffffff
#define MASK_08    0xff

struct HFNode {

    UINT32 count;
    HFNode *parent, *zero, *one;
    UCHAR symbol;
    bool is_leaf;
    HFNode(UCHAR c, UINT32 cnt) {
        count = cnt;
        symbol = c;
        parent = zero = one = NULL;
        is_leaf = true;
    }
    HFNode(HFNode* left, HFNode* right) {
        assert(left && right);
        count = left->count + right->count;
        symbol = 0;
        zero = left;
        one  = right;
        parent = NULL;
        zero->parent = this;
        one ->parent = this;
        is_leaf = false;
    }
    ~HFNode() {
        if (zero) delete zero;
        if (one ) delete one ;
    }
    // bool is_leaf() {
    //     // assert(!!zero ? !!one : !one);
    //     return zero == NULL;
    // }
};

static int compare_hist(const void* a, const void* b) {
    const HFNode* aa = *(const HFNode**) a;
    const HFNode* bb = *(const HFNode**) b;
    return
        aa == NULL  ?
        bb == NULL  ? 0 : 1 :
        bb == NULL  ?    -1 :
        aa->count > bb->count ?  1 :
        aa->count < bb->count ? -1 :
        0 ;
}

FQQBase::FQQBase() {
    m_tree = NULL;
    m_valid = true;

    // m_bit = 0;
    // m_wrd = 0;
}

FQQBase::~FQQBase() {
    set_tree(NULL);
}

void FQQBase::fill_cbits(HFNode* leaf) {

    if (leaf == NULL)
        ;
    else if (leaf->is_leaf) {
        UCHAR  nbits = 0;
        UINT64 vbits = 0;

        HFNode* node = leaf;
        while ( node->parent) {
            // Order:
            // LSB is level zero
            HFNode* par = node->parent;

            if (node == par->one)
                vbits |= (1ULL<<nbits);

            nbits ++ ;
            node = par ;
        }
        assert(nbits <= 64);

        m_cbits[leaf->symbol].nbits = nbits;
        m_cbits[leaf->symbol].vbits = vbits;
    }
    else {
        fill_cbits(leaf->zero);
        fill_cbits(leaf->one );
    }
}

void FQQBase::dump_cbits() {
    for (int i = 0, j=0; i < MAX_CHARS; i++)
        if (m_cbits[i].nbits > 0) 
            fprintf(stderr,
                    ( i < 33 ?
                      "%d: %2u:0x%08llx :: %s" :                      
                      "'%c' %2u:0x%08llx :: %s"
                     ), i, m_cbits[i].nbits, m_cbits[i].vbits, ++j%4?"":"\n");

    fprintf(stderr, "\n");
}

void FQQBase::set_tree(HFNode* tree) {
    if (m_tree)
        delete m_tree;
    m_tree = tree;
}

static HFNode* new_tree(UINT32* hist, int* p_num) {
    HFNode* nodes[MAX_CHARS] = {0};
    int num = 0 ;
    for (int i = 0; i < MAX_CHARS; i++)
        if (hist[i])
            nodes[num++] = new HFNode((UCHAR)i, hist[i]);

    assert(num && num < MAX_CHARS); // already mem-corrupted, but better than nothing

    qsort(nodes, num, sizeof(nodes[0]), compare_hist);
    for (int i = 0; i < num-1; i++) {

        nodes[0] = new HFNode(nodes[0], nodes[1]);
        nodes[1] = NULL;
        qsort(nodes, num, sizeof(nodes[0]), compare_hist);
        // num - i + 1 ?
    }
    assert(nodes[0] and not nodes[1]);
    if ( p_num )
        *p_num = num;

    return nodes[0];
}

enum FQQ_STMP {
    FQQ_STMP_TREE = 0x7ababa00,
    FQQ_STMP_DATA = 0x7adada00, // last byte used to store algorithm
};

QltSave::QltSave(const Config* conf ) : FQQBase() {
    // filer = NULL ;
    m_conf = conf;
    filer = new FilerSave(m_conf->open_w("qlt"));
    rcarr = new SIMPLE_MODEL<64>[RCARR_SIZE];
    assert(rcarr);

    clear_bucket();
    BZERO(stats);
}

QltSave::~QltSave() {
    save_bucket();
    fprintf(stderr, "::: QLT Algorithm  histogram: SELF:%u | PREV:%u | MOST:%u\n",
            stats.algo_hist[FQQ_ALGO_SELF],
            stats.algo_hist[FQQ_ALGO_PREV],
            stats.algo_hist[FQQ_ALGO_MOST] );
    DELETE(filer);
}

bool QltSave::is_valid() {
    return
        m_valid and
        filer and
        filer->is_valid();
}

void QltSave::filer_init() {
    assert(0); // TODO:
    save_bucket();
    DELETE(filer);
    filer = new FilerSave(m_conf->open_w("qlt"));
}

// bool QltSave::pager_put(UINT64 word) {
// 
//     return pager->put(word);
// }

// void QltSave::save_tree(const UINT32* hist, UCHAR num) {
// 
//     filer->put4(FQQ_STMP_TREE);
//     filer->put (num);
// 
//     int num_verify=0;
//     for (int i = 0 ; i < MAX_CHARS; i++)
//         if (hist[i]) {
//             num_verify++;
//             filer->put(i);
//             filer->put4(hist[i]);
//         }
//     assert(num_verify == num);
// }

// void QltSave::put_w() {
//     pager_put(m_wrd);
//     m_bit = 0;
//     m_wrd = 0;
// }
// 
// void QltSave::flush() {
//     // if (m_bit)
//     //     put_w();
// }

void QltSave::put_char(UCHAR b) {

    rcarr[rcarr_last].encodeSymbol(&rcoder, b);
    rcarr_last = ((rcarr_last <<6) + b) & RCARR_MASK;
    
    // const HFBits &cb = m_cbits[b];
    // // je-TODO (one day): can we do the whole chunk with mask?
    // // something like:
    // // if (m_bits + cb.nbits <=64) {
    // //     m_wrd |= cb.vbits << m_bit; <<== must pre-reverse vbits for this noe
    // //     m_bit += cb.nbits ; 
    // //     if (++ m_bit >= 64)
    // //         put_w();
    // // }
    // // else
    // for (int n = cb.nbits-1; n >= 0; n --) {
    // 
    //     if (cb.vbits & (1ULL<<n))
    //         // reverting bits on the way ...
    //         m_wrd |= (1ULL<<m_bit);
    // 
    //     // if (++ m_bit >= 64)
    //     //     put_w();
    // }
}

UCHAR QltSave::algo_self(size_t i) const {
    return UCHAR(bucket.buf[i] - '!');
}

inline static UCHAR hist_wrapper(UCHAR c, UCHAR p, UCHAR w) {
    return c >= p ? c - p : (UINT32)c + w - p;
}

UCHAR QltSave::algo_prev(size_t i) const {
    return
        LIKELY(i) ?
        hist_wrapper(bucket.buf[i  ],
                     bucket.buf[i-1],
                     bucket.hist_range ) :
        bucket.buf[i] - bucket.hist_first;
}

UCHAR QltSave::algo_most(size_t i) const {
    return
        LIKELY(i >= 3) ?
        hist_wrapper(bucket.buf[i  ],
                     bucket.buf[i-2] == bucket.buf[i-3] ? bucket.buf[i-2] : bucket.buf[i-1],
                     bucket.hist_range) :
        algo_prev(i);
}

// UCHAR QltSave::algo_prev_prev(size_t i, UCHAR prev) const {
//     return
//         hist_wrapper(algo_prev(i),
//                      prev,
//                      bucket.hist_range);
// }

void QltSave::range_init() {

    for (int i = 0; i < RCARR_SIZE; i++)
        // TODO: faster init (bzero?)
        rcarr[i].init();
    rcarr_last = 0;
}

void QltSave::clear_bucket() {
    bucket.index = 0;
    bucket.hist_range = 0;
    bucket.hist_first = 0;
    bzero(bucket.hist, sizeof(bucket.hist));
}

static UINT64 sum_cbits(HFNode* leaf) {
    if (leaf == NULL)
        return 0 ;
    else if (leaf->is_leaf) {

        UCHAR  nbits = 0;
        HFNode* node = leaf;
        while ( node->parent) {
            nbits ++ ;
            node = node->parent;
        }
        assert(nbits <= 64);
        return nbits * leaf->count;
    }
    else
        return
            sum_cbits(leaf->zero) +
            sum_cbits(leaf->one ) ;
}

static void dump_hists(UINT32 hist[][256]) {
    // debug
    for (int algo = 0 ; algo < FQQ_ALGO_LAST_DUMMY; algo++ ) {
        fprintf(stderr, "algo: %d\n", algo);

        for (int i = 0, j=0; i < MAX_CHARS; i++)
            if (hist[algo][i]) 
                fprintf(stderr,
                        ( i < 33 ? "%2d: %5u | %s" : "'%c' %5u | %s" ),
                        i, hist[algo][i], ++j%8?"":"\n");

    fprintf(stderr, "\n");
    }
}

void QltSave::determine_algo_n_tree(FQQ_ALGO* p_algo, int* p_num) {
    
    // TODO:
    //  if (user_option_fast) return FQQ_ALGO_SELF;

    // calc range
    int idx;
    for ( idx = 0;
          idx < MAX_CHARS
              and bucket.hist[FQQ_ALGO_SELF][idx] == 0;
          idx++);
    assert(idx < MAX_CHARS);
    bucket.hist_first = idx;
    for ( idx = MAX_CHARS - 1;
          idx > bucket.hist_first
              and bucket.hist[FQQ_ALGO_SELF][idx] == 0;
          idx --);
    bucket.hist_range = 1 + idx - bucket.hist_first ;

    //  calc alternative histograms 
    // assumes bucket.hist was already zeroed for non-FQQ_ALGO_SELF
    const size_t size = bucket.index;
    // UCHAR pprev = 0;
    for (size_t i = 0; i < size ; i ++) {
        bucket.hist[FQQ_ALGO_PREV][algo_prev(i)]++;
        bucket.hist[FQQ_ALGO_MOST][algo_most(i)]++;
        // bucket.hist[FQQ_ALGO_PREV_PREV]
        //     [ (pprev = algo_prev_prev(i, pprev)) ] ++;
    }
    if (0)
        dump_hists(bucket.hist);

    FQQ_ALGO algo = FQQ_ALGO_SELF;
    UINT64 bits_n = 0;
    int tree_num  = 0;

    for (int i = 0; i < FQQ_ALGO_LAST_DUMMY; i++) {
        FQQ_ALGO cand = (FQQ_ALGO)i;
        int num;
        HFNode* tree = new_tree(&(bucket.hist[cand][0]), &num);
        UINT64 n = sum_cbits(tree);
        if (bits_n == 0 or
            bits_n >  n) {
            bits_n =  n;
            set_tree(tree);
            tree_num = num;
            algo = cand;
        }
        else
            delete tree;
    }
    *p_num  = tree_num;
    *p_algo = algo;
}

void QltSave::save_bucket() {

    if (bucket.index == 0)
        return;

    FQQ_ALGO algo;
    int tree_num ;
    determine_algo_n_tree(&algo, &tree_num);

    assert(m_tree);
    BZERO (m_cbits);
    fill_cbits(m_tree);

   // fprintf(stderr, "%s\n", algo == FQQ_ALGO_SELF ? "self": algo  == FQQ_ALGO_MOST ? "most" : "prev");
    stats.algo_hist[algo] ++;

    // save tree
    // save_tree(bucket.hist[algo], tree_num);

    const size_t size = bucket.index;
    // save header
    // UINT64 n = FQQ_STMP_DATA | algo;
    // pager_put((n<<32)|size);
    // n = bucket.hist_first;
    // pager_put((n<<32)|bucket.hist_range);
    filer->put4(FQQ_STMP_DATA);
    filer->put(algo);
    filer->put(bucket.hist_first);
    filer->put(bucket.hist_range);
    filer->put4(size);

    rcoder.init(filer);
    range_init();

    // save data
    switch (algo) {

    case FQQ_ALGO_SELF: {
        for (UINT32 i = 0; i < size; i ++)
            put_char(algo_self(i));
    } break;

    case FQQ_ALGO_PREV: {
        for (UINT32 i = 0; i < size; i ++)
            put_char(algo_prev(i));
    } break;

    case FQQ_ALGO_MOST: {
        for (UINT32 i = 0; i < size; i++) 
            put_char(algo_most(i));
    } break;

    case FQQ_ALGO_LAST_DUMMY:
    default: assert(false);
    }
    
    rcoder.done();
    // flush();

    clear_bucket();
}

void QltSave::save(const UCHAR* buf, size_t size) {

    while (size) {
        rarely_if (bucket.index >= BUCKET_SIZE)
            save_bucket();

        size_t  num = std::min(size, (BUCKET_SIZE - bucket.index));
        size -= num;
        for (size_t i = 0 ; i < num; i++) {
            UCHAR c = buf[i];
            bucket.buf [bucket.index++] = c ;
            bucket.hist[FQQ_ALGO_SELF ] [ c ] ++;
        }
        buf += num;
    }
}

QltLoad::QltLoad(const Config* conf) : FQQBase() {
    filer = new FilerLoad(conf->open_r("qlt"), &m_valid);
    rcarr = new SIMPLE_MODEL<64>[RCARR_SIZE];
    bzero(&bucket, sizeof(bucket));
}

QltLoad::~QltLoad() {
    DELETE(filer);
}

#define CHECK_VALID if (not m_valid) return
bool QltLoad::is_valid() {
    return m_valid and filer and filer->is_valid();
}

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
 
void QltLoad::load_bucket() {

    bzero(&bucket, sizeof(bucket));

    // load_tree();
    // UINT64 n = pager->get();
    // CHECK_VALID;
    // 
    // UINT32 stamp = (n>>32) & MASK_STAMP ;
    // assert(stamp == FQQ_STMP_DATA);
    // bucket.algo = (FQQ_ALGO) ((n>>32) & MASK_08);
    // bucket.size = n & MASK_32;
    // 
    // n = pager->get();
    // bucket.hist_first = (n>>32) & MASK_32;
    // bucket.hist_range = n & MASK_32;
    UINT32 stmp = filer->get4();
    assert(stmp == FQQ_STMP_DATA);
    bucket.algo       = filer->get();
    bucket.hist_first = filer->get();
    bucket.hist_range = filer->get();
    bucket.hist_lastc = bucket.hist_first + bucket.hist_range;
    bucket.size = filer->get4();

    rcoder.init(filer);
    range_init();

    // m_wrd = 0;
    // m_bit = 0;
}

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

void QltLoad::range_init() {

    for (int i = 0; i < RCARR_SIZE; i++)
        // TODO: faster init (bzero?)
        rcarr[i].init();
    rcarr_last = 0;
}

UCHAR QltLoad::get_char() {
    UCHAR b = rcarr[rcarr_last].decodeSymbol(&rcoder);
    rcarr_last = ((rcarr_last <<6) + b) & RCARR_MASK;

    // HFNode* node = m_tree;
    // UINT32   sanity = 64;
    // while (--sanity) {
    //     if (node->is_leaf) {
    //         bucket.index++;
    //         return node->symbol;
    //     }
    //     node =
    //         get_b() ?
    //         node->one :
    //         node->zero;
    // }
    // assert(!is_valid());
    return b;                   // happy compiler
}

UCHAR QltLoad::algo_self(UCHAR o) const {
    return UCHAR(o + '!');
}

UCHAR QltLoad::algo_prev(UCHAR o, UCHAR p) const {

    UINT32 n = p + o;           // avoid UCHAR wrapp around
    return
        n < bucket.hist_lastc ?
        (UCHAR) n             :
        n - bucket.hist_lastc +
            bucket.hist_first ;
}

UCHAR QltLoad::algo_most(UCHAR o, UCHAR p1, UCHAR p2, UCHAR p3) const {
    return
        algo_prev(o,
                  bucket.index >= 3  ?
                  p2 == p3 ? p2 : p1 :
                  bucket.index       ?
                  p1                 :
                  bucket.hist_first  );
}

UINT32 QltLoad::load(UCHAR* buf, const size_t size) {

    assert(size > 3);           // I'm too lazy to handle all the special cases

    size_t offset = 0;
    while (offset < size) {

        if (bucket.index >= bucket.size)
            load_bucket();

        CHECK_VALID offset;

        size_t limit =
            bucket.size - bucket.index + offset < size ?
            bucket.size - bucket.index + offset : size ;

        switch(bucket.algo) {

        case FQQ_ALGO_SELF: {
            for (;offset < limit; offset++)
                buf[offset] = algo_self(get_char());
        } break;

        case FQQ_ALGO_PREV: {
            rarely_if(bucket.index == 0) {
                // new bucket, oh bother
                buf[offset++] = algo_prev(get_char(), bucket.hist_first );
                if (offset >= limit)
                    break;      // case
            }
            if (offset == 0)
                buf[offset++] = algo_prev(get_char(), bucket.prev[0]);

            for (;offset < limit; offset++)
                buf[offset] = algo_prev(get_char(), buf[offset-1]); 

        } break;

        case FQQ_ALGO_MOST: {
            rarely_if(bucket.index < 3) {
                for (; bucket.index < 3 and offset < size; offset++) {
                    buf[offset] = algo_prev(get_char(),
                                            bucket.index ?
                                            offset ?
                                            buf[offset-1] :
                                            bucket.prev[0] :
                                            bucket.hist_first);
                }
                if(offset >= limit)
                    break;      // case
            }

            if (offset == 0)
                buf[offset++] = algo_most(get_char(),
                                          bucket.prev[0],
                                          bucket.prev[1],
                                          bucket.prev[2]);
            if (offset == 1)
                buf[offset++] = algo_most(get_char(),
                                          buf[0],
                                          bucket.prev[0],
                                          bucket.prev[1]);
            if (offset == 2)
                buf[offset++] = algo_most(get_char(),
                                          buf[1],
                                          buf[0],
                                          bucket.prev[0]);
            
            for (; offset < limit; offset++)
                buf[offset] = algo_most(get_char(),
                                        buf[offset-1],
                                        buf[offset-2],
                                        buf[offset-3] );
        } break;

        case FQQ_ALGO_LAST_DUMMY:
        default: assert(0);
        }
        rcoder.done();
    }
    for (int i = 0; i < 3; i++)
        bucket.prev[i] = buf[offset-1-i];

    return offset;
}

