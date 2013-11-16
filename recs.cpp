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

void RecBase::range_init() {
    const int cnt = conf.level > 2 ? N_RANGE_BIG : N_RANGE_SML;
    ranger = new ranger_t [cnt];
    bzero(ranger, sizeof(ranger[0]) * cnt);
    m_range_last = cnt-1;
}

RecSave::RecSave() {

    m_valid = true;

    BZERO(m_last);
    BZERO(stats);
    range_init();

    filer = new FilerSave(conf.open_w("rec"));
    assert(filer);
    rcoder.init(filer);
}

RecSave::~RecSave() {
    put_type(0, ST_END, 0);
    rcoder.done();
    DELETE(filer);
    fprintf(stderr, "::: REC big i: %u | str n/l: %u/%u \n",
            stats.big_i, stats.str_n, stats.str_l );
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

static long long getnum(const UCHAR* &p) {
    // get number, forward pointer to border
    bool zap = RARELY(*p == 0);

    long long ret ;
    for(ret = 0; isdigit(*p); p++)
        ret = (ret<<3) + (ret<<1) + (*p) - '0';

    return zap ? 0 : ret;
}

// static bool isspace(UCHAR c) { switch(c) {
//     case 0  : case ' ': case '\t' : case '\n': case '/': cat ':':
//     case '_': 
//         return true;
//     default: return false;
//     } }

static const UCHAR* getspace(const UCHAR* p) {
    while (not isdigit(*p) and not isalpha(*p)) p++;
    return p;
}


void RecSave::put_str(UCHAR i, const UCHAR* p, UINT32 len) {
    stats.str_n ++ ;
    stats.str_l += len;
    ranger[i].num.put_u(&rcoder, len);
    for (UINT32 j = 0; j < len; j++)
        ranger[i].str.put(&rcoder, p[j]);
}


void RecSave::put_num(UCHAR i, long long num) {
    if (ranger[i].num.put_u(&rcoder, num))
        stats.big_i ++;
}

void RecSave::put_type(UCHAR i, seg_type type, UCHAR len) {
    ranger[i].type.put(&rcoder, type);
    ranger[i].len .put(&rcoder, len);
}

void RecSave::put_type(UCHAR i, seg_type type) {
    ranger[i].type.put(&rcoder, type);
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

        likely_if (isdigit(*b) and *b != '0') {
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
            const UCHAR* space = getspace(b);
            srs.push(ST_STR, len, space-b, b);
            b = space;
            p = getspace(p);
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

void RecSave::save_3(const UCHAR* buf, const UCHAR* end, const UCHAR* prev_buf, const UCHAR* prev_end) {
    // TODO:
    // split by getspace
    // save each num/str as var
    // if spaces are not equal, save as str (unlikely, affects compression ratio)
    // use ST_SAME when strcmp == 0
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

UCHAR RecLoad::get_len(UCHAR i) {
    return ranger[i].len.get(&rcoder);
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
                p = getspace(p);
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
    }

    return 0;
}
