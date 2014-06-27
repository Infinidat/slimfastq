
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "filer.cpp"

const char* tst_filename = "/tmp/slimfastq.filer.tst";

void test_big_file() {

    unlink(tst_filename);
    printf("Create %s..\n", tst_filename);
    FILE* out = fopen(tst_filename, "wb");
    if (not out)
        croak("Failed creating %d", 0);

    onef.init_write(out);
    for (UINT64 i = 0 ; i < (1ULL<<32)/FILER_PAGE + 100; i++)
        onef.allocate();        // bigger than 4G size

    {
        FilerSave tst("onef.tst");
        for (UCHAR c = 0; c < 0xf0; c++)
            tst.put(c);
    }

    off_t size = onef.finit_size();
    onef.finit_write();

    struct stat st;
    stat(tst_filename, &st);
    if (st.st_size != size)
        croak("Expected size %lld", size);
    else {
        int unlink_failed = unlink(tst_filename);
        assert(not unlink_failed);
        printf("Delete %s\n", tst_filename);
    }
        
}

int main(int, char**) {

    test_big_file();
    return 0;
}
