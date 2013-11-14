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





#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "common.hpp"
#include "filer.hpp"
#include "config.hpp"

#define protected public

#include "qlts.hpp"
#include "recs.hpp"
#include "gens.hpp"
// #include "power_ranger.hpp"

#undef  protected

#define TITLE(X) fprintf(stderr, "UTEST: %s\n", X)

void test_filer() {
    TITLE("filer");
    const char* fname = "/tmp/utest~filer"; 
    {
        FILE *fh = fopen(fname, "w");
        assert(fh);

        FilerSave filer(fh) ;
        assert(filer.is_valid());

        filer.put(8);
        filer.put4(0x12345678);
        filer.put4(0xfeda9877);
        filer.put8(UINT64(0x0102030405060708));
        filer.put8(UINT64(0xffeeddccbbaa9988));
        for (int i = 0; i < FILER_PAGE+10; i++)
            filer.put(i&0xff);
    }
    {
        FILE *fh = fopen(fname, "r");
        assert(fh);
        bool valid;

        FilerLoad filer(fh, &valid) ;
        assert(filer.is_valid());
        UINT64 val;
#define ASSERT(SUB, VAL) val = filer.SUB(); assert(val == VAL && valid)
        ASSERT(get, 8);
        ASSERT(get4, 0x12345678);
        ASSERT(get4, UINT64(0xfeda9877));
        ASSERT(get8, UINT64(0x0102030405060708));
        ASSERT(get8, UINT64(0xffeeddccbbaa9988));
#undef  ASSERT
        for (int i = 0; i < FILER_PAGE+10; i++) {
            UCHAR  c = filer.get();
            assert(c == (i&0xff));
        }
    }
}


void fill_buf(UCHAR* buf, int start, int size) {
    for (int i = 0; i < size ; i++)
        buf[i] = '!' + ((i*7+start) % 63);
}

Config conf;
void test_qlt() {
    TITLE("qlt stream");
    const char* argv[] = {"utest", "-f", "/tmp/utest", "-O"};
    conf.init(4, (char**)argv, 777);
    {
        QltSave qlt;
        UCHAR buf[100];
        for(int i = 0; i < 101; i++) {
            fill_buf(buf, i, 91);
            qlt.save_2(buf, 91);
        }
    }
    {
        // const char* argv[] = {"utest", "-f", "/tmp/utest", "-O", "-d"};
        // Config conf(5, (char**)argv, 777);
        QltLoad qlt;
        UCHAR buf[100];
        UCHAR tst[100];
        for(int i = 0; i < 101; i++) {
            fill_buf(tst, i, 91);
            qlt.load_2(buf, 91);
            for (int j=0; j<91; j++)
                assert(tst[j] == buf[j]);
        }
    }
}

const char* fname = "/tmp/utest~filer";
bool _valid ;
RCoder * rc = NULL;
FilerLoad* filer_l = NULL;
FilerSave* filer_s = NULL;

void rc_init(bool load=0) {
    _valid = true;
    rc = new RCoder;
    assert(rc);
    if (load) {
        FILE* fh = fopen(fname, "r");
        assert(fh);
        rc->init(filer_l = new FilerLoad(fh, &_valid));
    } 
    else  {
        FILE* fh = fopen(fname, "w");
        assert(fh);
        rc->init(filer_s = new FilerSave(fh));
    }
}

void rc_finit() {
    rc -> done();
    DELETE (rc);
    DELETE (filer_l);
    DELETE (filer_s);
}

void test_log64_ranger() {
    TITLE("log64 put/get");
    {
        rc_init(0);
        Log64Ranger ranger;
        BZERO(ranger);
        for (int i = 0; i < 300; i++)
            ranger.put(rc, i&0x3f);
        for (int i = 0; i < 1000; i+=17)
            ranger.put(rc, i&0x3f);
        rc_finit();
    }
    {
        rc_init(1);
        Log64Ranger ranger;
        BZERO(ranger);
        for (int i = 0; i < 300; i++) {
            UCHAR c = ranger.get(rc);
            assert(c == (i&0x3f));
        }
        for (int i = 0; i < 1000; i+=17) {
            UCHAR c = ranger.get(rc);
            assert(c == (i&0x3f));
        }
        rc_finit();
    }
}

void test_power_ranger() {
    TITLE("ranger put_c/get_c");
    {
        rc_init(0);
        PowerRanger ranger;
        BZERO(ranger);
        for (int i = 0; i < 300; i++)
            ranger.put_c(rc, i&0xff);
        for (int i = 0; i < 1000; i+=17)
            ranger.put_c(rc, i&0xff);
        rc_finit();
    }
    {
        rc_init(1);
        PowerRanger ranger;
        BZERO(ranger);
        for (int i = 0; i < 300; i++) {
            UCHAR c = ranger.get_c(rc);
            assert(c == (i&0xff));
        }
        for (int i = 0; i < 1000; i+=17) {
            UCHAR c = ranger.get_c(rc);
            assert(c == (i&0xff));
        }
        rc_finit();
    }
    TITLE("ranger put_i / get_i");
    int arr[] = { -121, -122, -1, 575};
    {
        rc_init();
        PowerRanger ranger;
        BZERO(ranger);
        for (int i = 0; i < 4; i++)
            ranger.put_i(rc, arr[i]);
        for (int i = -300; i < 300; i++)
            ranger.put_i(rc, i);
        for (int i = -100; i < 100; i++)
            ranger.put_i(rc, i*7);
        rc_finit();
    }
    {
        rc_init(1);
        PowerRanger ranger;
        BZERO(ranger);
        for (int i = 0; i < 4; i++){
            int c = ranger.get_i(rc);
            assert(c == arr[i]);
        }
        for (int i = -300; i < 300; i++) {
            int c = ranger.get_i(rc);
            assert(c == i);
        }
        for (int i = -100; i < 100; i++) {
            int c = ranger.get_i(rc);
            assert(c == i*7);
        }
        rc_finit();
    }
    TITLE("ranger put_u / get_u");
    {
        rc_init();
        PowerRanger ranger;
        BZERO(ranger);
        for (int i = 0; i < 300; i++)
            ranger.put_u(rc, i);
        for (int i = 1000; i ; i--)
            ranger.put_u(rc, i*77);

        rc_finit();
    }
    {
        rc_init(1);
        PowerRanger ranger;
        BZERO(ranger);
        for (int i = 0; i < 300; i++) {
            int c = ranger.get_u(rc);
            assert(c == i);
        }
        for (int i = 1000; i ; i--) {
            int c = ranger.get_u(rc);
            assert(c == i*77);
        }
        rc_finit();
    }
}

void test_power_ranger_extra() {
    TITLE("extra ranger");
    UINT64 array[] =
        { 0xfffffffffff, 0x123456789abcd, 0, 4,
          0xfffffffffff, 0x123456789abcd, 0, 4,
          0xa, 0xffff, 0xfffe, 100
        };
    long long arrai[] =
        { -0x7ffffff, 0x7ffffff, -100000L, 9999L,
          -1, 2994389, -2, -3
        };
    {
        FILE *fh = fopen(fname, "w");
        assert(fh);

        FilerSave filer(fh) ;
        assert(filer.is_valid());

        PowerRanger ranger;
        RCoder rcoder;
        rcoder.init(&filer);
        BZERO(ranger);
        RCoder* r = &rcoder;

        // bug hunt
        ranger.put_i(r, -1);
        ranger.put_u(r, '3');
        ranger.put_i(r, 2994389);
        ranger.put_u(r, 2308);
        for (int i = 0; i < 300; i++)
            ranger.put_u(r, (i&0x7f));
        for (UINT64 i=0; i < 1000; i++)
            ranger.put_u(r, i);
        for (UINT64 i=0xfff0; i < 0x10234; i++) 
            ranger.put_u(r, i);
        for (int i = 0; i < 12; i++)
            ranger.put_u(r, array[i]);
        for (int i = -300; i < 300; i++)
            ranger.put_i(r, i);
        for (int i = 0; i < 8; i++)
            ranger.put_i(r, 0|arrai[i]);
        rcoder.done();
    }
    {
        FILE *fh = fopen(fname, "r");
        assert(fh);
        bool valid;

        FilerLoad filer(fh, &valid) ;
        assert(filer.is_valid());

        PowerRanger ranger;
        RCoder rcoder;
        rcoder.init(&filer);
        BZERO(ranger);
        RCoder* r = &rcoder;

        assert(ranger.get_i(r) == -1);
        assert(ranger.get_u(r) == '3');
        assert(ranger.get_i(r) == 2994389);
        assert(ranger.get_u(r) == 2308);
        for (int i = 0; i < 300; i++) {
            UCHAR c = ranger.get_u(r);
            assert(c == (i&0x7f));
        }
        for (UINT64 i=0; i < 1000; i++) {
            UINT64 u = ranger.get_u(r);
            assert(u == i);
        }
        for (UINT64 i=0xfff0; i < 0x10234; i++) {
            UINT64 u = ranger.get_u(r);
            assert(u == i);
        }
        for (int i = 0; i < 12; i++) {
            UINT64 u = ranger.get_u(r);
            assert(u == array[i]);
        }
        for (int i = -300; i < 300; i++) {
            long l = ranger.get_i(r);
            assert(l == i);
        }
        for (int i = 0; i < 8; i++) {
            long u = ranger.get_i(r);
            assert(u == arrai[i]);            
        }
    }
}

void test_recbase() {

    TITLE("test RecBase");
    const UCHAR* mystr = (const UCHAR*)"welcome to Amsterdam had have a )*(&^(*4758*^&%)) +++~ time!";
    int mystr_len = strlen((const char*)mystr);
    {
        FILE *fh = fopen(fname, "w");
        assert(fh);

        FilerSave filer(fh) ;
        assert(filer.is_valid());

        RecBase base ;
        BZERO(base);
        base.range_init();
        base.rcoder.init(&filer);

        base.put_str(0, mystr, mystr_len);

        for (int i = -1000; i < 2000; i++) {
            base.put_len(0, i*11+9);
            base.put_num(0, i*7);
        }

    }
    {
        FILE *fh = fopen(fname, "r");
        assert(fh);
        bool valid;

        FilerLoad filer(fh, &valid) ;
        assert(filer.is_valid());
        RecBase base ;
        BZERO(base);
        base.range_init();
        base.rcoder.init(&filer);

        UCHAR str[1000];
        UCHAR* p = base.get_str(0, str);
        assert(0 == strcmp((const char*)str, (const char*)mystr));
        assert(p == str + mystr_len);

        for (int i = -1000; i < 2000; i++) {
            int len = base.get_len(0);
            assert(len == i*11+9);
            long long num = base.get_num(0);
            assert(num == i*7);
        }        
    }
}

int main(int argc, char *argv[]) {

    test_filer();
    test_log64_ranger();
    test_power_ranger();
    test_qlt();
    test_power_ranger_extra();
    test_recbase();

    return 0;
}
