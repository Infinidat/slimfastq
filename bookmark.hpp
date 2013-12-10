
#ifndef FILER_BOOKMARK_H
#define FILER_BOOKMARK_H

#define MAX_BMK_FILES 350

struct BookMark {
    struct {
        UINT64 rec_count;
        UINT64 gen_count;
        UINT64 offset;
        UINT16 nfiles;
    } mark;

    struct {
        UINT32 onef_i;
        UINT32 node_p;
        UINT32 node_i;
        UINT64 page_cnt;
        UINT64 page_cur;
    } file[MAX_BMK_FILES];

    int get_my_i(UINT32 onef_i) {
        for (int i = 0; i < mark.nfiles; i++)
            if (file[i].onef_i == onef_i)
                return i;
        return -1;
    }
} PACKED ;

#endif
