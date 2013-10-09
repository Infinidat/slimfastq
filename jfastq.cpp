//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
// #include <assert.h>

static const int version = 3;   // inernal version

#include "common.hpp"
#include "config.hpp"
#include "fq_usrs.hpp"

// static int get_llen(const Config* conf) {
//     int llen = atoi(conf->get_info("llen"));
//     if (llen > MAX_GN_LLEN or 
//         llen < 50 )
//         croak("illegal line length: %ld\n", llen);
// 
//     return llen;
// }


// static int encode(const Config* conf) {
// 
//     // UINT32  sanity = conf->profiling ? 100000 : 1000000000;
//     // 
//     UsrSave usr(conf);
//     return  usr.encode();
    // RecSave rec(conf);
    // GenSave gen(conf);
    // QltSave qlt(conf);
    // 
    // UCHAR *p_rec, *p_rec_end, *p_gen, *p_qlt;
    // int llen = 0;
    // size_t n_recs = 0;
    // 
    // bool gentype = 0;
    // while(usr.get_record(&p_rec, &p_rec_end, &p_gen, &p_qlt) and
    //       ++ n_recs < sanity) {
    // 
    //     rarely_if(not llen)
    //         llen = get_llen(conf);
    // 
    //     gen.save(p_gen, p_qlt, llen, &gentype);
    //     rec.save(p_rec, p_rec_end, gentype);
    //     qlt.save(p_qlt, llen);
    // }
    // conf->set_info("num_records", n_recs);
    // return 0;
// }
// 
// static int decode(const Config* conf) {
//     return UsrLoad (conf) . decode();
    // return  usr.decode();
    // RecLoad rec(conf);
    // GenLoad gen(conf);
    // QltLoad qlt(conf);
    // 
    // UCHAR* buf_qlt = usr.buf_qlt();
    // UCHAR* buf_gen = usr.buf_gen();
    // UCHAR* buf_rec = usr.buf_rec();
    // UINT32* size_rec = usr.size_rec();
    // 
    // UINT16 llen = get_llen(conf);
    // 
    // bool gentype = false;
    // size_t n_recs = atoi(conf->get_info("num_records"));
    // 
    // while (n_recs --) {
    //     *size_rec = rec.load(buf_rec, &gentype);
    //     if (not *size_rec)
    //         croak("premature EOF - %llu records left", n_recs+1);
    //     qlt.load(buf_qlt, llen);
    //     gen.load(buf_gen, buf_qlt, llen);
    //     usr.save();
    // }
    // sanity: verify all objects are done (by croak?)
    // return 0;
// }

int main(int argc, char** argv) {

    Config conf (argc, argv, version);
    return
        conf.encode ?
        UsrSave (&conf) . encode() :
        UsrLoad (&conf) . decode() ;
}
