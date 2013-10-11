

WALL      = -Wall -Werror
FLAGS     = $(WALL) -g -DDO_DEBUG  -std=c++0x
# PROF_DIR  = PROF <=  GCC Bug 47793: relative path turns into absolute
PROF_DIR  =
FLAGS_FAST= -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math
FLAGS_OPT = $(WALL) $(PROF_DIR) $(FLAGS_FAST) -fprofile-use
FLAGS_PROF= $(WALL) $(PROF_DIR) $(FLAGS_FAST) -fprofile-generate

SAMPLE_DIR=samples
TEST_FILES = $(wildcard $(SAMPLE_DIR)/*.fq)
all:  jfastq 
opt:  jfastq.opt 
prof-opt: jfastq.prof
	for f in $(TEST_FILES) ; do \
		echo $$f ... ; \
		./jfastq.prof -f /tmp/mytst -u $$f -O -P ; \
		./jfastq.prof -f /tmp/mytst -u /tmp/mytst.copy -O -d ; \
		done
	make opt
	make profclean

        # find . -name '*.gcda' -delete
profclean:
	mkdir -p PROF
	find PROF -name '*.gcda' -delete
	mv *.gcda PROF/ || true
	rm *.prof       || true

clean: profclean
	find . -name '*.o' -delete
	rm jfastq jfastq.opt jfastq.prof || true

# jfastq
# SOURCES = jfastq.cpp config.cpp fq_qlts.cpp pager.cpp fq_usrs.cpp
# HEADERS = common.hpp config.hpp fq_qlts.hpp pager.hpp fq_usrs.hpp

SOURCES= $(filter-out utest.cpp one.cpp molder.cpp, $(shell ls *.cpp))
HEADERS= $(shell echo *.hpp)
jfastq: $(SOURCES) $(HEADERS)
	g++ $(FLAGS) -o $@ $(SOURCES)

.PHONY: jfastq.prof jfastq.opt
jfastq.opt:
	mv PROF/*.gcda . || true
	g++ $(FLAGS_OPT)  -o $@ $(SOURCES)
	mv *.gcda PROF || true
jfastq.prof:
	g++ $(FLAGS_PROF) -o $@ $(SOURCES)

prof:
	g++ -O3 -fstrict-aliasing -ffast-math -pg -o jfastq.prof $(SOURCES)

molder: molder.cpp pager.cpp pager.hpp
	g++ $(FLAGS) molder.cpp pager.cpp -o $@

tags:
	etags $(SOURCES) $(HEADERS)

UTSRC= $(filter-out one.cpp molder.cpp jfastq.cpp, $(shell ls *.cpp))
UTHDR= $(shell ls *.hpp)
jfastq.utest: $(UTSRC) $(UTHDR)
	g++ $(FLAGS) $(UTSRC) -o $@

utest: jfastq.utest
	./jfastq.utest

# small: all
# 	./jfastq -f ../data/small.fq -u ../data/small.fq -O
# 	./jfastq -f ../data/small.fq -u ../data/small.fq.tst -O	-d
# 	../data/mydiff.pl ../data/small.fq ../data/small.fq.tst

# time: prof-opt
# 	time ./jfastq.opt -f ../data/t -u ../data/s.fastq -O
# 	time ./jfastq.opt -f ../data/t -u ../data/s.fastq.tst -O -d
# 	cmp ../data/s.fastq ../data/s.fastq.tst

test: all
	for f in $(TEST_FILES) ; do \
		echo $$f ...   ; \
		rm /tmp/mytst.* || true; \
		./jfastq -u $$f -f /tmp/mytst -O ; \
		./jfastq -u /tmp/mytst.fastq -f /tmp/mytst -O -d ; \
		tools/mydiff.pl $$f /tmp/mytst.fastq ; \
	done

playground:
	@ echo $(filter-out molder.cpp, $(shell echo *.cpp))


ONESRC=one.cpp
one: $(ONESRC)
	g++ $(FLAGS) -o $@ $<

COFLAGS  = -O2 -g -fomit-frame-pointer -fstrict-aliasing -ffast-math -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -Wall -msse2
one.opt:
	g++ $(COFLAGS) -o $@ $(ONESRC)
# 	mv PROF/*.gcda . || true
# 	g++ $(FLAGS_OPT)  -o $@ $(ONESRC)
# 	mv *.gcda PROF || true
# 
# one.prof:
# 	g++ $(FLAGS_PROF) -o $@ $(ONESRC)
# 
# one-prof-opt: one.prof
# 	for f in $(TEST_FILES) ; do \
# 		echo $$f ... ; \
# 		./one.prof $$f /tmp/mytst ; \
# 		./one.prof -d  /tmp/mytst /tmp/mytst.copy ; \
# 		done
# 	make one.opt
# 	make profclean


