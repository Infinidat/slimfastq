/***********************************************************************************************************************/
/* This program was written by Josef Ezra  <jezra@infinidat.com>                                                       */
/* Copyright (c) 2013, Infinidat                                                                                       */
/* All rights reserved.                                                                                                */
/*                                                                                                                     */
/* Redistribution and use in source and binary forms, with or without modification, are permitted provided that        */
/* the following conditions are met:                                                                                   */
/*                                                                                                                     */
/* Redistributions of source code must retain the above copyright notice, this list of conditions and the following    */
/* disclaimer.                                                                                                         */
/* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following */
/* disclaimer in the documentation and/or other materials provided with the distribution.                              */
/* Neither the name of the Infinidat nor the names of its contributors may be used to endorse or promote products      */
/* derived from this software without specific prior written permission.                                               */
/*                                                                                                                     */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,  */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE   */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,  */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR     */
/* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,   */
/* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE    */
/* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                            */
/***********************************************************************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "recs.hpp"

#define MAX_LLINE 400           // TODO: assert max is enough at constructor


void RecBase::range_init() {
    // const int cnt = conf.level > 2 ? N_RANGE_BIG : N_RANGE_SML;
    // ranger = new ranger_t [cnt];
    // // bzero(ranger, sizeof(ranger[0]) * cnt);
    // m_range_last = cnt-1;
}

RecSave::RecSave() {

    m_valid = true;

    BZERO(m_last);
    BZERO(stats);
    range_init();

    filer = new FilerSave(conf.open_w("rec"));
    assert(filer);
    rcoder.init(filer);

    smap[0].len = smap[1].len = 0;
}

RecSave::~RecSave() {
    rcoder.done();
    DELETE(filer);
    fprintf(stderr, "::: REC big int: %u | str num/sum: %u/%u | new line num/sum: %u/%u\n",
            stats.big_i, stats.str_n, stats.str_l, stats.new_n, stats.new_l );
}

UCHAR* sncpy(UCHAR* target, const UCHAR* source, int n) {
    int  i;
    for (i = 0 ; i < n and source[i]; i++)
        target[i] = source[i];
    target[i] = 0;
    return target + i;
}

void RecSave::save_first_line(const UCHAR* buf, const UCHAR* end) {
    UCHAR first[MAX_LLINE];
    sncpy(first, buf, end-buf);
    conf.set_info("rec.first", (const char*)first);
    conf.save_info();

    m_last.initilized = true;
}

void RecSave::put_str(UCHAR i, const UCHAR* p, UINT32 len) {
    ranger[i].num.put_u(&rcoder, len);
    for (UINT32 j = 0; j < len; j++)
        ranger[i].str.put(&rcoder, p[j]);
}


void RecSave::put_num(UCHAR i, long long num) {
    if (ranger[i].num.put_u(&rcoder, num))
        stats.big_i ++;
}

void RecSave::put_type(UCHAR i, seg_type type) {
    ranger[i].type.put(&rcoder, type);
}

RecLoad::RecLoad() {
    m_valid = true;
    range_init();

    filer = new FilerLoad(conf.open_r("rec"), &m_valid);
    assert(filer);
    rcoder.init(filer);

    BZERO(m_last);
}

RecLoad::~RecLoad() {
    rcoder.done();
}

size_t RecLoad::load_first_line(UCHAR* buf) {

    m_last.initilized = true;

    const char *first = conf.get_info("rec.first");
    strcpy((char*)buf, first);
    return strlen(first);
}

UCHAR RecLoad::get_type(UCHAR i) {
    return ranger[i].type.get(&rcoder);
}

long long RecLoad::get_num(UCHAR i) {
    return ranger[i].num.get_u(&rcoder);
}

UCHAR* RecLoad::get_str(UCHAR i, UCHAR* p) {
    UINT32 len = ranger[i].num.get_u(&rcoder);
    for (UINT32 j = 0; j < len; j++)
        p[j] = ranger[i].str.get(&rcoder);
    return p + len;
}


////////////////////////////////

static bool isword(UCHAR c) { return isdigit(c) or isalpha(c);}

void RecBase::map_space(const UCHAR* p, bool index) {
    smap[index].len = 0;
    smap[index].off[0] = 0;
    for (int i = 0; ; i++) {
        if (not isword(p[i])) {
            smap[index].wln[smap[index].len  ] = i - smap[index].off[smap[index].len];
            smap[index].str[smap[index].len++] = p[i];
            smap[index].off[smap[index].len  ] = i + 1;

            rarely_if (p[i] == 0 or p[i] == '\n')
                break;

            rarely_if(smap[index].len > 64)
                croak("ERROR: irregulal record (over 64 non alpha non digit). Is it a valid fastq file?");
        }
    }
}

#define IS_CLR(exmap, offset) (0 == (exmap&(1ULL<<offset)))
#define IS_SET(exmap, offset) !IS_CLR(exmap, offset)
#define DO_SET(exmap, offset) (exmap |=  (1ULL<<offset))
#define DO_CLR(exmap, offset) (exmap &= ~(1ULL<<offset))
#define BCOUNT(exmap)       __builtin_popcount(exmap)
#define BFIRST(exmap)       __builtin_ffsl(exmap)

static bool is_number(const UCHAR* p, int len, long long &num) {
    num = 0;
    for (int i = 0 ; i < len; i ++)
        if (isdigit(p[i]))
            num = (num<<3) + (num<<1) + (p[i]) - '0';
        else
            return false;
    return true;
}

void RecSave::save_2(const UCHAR* buf, const UCHAR* end, const UCHAR* prev_buf, const UCHAR* prev_end) {
    rarely_if(not m_last.initilized) {
        save_first_line(buf, end);
        map_space(buf, 0);
        lmap = 0;
        return;
    }
    bool pmap = lmap;
    lmap = lmap ? 0 : 1;
    map_space(buf, lmap);

    if (smap[lmap].len != smap[pmap].len or 
        memcmp(smap[lmap].str, smap[pmap].str, smap[lmap].len)) {
        // Too bad, new spacer
        put_type(0, ST_STR);
        put_str (0, buf, end-buf);
        stats.new_n ++ ;
        stats.new_l += end-buf;
        return;
    }

    put_type(0, ST_SAME);
    UINT64 map = 0;
    for (int i = 0; i < smap[lmap].len; i ++) 
        if ( smap[lmap].wln[i] != smap[pmap].wln[i] or 
             memcmp(buf + smap[lmap].off[i], prev_buf + smap[pmap].off[i], smap[lmap].wln[i]))
            DO_SET(map, i);

    put_num(0, map);

    for (int i = 0; i < smap[lmap].len; i ++) {
        if (IS_CLR(map, i))     // TODO: use BFIRST
            continue;
        long long bnum, pnum;
        const UCHAR* b =      buf + smap[lmap].off[i];
        const UCHAR* p = prev_buf + smap[pmap].off[i];
        if (is_number(b, smap[lmap].wln[i], bnum) and
            is_number(p, smap[pmap].wln[i], pnum)) {
            if (bnum > pnum) {
                put_type(i+1, ST_GAP);
                put_num (i+1, bnum - pnum);
            }
            else {
                put_type(i+1, ST_PAG);
                put_num (i+1, pnum - bnum);
            }
        }
        else {
            put_type(i+1, ST_STR);
            put_str (i+1, b, smap[lmap].wln[i]);
            stats.str_n ++ ;
            stats.str_l += smap[lmap].wln[i];
        }
    }
}

size_t RecLoad::load_2(UCHAR* buf, const UCHAR* prev) {

    rarely_if(not m_last.initilized)
        return load_first_line(buf);

    UCHAR type = get_type(0);
    if (type == ST_STR) {
        UCHAR* b = get_str(0, buf);
        return b - buf;
    }
    assert (type == ST_SAME);
    map_space(prev, 0);
    UINT64 map = get_num(0);
    UCHAR* b = buf;
    for (int i = 0; i < smap[0].len; i++) {
        if (IS_SET(map, i)) {

            type = get_type(i+1);
            switch ((seg_type)type) {
            case ST_STR: {
                b = get_str(i+1, b);
            } break;

            // case ST_GAP: {
            //     long long pval;
            //     bool expect_num = is_number(prev + smap[0].off[i], smap[0].wln[i], pval);
            //     assert(expect_num);
            //     long long gap = get_num(i+1);
            //     b += sprintf((char*)b, "%lld", pval + gap);
            // } break;

            case ST_GAP:
            case ST_PAG: {
                long long pval;
                bool expect_num = is_number(prev + smap[0].off[i], smap[0].wln[i], pval);
                assert(expect_num);
                long long gap = get_num(i+1);
                long long val = type == ST_GAP ? pval + gap : pval - gap ;
                b += sprintf((char*)b, "%lld", val);
            } break;

            default:
                croak("bade type value %d", type);
            }
        }
        else {
            b = (UCHAR*)mempcpy(b, prev + smap[0].off[i], smap[0].wln[i]);
        }
        *b ++ = smap[0].str[i];
    }
    return b-buf-1;
}

#ifdef This_is_an_Attic
// (well, technicaly it's a deep basement)

class SRStack {
    int idx ;
    int odx ;
    struct {
        UCHAR type;
        UCHAR len;
        long long num;
        const UCHAR* ptr;
    } s[100]; // unlimited, AFAWK

public:
    SRStack() { idx = odx = 0; }
    inline void push(UCHAR type, UCHAR len, long long num, const UCHAR* ptr=NULL) {
        s[idx].type = type;
        s[idx].len  = len ;
        s[idx].num  = num;
        s[idx].ptr  = ptr;
        idx++;
        assert(idx < 100);
    }
    inline bool pop(UCHAR &type, UCHAR &len, long long &num, const UCHAR* &ptr) {
        // order is: first, <reverse order>, last
        int i;
        if (odx == 0)
            i = 0, odx = idx-1 ;
        else if (odx == 1)
            i = idx-1, odx = idx;
        else
            i = --odx;

        type = s[i].type;
        len  = s[i].len;
        num  = s[i].num;
        ptr  = s[i].ptr;
        return true;
    }
    inline bool done() { return odx == idx; }
};

UCHAR RecLoad::get_len(UCHAR i) {
    return ranger[i].len.get(&rcoder);
}

void RecSave::put_type(UCHAR i, seg_type type, UCHAR len) {
    ranger[i].type.put(&rcoder, type);
    ranger[i].len .put(&rcoder, len);
}

static long long getnum(const UCHAR* &p) {
    // get number, forward pointer to border
    long long ret ;
    for(ret = 0; isdigit(*p); p++)
        ret = (ret<<3) + (ret<<1) + (*p) - '0';

    return ret;
}

// static bool isspace(UCHAR c) { switch(c) {
//     case 0  : case ' ': case '\t' : case '\n': case '/': cat ':':
//     case '_': 
//         return true;
//     default: return false;
//     } }

static const UCHAR* getspace(const UCHAR* p) {
    while (isword(*p)) p++;
    return p;
}

static int getspace(const UCHAR* p, bool &digit) {
    digit = true;
    for (int i = 0; ; i++ )
        if (not isdigit(p[i])) {
            if (not isalpha(p[i]))
                return i;
            digit = false;
        }
}

static long long getnum(const UCHAR* p, int len) {
    long long ret = 0;
    for(int i = 0; i < len; i ++)
        ret = (ret<<3) + (ret<<1) + p[i] - '0';
    return ret;
}

void RecSave::save_1(const UCHAR* buf, const UCHAR* end, const UCHAR* prev_buf, const UCHAR* prev_end) {
    rarely_if(not m_last.initilized)
        return save_first_line(buf, end);

    const UCHAR* b = buf;
    const UCHAR* p = prev_buf;

    SRStack srs;
    while (1) {
        const int blen = end - b;
        int len = 0;
        while (b[len] == p[len] and LIKELY(len < blen))
            len ++;

        rarely_if(len == blen) {
            srs.push(ST_END, len, 0);
            break;              // LOOP END
        }

        while (len and isdigit(b[len-1]))
            len --;             // TODO: set digit border while forward

        b += len;
        p += len;

        likely_if (len and isdigit(*b) and *b != '0') {
            bool single = len == 1 and b[-1] == p[-1];
            long long new_val = getnum(b);
            long long old_val = getnum(p); // TODO: cache old_val
            if (single) {
                if (new_val >= old_val)    // It can't really be equal, can it?
                    srs.push(ST_GAP, 0, new_val - old_val);
                else 
                    srs.push(ST_PAG, 0, old_val - new_val);
            }
            else {
                if (new_val >= old_val)
                    srs.push(ST_GAPL, len, new_val - old_val);
                else 
                    srs.push(ST_PAGL, len, old_val - new_val);
            }
            continue;
        }
        {
            // find next space in both
            const UCHAR* space = getspace(b+1);
            srs.push(ST_STR, len, space-b, b);
            b = space;
            p = getspace(p+1);
        }
        rarely_if (p >= prev_end) 
            p = prev_end-1;
    }

    UCHAR utype;
    UCHAR len;
    long long num;
    const UCHAR* ptr;
    for (int index = 0;
         srs.pop(utype, len, num, ptr);
         index++) {
        rarely_if(index > m_range_last) index = m_range_last;
        const seg_type type = (seg_type) utype;

        switch (type) {
        case ST_END: 
            put_type(index, type, len);
            assert(srs.done());
            return;

        case ST_GAP:
        case ST_PAG:
            put_type(index, type);
            put_num(index, num);
            break;

        case ST_GAPL:
        case ST_PAGL:
            put_type(index, type, len);
            put_num(index, num);
            break;

        case ST_STR:
            put_type(index, type, len);
            put_str(index, ptr, num);
            break;

        default:
            assert(0);
        }
    }
}

size_t RecLoad::load_1(UCHAR* buf, const UCHAR* prev) {

    rarely_if(not m_last.initilized)
        return load_first_line(buf);

    SRStack srs;
    UCHAR tmp_buf[1000];
    UCHAR* p_buf = tmp_buf;
    const UCHAR *p_tmp;
    UCHAR type, len;
    long long num;

    for (int index = 0;  ; index++ ) {
        rarely_if(index > m_range_last) index = m_range_last;
        type = get_type(index);
        assert(type < ST_LAST);
        len = (type == ST_GAP or type == ST_PAG) ? 1 : get_len(index);
        if (type == ST_END) {
            srs.push(type, len, 0);
            break;
        }
        num = get_num(index);
        if (type == ST_STR) {
            p_tmp = p_buf;
            p_buf = get_str(index, p_buf);
            srs.push(type, len, num, p_tmp);
        }
        else
            srs.push(type, len, num);
    }

    UCHAR* b = buf;
    const UCHAR* p = prev;

    while (srs.pop(type, len, num, p_tmp)) {
        for (int i = 0; i < len; i++) {
            if (1) assert(*p and *p != '\n'); // IF_DEBUG ...
            *b ++ = *p ++;
        }
        switch ((seg_type)type) {
        case ST_GAPL:
        case ST_GAP: {
                long long old_val = getnum(p);
                b += sprintf((char*)b, "%lld", old_val + num );
            } break;
        case ST_PAGL:
        case ST_PAG: {
                long long old_val = getnum(p);
                b += sprintf((char*)b, "%lld", old_val - num);
            } break;
        case ST_STR: {
                p = getspace(p+1);
                for (int j = 0; j < num; j++)
                    *b++ = *p_tmp++;
            } break;

        // case ST_0_F:  p++ ; 
        // case ST_0_B: *b++ = '0'; break;
        case ST_END:
            *b = 0;             // just for easy debugging
            return b - buf;
        // case ST_LAST:
        default: croak("bad record segment type: %u", type);
        }
        // rarely_if (p >= prev_end) 
        //     p = prev_end-1;
        rarely_if(*p == '\n')
            p -- ;
    }

    return 0;
}

void RecSave::save_3(const UCHAR* buf, const UCHAR* end, const UCHAR* prev_buf, const UCHAR* prev_end) {
    // TODO:
    // split by getspace
    // save each num/str as var
    // if spaces are not equal, save as str (unlikely, affects compression ratio)
    // use ST_SAME when strcmp == 0

    rarely_if(not m_last.initilized)
        return save_first_line(buf, end);

    const UCHAR* b = buf;
    const UCHAR* p = prev_buf;

    for (int index = 0; ; index ++) {
        rarely_if(index > m_range_last) index = m_range_last;

        rarely_if(b >= end) {
            put_type(index, ST_END);
            return;
        }

        bool bd, pd;
        int blen, plen;
        rarely_if(p >= prev_end) {
            bd = pd = false;
            blen = end-b;
            plen = 0;
        }
        else {
            blen = getspace(b, bd);
            plen = getspace(p, pd);
        }

        likely_if (blen == plen and 0 == memcmp(p, b, blen+1)) {
            // all match, including deliminator
            put_type(index, ST_SAME);
            b += blen + 1;
            p += plen + 1;
        }

        else if ((bd and pd) and
                 LIKELY(b[blen] == p[plen] or (b+blen >= end)) and
                 LIKELY(b[0] != '0' and blen<20 and plen<20)) {
            // numbers, matching deliminator
            long long new_val = getnum(b, blen);
            long long old_val = getnum(p, plen); // TODO: cache old_val
            if (new_val > old_val) {
                put_type(index, ST_GAP);
                put_num (index, new_val-old_val);
            }
            else {
                put_type(index, ST_PAG);
                put_num (index, old_val-new_val);
            }
            b += blen + 1;
            p += plen + 1;
        }
        else {
            // fallback, string, includes deliminator
            put_type(index, ST_STR);
            put_str (index, b, blen+1);
            b += blen;
            p += plen;
        }
    }
}

size_t RecLoad::load_3(UCHAR* buf, const UCHAR* prev) {
    rarely_if(not m_last.initilized)
        return load_first_line(buf);
    
    UCHAR* b = buf;
    const UCHAR* p = prev;
    bool dummy ;

    for (int index = 0;  ; index++ ) {
        rarely_if(index > m_range_last) index = m_range_last;
        UCHAR type = get_type(index);
        switch((seg_type)type) {
        case ST_SAME: {
            int plen = getspace(p, dummy);
            memcpy(b, p, plen+1);
            b += plen+1;
            p += plen+1;
        } break;

        case ST_PAG:
        case ST_GAP: {
            long long old_val = getnum(p);
            long long num = get_num(index);
            long long val = type == ST_GAP ? old_val + num : old_val - num ;
            b += sprintf((char*)b, "%lld", val);
            *b ++ = *p ++;
        } break;

        case ST_STR: {
            p += getspace(p, dummy);
            b = get_str(index, b);
            // (fallback) let deliminators have their own index 
        } break;

        case ST_END:
            *b = 0;             // debugging
            return b - buf - *p == '\n';

        default:
            croak("bad record segment type: %u", type);

        };
    }
}

#endif
