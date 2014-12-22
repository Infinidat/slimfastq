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


// void RecBase::range_init() {
//     // const int cnt = conf.level > 2 ? N_RANGE_BIG : N_RANGE_SML;
//     // ranger = new ranger_t [cnt];
//     // // bzero(ranger, sizeof(ranger[0]) * cnt);
//     // m_range_last = cnt-1;
// }

RecSave::RecSave() {

    m_valid = true;

    BZERO(m_last);
    BZERO(stats);
    // range_init();

    filer = new FilerSave("rec");
    assert(filer);
    rcoder.init(filer);

    x_file = new XFileSave("rec.x");

    smap[0].len = smap[1].len = 0;
}

RecSave::~RecSave() {
    rcoder.done();
    if (not conf.quiet and filer and x_file)
        fprintf(stderr, "::: REC comp size: %lu \t| EX size: %lu | bigint: %u | str num/size: %u/%u | newline num/size: %u/%u\n",
                filer->tell(), x_file->tell(),
                stats.big_i, stats.str_n, stats.str_l, stats.new_n, stats.new_l);

    DELETE(filer);
    DELETE(x_file);
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
    // conf.save_info();

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
    // range_init();

    filer = new FilerLoad("rec", &m_valid);
    assert(filer);
    rcoder.init(filer);

    BZERO(m_last);

    x_file = new XFileLoad("rec.x");
    m_last.index = x_file->get();
}

RecLoad::~RecLoad() {
    rcoder.done();
    DELETE(x_file);
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

static bool is_number(const UCHAR* p, int len, long long &num) {
    rarely_if (*p == '0')
        return false;
    num = 0;
    for (int i = 0 ; i < len; i ++)
        if (isdigit(p[i]))
            num = (num<<3) + (num<<1) + (p[i]) - '0';
        else
            return false;
    return true;
}

void RecSave::save(const UCHAR* buf, const UCHAR* end, const UCHAR* prev_buf, const UCHAR* prev_end) {
    rarely_if(not m_last.initilized) {
        save_first_line(buf, end);
        map_space(buf, 0);
        imap = 0;
        // last_map = 0;
        return;
    }
    bool pmap = imap;
    imap = imap ? 0 : 1;
    map_space(buf, imap);

    rarely_if (smap[imap].len != smap[pmap].len or 
               memcmp(smap[imap].str, smap[pmap].str, smap[imap].len)) {
        // Too bad, new spacer
        // put_type(0, ST_LINE);
        // put_str (0, buf, end-buf);
        // last_map = 0;
        x_file->put(g_record_count - m_last.index);
        m_last.index = g_record_count;
        x_file->put_str(buf, end-buf);

        stats.new_n ++ ;
        stats.new_l += end-buf;
        return;
    }

    UINT64 map = 0;
    for (int i = 0; i < smap[imap].len; i ++) 
        if ( smap[imap].wln[i] != smap[pmap].wln[i] or 
             memcmp(buf + smap[imap].off[i], prev_buf + smap[pmap].off[i], smap[imap].wln[i]))
            DO_SET(map, i);

    // if (last_map == map)
        // put_type(0, ST_SAME);
    // else {
    //     put_type(0, ST_SMAP);
        put_num (0, map);
    //     last_map =  map;
    // }

    while (map) {
        int i = BFIRST(map)-1;
        DO_CLR(map, i);
        long long bnum, pnum;
        const UCHAR* b =      buf + smap[imap].off[i];
        const UCHAR* p = prev_buf + smap[pmap].off[i];
        if (is_number(b, smap[imap].wln[i], bnum) and
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
            put_str (i+1, b, smap[imap].wln[i]);
            stats.str_n ++ ;
            stats.str_l += smap[imap].wln[i];
        }
    }
}

size_t RecLoad::load(UCHAR* buf, const UCHAR* prev) {

    rarely_if(not m_last.initilized) {
        // last_map = 0;
        return load_first_line(buf);
    }

    // if (type == ST_LINE) {
    rarely_if(m_last.index == g_record_count) {
        // last_map = 0;
        // UCHAR* b = get_str(0, buf);
        UCHAR* b = x_file->get_str(buf);
        m_last.index += x_file->get();
        return b - buf;
    }

    map_space(prev, 0);
    // if (type == ST_SMAP)
    //     last_map = get_num(0);
    // else
    // UCHAR type = get_type(0);
    // rarely_if (type != ST_SAME)
    //     croak("REC: bad type value %d", type);

    UINT64 map = get_num(0);

    UCHAR* b = buf;
    for (int i = 0; i < smap[0].len; i++) {
        // if (IS_SET(last_map, i)) {
         if (IS_SET(map, i)) {

             UCHAR type = get_type(i+1);
             switch ((seg_type)type) {

             case ST_GAP:
             case ST_PAG: {
                 long long pval;
                 bool expect_num = is_number(prev + smap[0].off[i], smap[0].wln[i], pval);
                 assert(expect_num);
                 long long gap = get_num(i+1);
                 long long val = type == ST_GAP ? pval + gap : pval - gap ;
                 b += sprintf((char*)b, "%lld", val);
             } break;

             case ST_STR: {
                 b = get_str(i+1, b);
             } break;

             default:
                 croak("REC: bad type value %d", type);
             }
         }
        else {
            // b = (UCHAR*)mempcpy(b, prev + smap[0].off[i], smap[0].wln[i]);
            // mempcpy is not compatible with Mac OS, must do it the hard way ..
            UINT32 count = smap[0].wln[i] ;
            memcpy(b, prev + smap[0].off[i], count );
            b += count;
        }
        *b ++ = smap[0].str[i];
    }
    return b-buf-1;
}
