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

    enum { m_range_last = 66
    };

    struct ranger_t {
        PowerRanger type;
        PowerRanger  len;
        PowerRanger  str;
        PowerRangerU num;
    } PACKED ;

    ranger_t ranger[66];

    RCoder rcoder;

    struct {
        bool   initilized;
        // long long num[10]; - TODO: cache array of prev atoi and end pointers
    } m_last;

    struct {
        UINT32 big_i;
        UINT32 str_n;
        UINT32 str_l;
        UINT32 new_n;
        UINT32 new_l;
    } stats;

    bool m_valid;

    void range_init();

    // Division of labor
    enum seg_type {
        ST_SAME,
        ST_GAP,
        ST_PAG,
        ST_GAPL,
        ST_PAGL,
        ST_END,
        ST_STR,
        // ST_0_F,
        // ST_0_B,
        ST_LAST
    };

    struct space_map {
        int   off[65];
        int   wln[65];
        UCHAR str[65];
        UCHAR len;
    };
    space_map smap[2];
    bool lmap;
    void map_space(const UCHAR* p, bool index);
};

class RecSave : private RecBase {
public:
     RecSave();
    ~RecSave();

    void save_1(const UCHAR* buf, const UCHAR* end, const UCHAR* prev_buf, const UCHAR* prev_end);
    void save_2(const UCHAR* buf, const UCHAR* end, const UCHAR* prev_buf, const UCHAR* prev_end);
    void save_3(const UCHAR* buf, const UCHAR* end, const UCHAR* prev_buf, const UCHAR* prev_end);
private:
    void save_first_line(const UCHAR* buf, const UCHAR* end);
    void put_type(UCHAR i, seg_type type, UCHAR len);
    void put_type(UCHAR i, seg_type type);
    void put_num(UCHAR i, long long num);
    void put_str(UCHAR i, const UCHAR* p, UINT32 len);

    FilerSave* filer;
};

class RecLoad : private RecBase {
public:
     RecLoad();
    ~RecLoad();

    inline bool is_valid() {return m_valid;}
    size_t load_1(UCHAR* buf, const UCHAR* prev);
    size_t load_2(UCHAR* buf, const UCHAR* prev);
    size_t load_3(UCHAR* buf, const UCHAR* prev);

private:
    size_t load_first_line(UCHAR* buf);

    long long get_num (UCHAR i);
    UCHAR     get_type(UCHAR i);
    UCHAR     get_len (UCHAR i);
    UCHAR*    get_str (UCHAR i, UCHAR* p);

    FilerLoad* filer;
};



#endif
