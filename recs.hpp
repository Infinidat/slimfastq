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


#ifndef FQ_RECS_H
#define FQ_RECS_H

#include "common.hpp"
#include "config.hpp"
#include <stdio.h>

#include "filer.hpp"
#include "power_ranger.hpp"

class RecBase {
protected: 

    RecBase()  {}
    ~RecBase() {rcoder.done();}

    // 10 type
    // 10 len
    // 10 num

    struct {
        PowerRanger type;
        PowerRanger  len;
        PowerRanger  str;
        PowerRangerU num;
    } PACKED ranger[10];

    RCoder rcoder;

    struct {
        UCHAR  len[10];
        bool   initilized;
        // long long num[10]; - TODO: keep array of prev atoi and end pointers
    } m_last;

    struct {
        UINT32 big_i;
        UINT32 str_n;
        UINT32 str_l;
        UINT32 zero_f;
        UINT32 zero_b;
    } stats;

    bool m_valid;

    void range_init();

    // Division of labor
    enum seg_type {
        ST_GAP,
        ST_PAG,
        ST_END,
        ST_STR,
        ST_0_F,
        ST_0_B,
        ST_LAST
    };


private:
    UCHAR norm(int i) { return (LIKELY(i < 10) ? i : 9); }

protected:
    // void put_len(UCHAR i, int len) {
    //     ranger[MAX5(0, i)].put_u(&rcoder, len);
    // }
    // int get_len(UCHAR i) {
    //     return ranger[MAX5(0, i)].get_u(&rcoder);
    // }
    void put_type(UCHAR i, seg_type type, UCHAR len) {
        i = norm(i);
        ranger[i].len .put(&rcoder, len);
        ranger[i].type.put(&rcoder, type);
        // if (len == m_last.len[i])
            // ranger[i].type.put(&rcoder, type);
        // else {
            // ranger[i].type.put(&rcoder, type | 8);
            // ranger[i].len .put(&rcoder, len);
        //     m_last.len[i] = len;
        // }
    }
    seg_type get_type(UCHAR i, UCHAR* len) {
        i = norm(i);
        * len = ranger[i].len .get(&rcoder);
        return (seg_type) ranger[i].type.get(&rcoder);
        // UCHAR c  = ranger[i].type.get(&rcoder);
        // *len =
        //     (c & 8) ?
        //     (m_last.len[i] = ranger[i].len .get(&rcoder)) :
        //     (m_last.len[i]);
        // return (seg_type)(c & 7);
    }

    void put_num(UCHAR i, long long num) {
        i = norm(i);
        if (ranger[i].num.put_u(&rcoder, num))
            stats.big_i ++;
    }
    long long get_num(UCHAR i) {
        i = norm(i);
        return ranger[i].num.get_u(&rcoder);
    }
    void put_str(UCHAR i, const UCHAR* p, UINT32 len) {
        i = norm(i);
        stats.str_n ++ ;
        stats.str_l += len;
        ranger[i].num.put_u(&rcoder, len);
        for (UINT32 j = 0; j < len; j++)
            ranger[i].str.put(&rcoder, p[j]);
    }
    UCHAR* get_str(UCHAR i, UCHAR* p) {
        i = norm(i);
        UINT32 len = ranger[i].num.get_u(&rcoder);
        for (UINT32 j = 0; j < len; j++)
            p[j] = ranger[i].str.get(&rcoder);
        return p + len;
    }
};

class RecSave : private RecBase {
public:
     RecSave();
    ~RecSave();

    void save_1(const UCHAR* buf, const UCHAR* end, const UCHAR* prev_buf, const UCHAR* prev_end) { save_2(buf, end, prev_buf, prev_end); }
    void save_2(const UCHAR* buf, const UCHAR* end, const UCHAR* prev_buf, const UCHAR* prev_end);
    void save_3(const UCHAR* buf, const UCHAR* end, const UCHAR* prev_buf, const UCHAR* prev_end) { save_2(buf, end, prev_buf, prev_end); }
private:
    void save_first_line(const UCHAR* buf, const UCHAR* end);

    FilerSave* filer;
};

class RecLoad : private RecBase {
public:
     RecLoad();
    ~RecLoad();

    inline bool is_valid() {return m_valid;}
    size_t load_1(UCHAR* buf, const UCHAR* prev) { return load_2(buf, prev) ;}
    size_t load_2(UCHAR* buf, const UCHAR* prev);
    size_t load_3(UCHAR* buf, const UCHAR* prev) { return load_2(buf, prev) ;}

private:
    size_t load_first_line(UCHAR* buf);

    FilerLoad* filer;
};



#endif
