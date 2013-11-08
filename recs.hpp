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
// #include "pager.hpp"
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
          save len                         - [0]   - put_i
          save 0 (as diff)                 - [1]   - put_i
      if prev is digit:
          save len                         - [0]   - put_i
          save prev-new                    - [1]   - put_i
      else:
          save  -len                       - [0]   - put_i
          save chr len to space/end        - [2]   - put_u
          save chrs                        - [3]   - char put - add stream (PowerRanger - CharRanger)
   }

   also - unify signed/unsigned
        - unify char (16 values - last % f ?)

*/

class RecBase {
protected: 

    enum exception_t {
        ET_v0,
        ET_v1,
        ET_v2,
        ET_v3,
        ET_v9,
        ET_al,
        ET_iseq,
        ET_END
    };

    RecBase()  {}
    ~RecBase() {rcoder.done();}

    PowerRanger ranger[10];
    RCoder rcoder;
    PowerRanger rangerx[10];
    RCoder rcoderx;

    const char* m_ids[10];
    int m_len[10];

    struct {
        UINT64 line_n;
        // UINT64 x;
        // UINT64 y;
        UINT64 n[10];
        UINT64 rec_count;
        UINT64 rec_count_tag;
        UCHAR alal;
    } m_last;

    struct {
        UINT32 big_u;
        UINT32 big_i;
        UINT32 exceptions;
    } stats;

    int    m_type;
    bool   m_valid;

    // const Config* m_conf;

    void range_init();

    // Yeh yeh, I know it should be getter and putter but I'll keep it aligned
    void put_i(int i, long long num, UINT64* old=NULL) {
        if (ranger[i].put_i(&rcoder, num, old))
            stats.big_i ++;
    }
    void put_u(int i, UINT64    num, UINT64* old=NULL) {
        if (ranger[i].put_u(&rcoder, num, old))
            stats.big_u ++;
    }
    long long get_i(int i, UINT64* old=NULL) {
        return ranger[i].get_i(&rcoder, old);
    }
    UINT64    get_u(int i, UINT64* old=NULL) {
        return ranger[i].get_u(&rcoder, old);
    };

    void putx_i(int i, long long num, UINT64* old=NULL) {
        rangerx[i].put_i(&rcoderx, num, old);
    }
    void putx_u(int i, UINT64    num, UINT64* old=NULL) {
        rangerx[i].put_u(&rcoderx, num, old);
    }
    long long getx_i(int i, UINT64* old=NULL) {
        return rangerx[i].get_i(&rcoderx, old);
    }
    UINT64    getx_u(int i, UINT64* old=NULL) {
        return rangerx[i].get_u(&rcoderx, old);
    };
};

class RecSave : private RecBase {
public:
     RecSave();
    ~RecSave();

    bool save_1(const UCHAR* buf, const UCHAR* end) { return save_2(buf, end); }
    bool save_2(const UCHAR* buf, const UCHAR* end);
    bool save_3(const UCHAR* buf, const UCHAR* end) { return save_2(buf, end); }
private:
    void save_allele(char a);
    bool save_t_1(const UCHAR* buf, const UCHAR* end);
    bool save_t_3(const UCHAR* buf, const UCHAR* end);
    bool save_t_5(const UCHAR* buf, const UCHAR* end);
    bool save_t_6(const UCHAR* buf, const UCHAR* end);
    bool save_t_7(const UCHAR* buf, const UCHAR* end);

    void update(exception_t type, int val);

    void determine_rec_type(const UCHAR* buf, const UCHAR* end);
    int  get_rec_type(const char* start);

    FilerSave* filer;
    FilerSave* filerx;
};

class RecLoad : private RecBase {
public:
     RecLoad();
    ~RecLoad();

    inline bool is_valid() {return m_valid;}
    size_t load_1(UCHAR* buf) { return load_2(buf) ;}
    size_t load_2(UCHAR* buf);
    size_t load_3(UCHAR* buf) { return load_2(buf) ;}

private:

    size_t load_t_1(UCHAR* buf);
    size_t load_t_3(UCHAR* buf);
    size_t load_t_5(UCHAR* buf);
    size_t load_t_6(UCHAR* buf);
    size_t load_t_7(UCHAR* buf);
    void update(); // update last values

    void determine_ids(int size) ;

    FilerLoad* filer;
    FilerLoad* filerx;
};



#endif
