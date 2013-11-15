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
// #include <ctype.h>
#include "recs.hpp"

#define MAX_LLINE 400           // TODO: assert max is enough at constructor


void RecBase::range_init() {
    BZERO(ranger);
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
    fprintf(stderr, "::: REC big i: %u | str n/l: %u/%u | zero f/b: %u/%u\n",
            stats.big_i, stats.str_n, stats.str_l, stats.zero_f, stats.zero_b );
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

static bool is_digit(UCHAR c) {
    return c >= '0' and c <= '9';
}

static long long getnum(const UCHAR* &p) {
    // get number, forward pointer to border
    bool zap = RARELY(*p == 0);

    long long ret ;
    for(ret = 0; is_digit(*p); p++)
        ret = (ret<<3) + (ret<<1) + (*p) - '0';

    return zap ? 0 : ret;
}

static bool isspace(UCHAR c) { switch(c) {
    case 0: case ' ': case '\t' : case '\n': case '/': return true;
    default: return false;
    } }

static const UCHAR* getspace(const UCHAR* p) {
    while (not isspace(*p)) p++;
    return p;
}

void RecSave::save_2(const UCHAR* buf, const UCHAR* end, const UCHAR* prev_buf, const UCHAR* prev_end) {
    rarely_if(not m_last.initilized)
        return save_first_line(buf, end);

    const UCHAR* b = buf;
    const UCHAR* p = prev_buf;

    for (int index = 0; ; index++) {
        int len = 0;

        while (*b == *p and LIKELY(b < end))
            b++, p++, len++;
        
        rarely_if(b == end) {
            put_type(index, ST_END, len);
            return;
        }

        rarely_if(*b == '0') {

            int i = 1;
            while (i < len and
                   b[-i] == '0')
                i ++ ;

            if (is_digit(b[-i]) and i < len) {
                b  -= i;
                p  -= i;
                len-= i;
            }
            else {
                likely_if(is_digit(p[1])) {
                    stats.zero_f++;
                    put_type(index, ST_0_F, len);
                    b++, p++;
                }
                else {
                    stats.zero_b++;
                    put_type(index, ST_0_B, len);
                    b++;
                }
                continue;
            }
        }
        likely_if (is_digit(*b)) {
            long long new_val = getnum(b);
            long long old_val = getnum(p); // TODO: cache old_val
            if (new_val >= old_val) {      // It can't really be equal, can it?
                put_type(index, ST_GAP, len);
                put_num (index, new_val - old_val);
            }
            else {
                put_type(index, ST_PAG, len);
                put_num (index, old_val - new_val);
            }
            continue;
        }
        {
            // find next space in both
            const UCHAR* space = getspace(b);
            put_type(index, ST_STR, len);
            put_str(index, b, space-b);
            b = space;
            p = getspace(p);
        }
        rarely_if (p >= prev_end) 
            p = prev_end-1;
    }
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

size_t RecLoad::load_2(UCHAR* buf, const UCHAR* prev) {

    rarely_if(not m_last.initilized)
        return load_first_line(buf);

    UCHAR* b = buf;
    const UCHAR* p = prev;

    for (int index = 0;  ; index++ ) {
        UCHAR len;
        seg_type type = get_type(index, &len);

        for (int i = 0; i < len; i++) {
            if (1) assert(*p and *p != '\n'); // IF_DEBUG ...
            *b ++ = *p ++;
        }

        switch ( type) {
        case ST_GAP: {
                long long old_val = getnum(p);
                long long new_val = old_val + get_num(index);
                b += sprintf((char*)b, "%lld", new_val);
            } break;
        case ST_PAG: {
                long long old_val = getnum(p);
                long long new_val = old_val - get_num(index);
                b += sprintf((char*)b, "%lld", new_val);                
            } break;
        case ST_STR: {
                p = getspace(p);
                b = get_str(index, b);
            } break;

        case ST_0_F:  p++ ; 
        case ST_0_B: *b++ = '0'; break;
        case ST_END: return b - buf;
        case ST_LAST:
        default: croak("bad record segment type: %u", type);
        }
    }

    return 0;
}
