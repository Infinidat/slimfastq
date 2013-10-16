

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "common.hpp"
#include "filer.hpp"
#include "config.hpp"
#include "fq_qlts.hpp"
#include "fq_recs.hpp"

#define TITLE(X)

void test_filer() {
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

void test_qlt() {
    const char* argv[] = {"utest", "-f", "/tmp/utest", "-O"};
    Config conf(4, (char**)argv, 777);
    {
        QltSave qlt(&conf);
        UCHAR buf[100];
        for(int i = 0; i < 101; i++) {
            fill_buf(buf, i, 91);
            qlt.save(buf, 91);
        }
    }
    {
        // const char* argv[] = {"utest", "-f", "/tmp/utest", "-O", "-d"};
        // Config conf(5, (char**)argv, 777);
        QltLoad qlt(&conf);
        UCHAR buf[100];
        UCHAR tst[100];
        for(int i = 0; i < 101; i++) {
            fill_buf(tst, i, 91);
            qlt.load(buf, 91);
            for (int j=0; j<91; j++)
                assert(tst[j] == buf[j]);
        }
    }
}

void test_ranger() {
    const char* fname = "/tmp/utest~filer";
    {
        FILE *fh = fopen(fname, "w");
        assert(fh);
        FilerSave filer(fh) ;
        assert(filer.is_valid());
        RangeCoder rcoder;
        rcoder.init(&filer);
        PowerRanger<8> ranger;
        BZERO(ranger);
        for (int i = 0; i < 300; i++)
            ranger.put(&rcoder, i&0xff);
        rcoder.done();
    }
    {
        FILE *fh = fopen(fname, "r");
        assert(fh);
        bool valid;
        FilerLoad filer(fh, &valid) ;
        assert(filer.is_valid());
        RangeCoder rcoder;
        rcoder.init(&filer);
        PowerRanger<8> ranger;
        BZERO(ranger);
        for (int i = 0; i < 300; i++) {
            UCHAR c = ranger.get(&rcoder);
            assert(c == (i&0xff));
        }
        rcoder.done();
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
    test_ranger();
    test_qlt();
    test_recbase();

    return 0;
}
