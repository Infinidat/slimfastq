/***********************************************************************************************************************/
/* This program was written by Josef Ezra  <jezra@infinidat.com>                                                       */
/* Copyright (c) 2013, infinidat                                                                                       */
/* All rights reserved.                                                                                                */
/*                                                                                                                     */
/* Redistribution and use in source and binary forms, with or without modification, are permitted provided that        */
/* the following conditions are met:                                                                                   */
/*                                                                                                                     */
/* Redistributions of source code must retain the above copyright notice, this list of conditions and the following    */
/* disclaimer.                                                                                                         */
/* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following */
/* disclaimer in the documentation and/or other materials provided with the distribution.                              */
/* Neither the name of the <ORGANIZATION> nor the names of its contributors may be used to endorse or promote products */
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

bool _valid ;
RCoder * rc = NULL;
FilerLoad* filer_l = NULL;
FilerSave* filer_s = NULL;

void rc_init(bool load=0) {
    const char* fname = "/tmp/utest~filer";
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
    TITLE("power ranger put/get");
    {
        rc_init(0);
        PowerRanger ranger;
        BZERO(ranger);
        for (int i = 0; i < 300; i++)
            ranger.put(rc, 0, i&0xff);
        for (int i = 0; i < 1000; i+=17)
            ranger.put(rc, 0, i&0xff);
        rc_finit();
    }
    {
        rc_init(1);
        PowerRanger ranger;
        BZERO(ranger);
        for (int i = 0; i < 300; i++) {
            UCHAR c = ranger.get(rc, 0);
            assert(c == (i&0xff));
        }
        for (int i = 0; i < 1000; i+=17) {
            UCHAR c = ranger.get(rc, 0);
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
            ranger.put(rc, 0, i&0xff);
        rc_finit();
    }
    {
        rc_init(1);
        PowerRanger ranger;
        BZERO(ranger);
        for (int i = 0; i < 300; i++) {
            int c = ranger.get(rc, 0);
            assert(c == (i&0xff));
        }
        rc_finit();
    }
}

void test_recbase() {
    const char* fname = "/tmp/utest~filer";
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

        RecBase base ;
        BZERO(base);
        base.range_init();
        base.rcoder.init(&filer);
        // bug hunt
        base.put_i(0, -1);
        base.put_u(1, '3');
        base.put_i(0, 2994389);
        base.put_u(2, 2308);
        for (int i = 0; i < 300; i++)
            base.put_u(0, (i&0x7f));
        for (UINT64 i=0; i < 1000; i++)
            base.put_u(0, i);
        for (UINT64 i=0xfff0; i < 0x10234; i++) 
            base.put_u(0, i);
        for (int i = 0; i < 12; i++)
            base.put_u(0, array[i]);
        for (int i = -300; i < 300; i++)
            base.put_i(1, i);
        for (int i = 0; i < 8; i++)
            base.put_i(1, 0|arrai[i]);
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
        assert(base.get_i(0) == -1);
        assert(base.get_u(1) == '3');
        assert(base.get_i(0) == 2994389);
        assert(base.get_u(2) == 2308);
        for (int i = 0; i < 300; i++) {
            UCHAR c = base.get_u(0);
            assert(c == (i&0x7f));
        }
        for (UINT64 i=0; i < 1000; i++) {
            UINT64 u = base.get_u(0);
            assert(u == i);
        }
        for (UINT64 i=0xfff0; i < 0x10234; i++) {
            UINT64 u = base.get_u(0);
            assert(u == i);
        }
        for (int i = 0; i < 12; i++) {
            UINT64 u = base.get_u(0);
            assert(u == array[i]);
        }
        for (int i = -300; i < 300; i++) {
            long l = base.get_i(1);
            assert(l == i);
        }
        for (int i = 0; i < 8; i++) {
            long u = base.get_i(1);
            assert(u == arrai[i]);            
        }
    }
}

int main(int argc, char *argv[]) {

    test_filer();
    test_log64_ranger();
    test_power_ranger();
    test_qlt();
    test_recbase();

    return 0;
}
