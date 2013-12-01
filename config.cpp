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

// Globals
unsigned long long g_record_count = 0;

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

void Config::statistics_dump() const {
    fprintf(stderr, ":::: Info: \n");
    for (std::map<std::string, std::string>::iterator it = info_map.begin();
         it != info_map.end();
         it ++ )
        fprintf(stderr, "%s=%s\n", it->first.c_str(), it->second.c_str());
    fprintf(stderr, ":::: Files: \n");
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
-v / -h          : internal version / this message \n\
-S               : internal statistics about a compressed file (set by -f)\n\
"); }                      // exit ?

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

    std::string usr, fil;
    bool overwrite = false;
    bool statistics = false;

    // TODO? long options 
    // const char* short_opt = "POvhd 1234 u:f:s:p:l:"; 
    const char* short_opt = "SPOvhd 1234 u:f:l:"; 
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
            exit(0);
        case 'S': statistics = true; encode = false; break;
            
        default:
            croak("Ilagal args: use -h for help");
        }

    for (char** files = argv+optind; *files; files++) {
        // TODO:
        //  if F1 exists:
        //     if sfq file: (^whoami=fastq)
        //        use as fil and set decode
        //     else if fastq file (^@)
        //        use as usr and set encode
        //  if F2 not exists, or -O (dangerous?)
        //     use as encode ? fil : usr
        //   
    }

    check_op(fil.length(), 'f');
    // m_file = strdup(fil.c_str());

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
        }
        else {
            set_info("orig.filename", "<< stdin >>");
            f_usr = stdin ;
        }
        check_fh(f_usr, usr, true);
    }
    else {
        FILE* fh = fopen(fil.c_str(), "rb");
        check_fh(fh, fil, true);
        FilerLoad::init(fh);
        load_info();
        if (statistics)
            statistics_dump();

        level = range_level(get_long("config.level", 2));

        f_usr = usr.length() ? fopen(usr.c_str(), wr_flags) : stdout;
        check_fh(f_usr, usr);
    }
}

Config::~Config() {
    if (m_info_filer) {
        delete(m_info_filer);
        FilerSave::finit();
    }

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
