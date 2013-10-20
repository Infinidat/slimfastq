//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

static const int version = 3;   // inernal version

#include "common.hpp"
#include "config.hpp"
#include "usrs.hpp"

Config conf;
int main(int argc, char** argv) {

    conf.init(argc, argv, version);
    return
        conf.encode ?
        UsrSave () . encode() :
        UsrLoad () . decode() ;
}
