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

void RecSave::put_type(UCHAR i, UCHAR type) {
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

void RecBase::map_space(const UCHAR* p, bool flip) {
    smap[flip].len = 0;
    smap[flip].off[0] = 0;
    for (int i = 0; ; i++) {
        if (not isword(p[i])) {
            smap[flip].wln [smap[flip].len  ] = i - smap[flip].off[smap[flip].len];
            smap[flip].str[smap[flip].len++] = p[i];
            smap[flip].off[smap[flip].len  ] = i + 1;

            rarely_if (p[i] == 0 or p[i] == '\n')
                break;

            rarely_if(smap[flip].len > 64)
                croak("ERROR: irregulal record (over 64 non alpha non digit). Is it a valid fastq file?");
        }
    }
}

    // Division of labor
enum seg_type {
    // keep these values backward compatible
    ST_DGT = 0,             // deci, greater than
    ST_DLT = 1,             // deci, less than

    ST_STR = 2,             // string

    // version 5 new values

    ST_HGT = 3,             // hexa, greater than, 'a' - 'f'
    ST_HLT = 4,             // hexa, less than

    // hex numbers with one zero preceding (rare)
    ST_HGT_Z = 5,           // '0'.hexa, greater than, 'a' - 'f'
    ST_HLT_Z = 6,           // '0'.hexa, less than


    //  capital letter numbers (Yack!)
    ST_HGTC = 7,            // hexa, greater than, capital 'A'-'F'
    ST_HLTC = 8,            // hexa, greater than, capital 'A'-'F'

    //  capital letter numbers
    ST_HGTC_Z = 9,          // '0'.hexa, greater than, capital 'A'-'F'
    ST_HLTC_Z = 10,         // '0'.hexa, greater than, capital 'A'-'F'

    // very unlikely
    ST_DGT_Z = 11,          // '0'.deci, greater than
    ST_DLT_Z = 12,          // '0'.deci, less than

};


static UCHAR numberwang(const UCHAR* p, int len, long long &num, UCHAR pctype)
{
    // return type
    //  hex if deci & prev == hex
    //  hex if hex
    //  deci if deci
    //  str else
    int i = 0;
    bool has_z = p[i] == '0';
    rarely_if (has_z)
        if ( p[++i] == '0')     // can't recreate two preceding zeros
            return ST_STR;

    UCHAR caps  = 0; // 0=? 1=lower 2=caps

    num = 0;
    while (pctype == 2) {       // prev weren't hex
        if (i >= len)
            return has_z ? ST_DGT_Z : ST_DGT ;

        if (isdigit(p[i])) {
            num = (num<<3) + (num<<1) + (p[i++]) - '0';
            continue;
        }
        // Here: not a deci

        if ((p[i]|0x20) < 'a' or
            (p[i]|0x20) > 'f') 
            return ST_STR;

        // reset and try as hex
        caps = 1 + (p[i] < 'a');
        i = has_z;
        num = 0;
        break;
    }

    for (; i < len; i ++) {
       int nibel;
       if (isdigit (p[i]))
           nibel = (p[i]) - '0';

       else if (p[i] >= 'a' and p[i] <= 'f') {
           if (caps == 2)
               return ST_STR;   // can't combine upper and lower
           caps = 1;
           nibel = 10 + (p[i] - 'a');
       }
       else if (p[i] >= 'A' and p[i] <= 'F') {
           if (caps == 1)
               return ST_STR;   // can't combine upper and lower
           caps = 2;
           nibel = 10 + (p[i] - 'A');
       }
       else
           return ST_STR;

       num = (num<<4) + nibel;
    }

    // Here: it's an hexa
    return
        caps == 2 ?
        has_z ? ST_HGTC_Z : ST_HGTC :
        has_z ? ST_HGT_Z  : ST_HGT ;
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
        bzero(ctype[0], sizeof(ctype[0])); // clear cache
        bzero(ctype[1], sizeof(ctype[1])); // clear cache
        imap = 0;
        // last_map = 0;
        return;
    }
    bool pmap = imap;
    imap = imap ? 0 : 1;
    map_space(buf, imap);

    rarely_if (smap[imap].len != smap[pmap].len or
               memcmp(smap[imap].str, smap[pmap].str, smap[imap].len)) {
        // Too bad, new kind of header
        // must push to extension file

        x_file->put(g_record_count - m_last.index);
        m_last.index = g_record_count;
        x_file->put_str(buf, end-buf);
        bzero(ctype[imap], sizeof(ctype[imap])); // clear cache

        stats.new_n ++ ;
        stats.new_l += end-buf;
        return;
    }

    UINT64 map = 0;
    for (int i = 0; i < smap[imap].len; i ++) 
        if ( smap[imap].wln[i] != smap[pmap].wln[i] or 
             memcmp(buf + smap[imap].off[i], prev_buf + smap[pmap].off[i], smap[imap].wln[i]))
            DO_SET(map, i);

    put_num (0, map);           // mapping of changes. (zero could have meant copy prev)

    for (int i = 0; i < smap[0].len; i++) {
        if (IS_SET(map, i)) {

            int i = BFIRST(map)-1;
            DO_CLR(map, i);
            const UCHAR* b =      buf + smap[imap].off[i];
            // const UCHAR* p = prev_buf + smap[pmap].off[i];
            long long bnum;

            UCHAR type = numberwang(b, smap[imap].wln[i], bnum, ctype[pmap][i]);
            if ( type == ST_STR) {
                put_type(i+1, type);
                put_str (i+1, b, smap[imap].wln[i]);
                stats.str_n ++ ;
                stats.str_l += smap[imap].wln[i];
                ctype[pmap][i] = 0; // str
                continue;
            }
            // HERE: this is a number
            long long pnum = ctype[pmap][i] ? cnumb[pmap][i] : 0;
            long long gap ;

            ctype[imap][i] = (type < ST_STR or type >= ST_DGT_Z) ? 1 : 2;
            if (bnum < pnum) {
                gap  = pnum - bnum;
                type ++ ;
            }
            else
                gap  = bnum - pnum;

            put_type(i+1, type);
            put_num (i+1, gap);

            //     if (is_number(b, smap[imap].wln[i], bnum) and
            //         is_number(p, smap[pmap].wln[i], pnum)) {
            //         if (bnum > pnum) {
            //             put_type(i+1, ST_DGT);
            //             put_num (i+1, bnum - pnum);
            //         }
            //         else {
            //             put_type(i+1, ST_DLT);
            //             put_num (i+1, pnum - bnum);
            //         }
            //     }
            //     else {
            //         put_type(i+1, ST_STR);
            //         put_str (i+1, b, smap[imap].wln[i]);
            //         stats.str_n ++ ;
            //         stats.str_l += smap[imap].wln[i];
            //     }
        }
        else 
            ctype[imap][i] = ctype[pmap][i];
    }
}

size_t RecLoad::load(UCHAR* buf, const UCHAR* prev) {
    rarely_if(not m_last.initilized) {
        cache_version = conf.get_long("version");
        bzero(ctype[0], sizeof(ctype[0])); // clear cache
        bzero(ctype[1], sizeof(ctype[1])); // clear cache
        imap = 0;
        return load_first_line(buf);
    }

    bool pmap = imap;
    imap = imap ? 0 : 1;

    rarely_if(m_last.index == g_record_count) {

        UCHAR* b = x_file->get_str(buf);
        m_last.index += x_file->get();
        bzero(ctype[imap], sizeof(ctype[imap])); // clear cache

        return b - buf;
    }

    map_space(prev, 0);         // TODO? optimize by caching prev

    rarely_if (cache_version < 5)
        return load_pre5(buf, prev);

    UINT64 map = get_num(0);
    UCHAR* b = buf;

    for (int i = 0; i < smap[0].len; i++) {
        // if (IS_SET(last_map, i)) {
         if (not IS_SET(map, i)) {
            // b = (UCHAR*)mempcpy(b, prev + smap[0].off[i], smap[0].wln[i]);
            // mempcpy is not compatible with Mac OS, must do it the hard way ..
            UINT32 count = smap[0].wln[i] ;
            memcpy(b, prev + smap[0].off[i], count );
            b += count;
            continue;
         }

         UCHAR type = get_type(i+1);

         if ( type == ST_STR ) {
             b = get_str(i+1, b);
             ctype[imap][i] = 0;
             continue;
         }

         long long pval = ctype[pmap][i] == 0 ? 0 : cnumb[pmap][i];
         long long gap = get_num(i+1);

         switch ((seg_type)type) {

             case ST_DGT:
                 b += sprintf((char*)b, "%lld", pval + gap);
                 break;

             case ST_DLT:
                 b += sprintf((char*)b, "%lld", pval - gap);
                 break;

             case ST_STR:
                 croak("WTF"); break;

             case ST_HGT:
                 b += sprintf((char*)b, "%llx", pval + gap);
                 break;

             case ST_HLT:
                 b += sprintf((char*)b, "%llx", pval - gap);
                 break;

             case ST_HGT_Z:
                 b += sprintf((char*)b, "0%llx", pval + gap);
                 break;

             case ST_HLT_Z:
                 b += sprintf((char*)b, "0%llx", pval - gap);
                 break;

             case ST_HGTC:
                 b += sprintf((char*)b, "%llX", pval + gap);
                 break;

             case ST_HLTC:
                 b += sprintf((char*)b, "%llX", pval - gap);
                 break;

             case ST_HGTC_Z:
                 b += sprintf((char*)b, "0%llX", pval + gap);
                 break;

             case ST_HLTC_Z:
                 b += sprintf((char*)b, "0%llX", pval - gap);
                 break;

             case ST_DGT_Z:
                 b += sprintf((char*)b, "0%lld", pval + gap);
                 break;

             case ST_DLT_Z:
                 b += sprintf((char*)b, "0%lld", pval - gap);
                 break;


             default:
                 croak("REC: bad type value %d", type);
             }
        *b ++ = smap[0].str[i];
    }
    return b-buf-1;
}

size_t RecLoad::load_pre5(UCHAR* buf, const UCHAR* prev) {

    UINT64 map = get_num(0);

    UCHAR* b = buf;
    for (int i = 0; i < smap[0].len; i++) {
        // if (IS_SET(last_map, i)) {
         if (IS_SET(map, i)) {

             UCHAR type = get_type(i+1);
             switch (type) {

             case ST_DGT:
             case ST_DLT: {
                      // compatibility - let it open old fi
                 long long pval;
                 bool expect_num = is_number(prev + smap[0].off[i], smap[0].wln[i], pval);
                 assert(expect_num);
                 long long gap = get_num(i+1);
                 long long val = type == ST_DGT ? pval + gap : pval - gap ;
                 // if (UINT64(val) > 0xffffffff00000000) {
                 //     fprintf(stderr, "big val: 0x%llx gap=0x%llx %s pval=0x%llx\n",
                 //             val, gap, (type == ST_DGT ? "GAP" : "PAG"), pval);
                 //     // debug point
                 // }

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
