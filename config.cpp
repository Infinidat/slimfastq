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




#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include <string>
#include <map>
#include <iostream>
#include <fstream>

#include "config.hpp"
#include "usrs.hpp"

// Globals
unsigned long long g_record_count = 0;
unsigned long long g_genofs_count = 0;

typedef std::map<std::string, std::string> info_t;
typedef std::pair<std::string, std::string> info_pair;
info_t info_map;

void croak(const char* msg) {
    if (errno) 
        fprintf(stderr, "error: %s: %s\n", msg, strerror(errno));
    else 
        fprintf(stderr, "error: %s\n", msg);
    exit(1);
}

void croak(const char* fmt, long long num) {
    fprintf(stderr, fmt, num);
    fprintf(stderr, "\n");
    exit(1);
}

void Config::chapters_dump() const {
    UsrLoad::bookmark_dump();
    exit(0);
}
void Config::statistics_dump() const {
    fprintf(stderr, ":::: Info ::::\n");
    for (std::map<std::string, std::string>::iterator it = info_map.begin();
         it != info_map.end();
         it ++ )
        fprintf(stderr, "%s\t%s=\t%s\n", it->first.c_str(), it->first.length() < 9? "\t" : "", it->second.c_str());
    fprintf(stderr, "\n:::: Files ::::\n");
    FilerLoad::confess();
    exit(0);
}

void Config::load_info() const {

    info_map.clear();
    char line[0x200];
    bool valid;
    FilerLoad filer(42, &valid);
    while (valid) {
        for (int i = 0; i < 0x200; i++) {
            line[i] = filer.get();
            if (not valid or
                line[i] == '\n')
                line[i] = 0;
            if (line[i] == 0)
                break;
        }
        char* pos = index(line, '=');
        if (pos) {
            *pos = 0;
            info_map.insert(info_pair (line, pos+1));
        }
    }
}

bool Config::has_info(const char* key) const {
    const char* something = info_map[key].c_str();
    return 0 != strlen(something);
}

const char* Config::get_info(const char* key) const {
    const char* something = info_map[key].c_str();
    if (0 == strlen(something)) {
        // fprintf(stderr, "%s: no value for '%s'\n", m_info_filename, key);
        exit(1);
    }
    return something;
}

bool Config::get_bool(const char* key) const {
    const char* something = info_map[key].c_str();
    return strlen(something) and *something != '0';
}

long long Config::get_long(const char* key, long long val) const {
    const char* something = info_map[key].c_str();
    return strlen(something) ? atoll(something) : val;
}

void put_str(FilerSave* filer, const char* str) {
    int sanity = 0x200;
    while (*str and --sanity)
        filer->put(*str++);
    if (not sanity)
        croak("oversize string value");
}

void Config::set_info(const char* key, const char* val) const {
    
    put_str(m_info_filer, key);
    m_info_filer->put('=');
    put_str(m_info_filer, val);
    m_info_filer->put('\n');
    
    info_map.insert(info_pair(key, val));
}

void Config::set_info(const char* key, long long num) const {
    char buf[40];
    sprintf(buf, "%lld", num);
    set_info(key, buf);
}

void Config::usage() const {
    printf("\
Usage: \n\
-u  usr-filename : (default: stdin)\n\
-f comp-filename : reuired - compressed\n\
-d               : decode (instead of encoding) \n\
-O               : overwrite existing files\n\
-l level         : compression level 1 to 4 (default is 2 ) \n\
-1, -2, -3, -4   : alias for -l 1, -l 2, etc \n\
 where levels are:\n\
 1: worse compression, yet uses less than 4M memory \n\
 2: default - use about 30M memory, resonable compression \n\
 3: best compression, use about 80M \n\
 4: compress little more, but very costly (competition mode?) \n\
\n\
-M <SIZE>        : bookmark each <SIZE>MB as a chapter (default 200MB) \n\
-I               : index chapters \n\
-C <X>           : decopress chapter X only \n\
\n\
-v / -h          : internal version / this message \n\
-S               : internal statistics about a compressed file (the value of -f)\n\
\n\
Intuitive use of 'slimfastq A B' : \n\
If A appears to be a fastq file, and:\n\
    B does not exists (or -O option is used): compress A to B \n\
If A appears to be a slimfastq file, and: \n\
    B does not exist (or -O option is used): decompress A to B \n\
    no B is specified: dcompress to stdout \n\
"); exit(0);
}

// (DISABLED -TBD)-s size          : set partition to <size> (megabyte units) \n\ -
// (DISABLED -TBD)-p partition     : only open this partition (-d implied) \n\    -
// \n\ -

static void check_fh(FILE* f, std::string name, bool read=false) {
    if (f) return;
    fprintf(stderr, "Can't %s file '%s': %s\n",
            read?"read":"write", name.c_str(), strerror(errno));
    exit(1);
}

static void check_op(int something, char chr) {
    if (something) return;
    fprintf(stderr, "Missing essential argument: -%c\n", chr);
    exit(1);
}

Config::Config(){

    version = 0;

    encode = true;
    profiling = false;
    level = 2;
    m_info_filer = NULL;
}

static int range_level(int level) {
    return
        level > 4 ? 4 :
        level < 1 ? 1 :
        level ;
}

static bool initialized = false;
void Config::init(int argc, char **argv, int ver) {
    if (initialized)
        croak("Internal error: 2nd Config init");

    initialized = true;
    version = ver;
    chapter_size = 0;

    std::string usr, fil;
    bool overwrite = false;
    bool statistics = false;
    bool list_chapters = false;

    // TODO? long options 
    // const char* short_opt = "POvhd 1234 u:f:s:p:l:";
    if (argc == 1) usage();
    const char* short_opt = "SPOvhd 1234 I u:f:l: C:M: "; 
    for ( int opt = getopt(argc, argv, short_opt);
          opt != -1;
          opt     = getopt(argc, argv, short_opt))
        switch (opt) {
        case 'u': usr = optarg ; break;
        case 'f': fil = optarg ; break;

        // case 's':
        //     partition.size = strtoll(optarg, 0, 0) * (1<<20);
        //     break;
        // case 'p': 
        //     encode = false ;
        //     partition.size = 1;
        //     partition.param = strtoll(optarg, 0, (strlen(optarg) == 10 and optarg[0] == '0') ? 16 : 0 );
        //     break;
            
        case 'l': level = strtoll(optarg, 0, 0);  break;
        case '1': case '2' : case '3': case '4':
            level = opt - '0'; break;

        case 'd': encode     = false; break;
        case 'O': overwrite  = true ; break;
        case 'P': profiling  = true ; break;
        case 'v':
            printf("Version %u\n", version);
            exit(0);
        case 'h':
            usage();
        case 'S': statistics = true; encode = false; break;

        case 'I': // list chapters index
            list_chapters = true;
            break;
        case 'C': // Decomp chapter X
            encode = false;
            chapter_bookmark = 1 + (optarg ? strtoll(optarg, 0, 0) : 0);
            break;
            
        case 'M': // mark chapters according to size
            encode = true;
            chapter_size = optarg ? strtoll(optarg, 0, 0) : 200 ;
            break;
            
        default:
            croak("Ilagal args: use -h for help");
        }

    while (optind < argc) {
        char *file = argv[optind ++];
        FILE* fh = fopen(file, "rb");
        if (! fh) {
            if  (not encode and not usr.length())
                usr = file;
            else if (encode and not fil.length())
                fil = file;
            else {
                fprintf(stderr, "What am I suppose to do with '%s'?\n (please specify explicitly with -f/-u prefix)\n(Note: not an existing file)\n",
                        file);
                exit(1);
            }
            continue;
        }
        const char* sfqstamp = "whoami=slimfastq" ;
        char initline[20];
        BZERO(initline);
        size_t cnt = fread(initline, 1, 19, fh);
        if (cnt and
            not fil.length() and
            0 == strncmp(initline, sfqstamp, strlen(sfqstamp))) {
            fil = file;
            encode = !! usr.length();
        }
        else if (cnt and
                 not usr.length() and
                 initline[0] == '@') {
            usr = file;
        }
        else if (not usr.length() and
                 not encode and
                 (not cnt or overwrite)) {
            usr = file;
        }
        else {
            fprintf(stderr, "What am I suppose to do with '%s'?\n (please specify explicitly with -f/-u prefix)\n(Note: file exists!)\n", file);
            exit(1);
        }
    }

    check_op(fil.length(), 'f');
    const char* wr_flags = overwrite ? "wb" : "wbx" ;
    if (encode) {
        FILE* fh = fopen(fil.c_str(), wr_flags);
        check_fh(fh, fil, false);
        FilerSave::init(fh);
        m_info_filer = new FilerSave(42);
        set_info("whoami", "slimfastq");
        set_info("version", version);
        set_info("config.level", range_level(level));

        if (usr.length()) {
            set_info("orig.filename", usr.c_str());
            f_usr = fopen(usr.c_str(), "rb") ;
            check_fh(f_usr, usr, true);
            fseek(f_usr, 0L, SEEK_END);
            set_info("orig.size", ftell(f_usr));
            fseek(f_usr, 0L, SEEK_SET);
        }
        else {
            set_info("orig.filename", "<< stdin >>");
            f_usr = stdin ;
        }
    }
    else {
        FILE* fh = fopen(fil.c_str(), "rb");
        check_fh(fh, fil, true);
        FilerLoad::init(fh);
        load_info();
        if (statistics)
            statistics_dump();
        if (list_chapters)
            chapters_dump();

        level = range_level(get_long("config.level", 2));

        f_usr = usr.length() ? fopen(usr.c_str(), wr_flags) : stdout;
        check_fh(f_usr, usr);
    }
}

void Config::finit() {
    if (m_info_filer) {
        delete(m_info_filer);
        FilerSave::finit();
        m_info_filer = NULL;
    }
}

Config::~Config() {
    finit();

#if 0
 TODO:
        if (ferror(in)) {
            fprintf(stderr, "Read error: %s\n", strerror(errno));
            return 1;
        }
        if (ferror(out)) {
            fprintf(stderr, "Write error: %s\n", strerror(errno));
            return 2;
        }
#endif

}
