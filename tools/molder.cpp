//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include <string>
#include <map>
#include "common.hpp"
#include "pager.hpp"

static const int version = 1;   // inernal version
#define PR_BASE 5
// 31 bits prime = 0x7fffffff, 32 - 0xfffffffb
#define PR_MODL 0xfffffffb
#define PR_POWR 0x86eed05a


struct eqstr { bool operator()(UINT32 s1, UINT32 s2) const { return (s1 == s2) ; } };

#define HASH_DENSE 1
#ifdef  HASH_DENSE
#include <google/dense_hash_map>
typedef google::dense_hash_map<UINT32,UINT32, std::hash<UINT32>, eqstr> my_hash;
#else
#include <google/sparse_hash_map>
typedef google::sparse_hash_map<UINT32,UINT32, std::hash<UINT32>, eqstr> my_hash;
#endif
#include <unordered_map>

void croak(const char* msg) {
    if (errno) 
        fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    else 
        fprintf(stderr, "%s\n", msg);
    exit(1);
}

void croak(const char* fmt, long long num) {
    fprintf(stderr, fmt, num);
    fprintf(stderr, "\n");
    exit(1);
}

void usage() {
    printf("\
TODO : add usage \n\
\n\
");
}

static void check_fh(FILE* f, const char* name, bool read=false) {
    if (f) return;
    fprintf(stderr, "Can't %s file '%s': %s\n",
            read?"read":"write", name, strerror(errno));
    exit(1);
}

static void check_op(int something, char chr) {
    if (something) return;
    fprintf(stderr, "Missing essential argument: -%c\n", chr);
    exit(1);
}

const char* withsuffix(std::string str, const char* suffix) {
    str += suffix;
    return strdup(str.c_str());
}

// using google::dense_hash_map;  

class Molder {
    FILE  *f_usr, *f_key, *f_seq, *f_rec;
    PagerSave* pgkey;
    PagerSave* pgseq;

    // TODO: use google's sparasehash,
    // where molder uses  sparse_hash_set
    // and izg-fq uses dense_hash_map
    // std::map<UINT32, UINT32> all;
    my_hash all;

    UINT32 f_offs, c_offs;
    UINT32 c_bases;

    // TODO: verify no fastq file holds lines shorter that this one
#define LLINE 76

    UCHAR raw_a [LLINE] ;
    UCHAR raw_i ;
    UINT64 rabin; // TODO: rrabin inplace 
    void add_fc(UCHAR c) {
        UINT64 oldc = raw_a[raw_i] * PR_POWR % PR_MODL;
        rabin *= PR_BASE;
        rabin += raw_a[raw_i] = c;
        rabin %= PR_MODL;
        if (rabin < oldc) 
            rabin += PR_MODL;
        rabin -= oldc;
        ++ raw_i %= LLINE;
        ++ c_bases;
    }
    bool exists() {
        my_hash::iterator it = all.find(rabin);
        return it != all.end();
    }
    void add_c(UCHAR c) {
        // if unique key, save key + cur c
        // calc next key
        if (not exists()) {
            // UINT64 tmp = (key << 32) | f_offs;
            // pgseq->put(tmp);
            // fputc(c, f_seq);
            all[rabin] = f_offs++;
        }
        add_fc(c);
    }
    void process() {
        for(int
            c = fgetc(f_usr);
            c != EOF;
            c = fgetc(f_usr)) {
            int val = 4;
            switch(c) {
            case '>':
                while (c != '\n') c = fgetc(f_usr);
                break;
            case '\n': case 'N': case 'n': break;
            default: croak("unexpected value: %c", c);
            case 'a': case 'A' : val = 1; break;
            case 'c': case 'C' : val = 2; break;
            case 'g': case 'G' : val = 3; break;
            case 't': case 'T' : val = 4; break; 
            }
            if (val < 3)
                add_c(val);
        }
    }
    void init_raw() {
        for (int i = 0, c = fgetc(f_usr);
             i < LLINE and c != EOF;
             c = fgetc(f_usr)) {
            UCHAR val = 4;
            switch(c) {
            case '>': while (c != '\n') c = fgetc(f_usr); continue;
            case '\n': case 'N': case 'n':                continue;
            default: croak("unexpected value: %c", c);    continue;

            case 'a': case 'A' : val = 0; break;
            case 'c': case 'C' : val = 1; break;
            case 'g': case 'G' : val = 2; break;
            case 't': case 'T' : val = 3; break; 
            }
            assert (val < 4);
            i ++;
            add_fc(val);
        }
    }
public:
    Molder(const char* usr, const char* key, const char* seq) {

#ifdef HASH_DENSE
        all.set_empty_key(0);
#endif

        f_usr = *usr ? fopen(usr, "rb") : stdin ;
        check_fh(f_usr, usr, true);
        const char *write_fl = "wb"; // TODO: "ab"

        f_key = fopen(key, write_fl);
        check_fh(f_key, key);
        pgkey = new PagerSave(f_key);

        f_seq = fopen(seq, write_fl);
        check_fh(f_seq, seq);

        c_bases = 0;
        // init keys
        f_offs = ftell(f_seq);
        c_offs = 0;

        rabin = 0;
        BZERO(raw_a);
        raw_i = 0;

        init_raw();
        process();
    }
    ~Molder() {
        fprintf(stdout, "%u bases, %lu keys\n", c_bases, all.size());
    }

};

int main(int argc, char** argv) {

    std::string usr, fil;
    const char* short_opt = "hvf:u:";
    for ( int opt = getopt(argc, argv, short_opt);
          opt != -1;
          opt     = getopt(argc, argv, short_opt))
        switch(opt) {
        case 'u': usr = optarg ; break;
        case 'f': fil = optarg ; break;
        case 'v': printf("Version: %u\n", version); exit(0);
        case 'h': usage(); exit(0);
        };

    check_op(fil.length(), 'f');

    const char *key = withsuffix(fil, ".key");
    const char *seq = withsuffix(fil, ".seq");
    // const char *rec = withsuffix(fil, ".rec");
    Molder mold(usr.c_str(), key, seq);
    return 0;
}


