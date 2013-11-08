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

/*

save/init_first_line();
while ()
   len = 0
   while(same )
        len++;
   if (digit) {
      if end:
          save len                         - [0-9]   - put_i
          save 0 (as diff)                 - [10 ]   - put_i
      if prev is digit:
          save len                         - [0-9]   - put_i
          save prev-new                    - [10 ]   - put_i
      else:
          save  -len                       - [0-9]   - put_i
          save chr len to space/end        - [11 ]   - put_u
          save chrs                        - [12 ]   - char put - add stream (PowerRanger - CharRanger)
   }

   also - unify signed/unsigned
        - unify char (16 values - last % f ?)

*/

class RecBase {
protected: 

    RecBase()  {}
    ~RecBase() {rcoder.done();}

    PowerRanger ranger[13];
    RCoder rcoder;

    struct {
        UINT64 rec_count;
        bool   initilized;
        // long long num[10]; - TODO: keep array of prev atoi and end pointers
    } m_last;

    struct {
        UINT32 big_i;
    } stats;

    bool m_valid;

    void range_init();

    UCHAR lenoff(UCHAR i) { return 0 + LIKELY(i < 5) ? i : 4 ; }
    UCHAR numoff(UCHAR i) { return 5 + LIKELY(i < 5) ? i : 4 ; }

    void put_len(UCHAR i, int len) {
        ranger[lenoff(i)].put_i(&rcoder, len);
    }
    int get_len(UCHAR i) {
        return ranger[lenoff(i)].get_i(&rcoder);
    }
    void put_num(UCHAR i, long long num) {
        if (ranger[numoff(i)].put_i(&rcoder, num))
            stats.big_i ++;
    }
    long long get_num( UCHAR i) {
        return ranger[numoff(i)].get_i(&rcoder);
    }
    const UCHAR* put_str(UCHAR index, const UCHAR* p, UINT32 len) {
        ranger[index].put_u(&rcoder, len);
        for (UINT32 i = 0; i < len; i++)
            ranger[11].put_c(&rcoder, p[i]);
        return p + len;
    }
    UCHAR* get_str(UCHAR index, UCHAR* p) {
        UINT32 len = ranger[index].get_u(&rcoder);
        for (UINT32 i = 0; i < len; i++)
            p[i] = ranger[11].get_c(&rcoder);
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
