//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

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
typedef std::map<std::string, std::string> info_t;
typedef std::map<std::string, std::string>::iterator info_itr_t;
typedef std::pair<std::string, std::string> info_pair;
info_t info_map;
std::ofstream filename_stream;

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

void Config::load_info() const {
    // FILE* f = fopen(m_info_filename, "r");
    std::ifstream is_f (m_info_filename);
    if (not is_f.is_open())
        croak(m_info_filename);

    info_map.clear();
    std::string line;
    while(std::getline(is_f, line)) {
        int pos = line.find_first_of('=');
        if (pos > 0)
            info_map.insert(info_pair (line.substr(0, pos), line.substr(pos+1)));
    }
    is_f.close();
}

void Config::save_info() const {
    std::ofstream os_f(m_info_filename);
    if (not os_f.is_open())
        croak(m_info_filename);

    for (info_itr_t
         itr  = info_map.begin();
         itr != info_map.end();
         itr ++)
        os_f << itr->first << "=" << itr->second << "\n";
    m_saved = true;
}

bool Config::has_info(const char* key) const {
    const char* something = info_map[key].c_str();
    return 0 != strlen(something);
}

const char* Config::get_info(const char* key) const {
    const char* something = info_map[key].c_str();
    if (0 == strlen(something)) {
        fprintf(stderr, "%s: no value for '%s'\n", m_info_filename, key);
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

void Config::set_info(const char* key, const char* val) const {
    info_map.insert(info_pair(key, val));
    if (m_saved) // not during startup?
        save_info();
}

void Config::set_info(const char* key, long long num) const {
    char buf[40];
    sprintf(buf, "%lld", num);
    set_info(key, buf);
}

void Config::usage() const {
    printf("\
Usage: \n\
-u usr-filename  : (default: stdin)\n\
-f comp-basename : reuired \n\
-d               : decode (instead of encoding) \n\
-O               : overwrite existing files\n\
\n\
-s size          : set partition to <size> (megabyte units) \n\
-p partition     : only open this partition (-d implied) \n\
\n\
-P               : profile mode (stop after ) \n\
-v / -h          : internal version / this message \n\
"); }                      // exit ?

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

const char* withsuffix(const char* name, const char* suffix) {
    std::string str = name;
    return strdup((str + suffix).c_str());
}

// void Config::set_partition(const char* str) {
//     m_part[0] = '.';
//     long long num = strtoll(str, NULL, 
//                             (strlen(str) == 10 and
//                              str[0] == '0') ?
//                             16 : 0 );
//     if (num < 0)                // TODO: orig file size
//         croak("illegal 'p' param %lld", num);
// 
//     bool valid;
//     PagerLoad ppart(m_conf->open_r("part"), &valid);
//     /* UINT64 version = */ ppart.get(); 
//     for (int i = 0 ; valid ; i++) {
//         UINT64 offs = ppart.get();
//         UINT64 nrec = ppart.get();
//         if (num == i or
//             num == offs) {
//             sprintf(&m_part[1], "%010llx", num);
//             partition_size = nrec;
//             return;
//         }
//     }
//     croak("couldn't find matching partition of -p %lld", num);
// }

static bool already_exists = false;
Config::Config(int argc, char **argv, int ver) {
    if (already_exists)
        croak("Internal error: 2nd Config");
    already_exists = true;

    m_part[0] = 0;
    version = ver;
    encode = true;
    profiling = false;
    m_saved = false;
    bzero(&partition, sizeof(partition));

    std::string usr, fil;
    bool overwrite = false;

    // TODO? long options 
    const char* short_opt = "POvhd u:f: s:p:"; 
    for ( int opt = getopt(argc, argv, short_opt);
          opt != -1;
          opt     = getopt(argc, argv, short_opt))
        switch (opt) {
        case 'u': usr = optarg ; break;
        case 'f': fil = optarg ; break;

        case 's':
            partition.size = strtoll(optarg, 0, 0) * (1<<20);
            break;
        case 'p': 
            encode = false ;
            partition.size = 1;
            partition.param = strtoll(optarg, 0, (strlen(optarg) == 10 and optarg[0] == '0') ? 16 : 0 );
            break;
            
        case 'O': overwrite = true; break;
        case 'd': encode = false  ; break;
        case 'P': profiling = true; break;
        case 'v':
            printf("Version %u\n", version);
            exit(0);
        case 'h':
            usage();
            exit(0);
        default:
            croak("Ilagal args: use -h for help");
        }

    check_op(fil.length(), 'f');
    m_file = strdup(fil.c_str());

    m_info_filename = strdup(withsuffix(m_file, ".info"));

    m_wr_flags = overwrite ? "wb" : "wbx" ;

    if (encode) {
        f_usr = usr.length() ? fopen(usr.c_str(), "rb") : stdin ;
        check_fh(f_usr, usr, true);
        filename_stream.open(withsuffix(m_file, ".files"));
    }
    else {
        load_info();

        f_usr = usr.length() ? fopen(usr.c_str(), m_wr_flags) : stdout;
        check_fh(f_usr, usr);
    }
}

const char* Config::get_filename(const char* suffix) const {
    std::string str = m_file;
    str += m_part;
    str += ".";
    str += suffix ;
    if (encode)
        filename_stream << str << '\n';
    return strdup(str.c_str());
}

FILE* Config::open_w(const char* suffix) const {
    const char* name = get_filename(suffix);
    FILE* fh = fopen(name, m_wr_flags);
    check_fh(fh, name);

    delete[] name;
    return fh;
}

FILE* Config::open_r(const char* suffix, bool must) const {
    const char* name = get_filename(suffix);
    FILE* fh = fopen(name, "rb");
    if (must) check_fh(fh, name, true);

    delete [] name ;
    return fh;
}

void Config::set_part_offs(unsigned long long offs) const {
    sprintf(m_part, ".%010llx", offs);
}

Config::~Config() {
    save_info();

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
