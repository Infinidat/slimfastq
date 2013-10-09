//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 


#ifndef FQ_CONFIG_H
#define FQ_CONFIG_H

#include <stdio.h>
class Config;


// NOTE: soon there would be new machines, providing longer lines. 
#define MAX_REC_LEN  1000
#define MAX_ID_LLEN  250
#define MAX_GN_LLEN  250
#define MIN_GN_LLEN   50

void croak(const char* msg               ) __attribute__ ((noreturn, cold));
void croak(const char* fmt, long long num) __attribute__ ((noreturn, cold));

class Config {                  
    // Singleton - croaks if already exists
public:
    Config(int argc, char **argv, int ver);
    ~Config();
    FILE * file_usr() const { return reinterpret_cast<FILE*>(f_usr);}
    FILE* open_w(const char* suffix) const;
    FILE* open_r(const char* suffix, bool must=true) const;
    int   version;
    bool  encode, profiling;
    struct {
        long long size; // NOTE: this means different things during decode, encode
        long long param;
    } partition;


    void load_info() const;
    void save_info() const;
    void set_info(const char* key, const char* val) const;
    void set_info(const char* key, long long num) const;
    void set_part_offs(unsigned long long offs) const;
    //    bool has_info(const char* key) const ;
    const char* get_info(const char* key) const;
    bool        has_info(const char* key) const;
    bool        get_bool(const char* key) const;
    long long   get_long(const char* key, long long val=0) const;
private:
    void usage() const;
    const char* get_filename(const char* suffix) const;
    // void set_partition(const char* str);
    mutable bool m_saved;

    FILE  *f_usr;
    const char* m_info_filename;
    const char* m_file;
    const char* m_wr_flags;
    mutable char m_part[100];
};


#endif  // FQ_CONFIG_H
