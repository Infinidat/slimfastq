

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "common.hpp"
#include "filer.hpp"
#include "config.hpp"
#include "fq_qlts.hpp"


#define TITLE(X)

void test_filer() {
    const char* fname = "/tmp/utest.file"; 
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

int main(int argc, char *argv[]) {

    test_filer();
    test_qlt();

    return 0;
}
