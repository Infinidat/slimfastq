

WALL      = -Wall -Werror
FLAGS     = $(WALL) -g -DDO_DEBUG -D_FILE_OFFSET_BITS=64 -std=c++0x
# PROF_DIR  = PROF <=  GCC Bug 47793: relative path turns into absolute
PROF_DIR  =
FLAGS_FAST= -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -msse3 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
FLAGS_OPT = $(WALL) $(PROF_DIR) $(FLAGS_FAST) -fprofile-use
FLAGS_PROF= $(WALL) $(PROF_DIR) $(FLAGS_FAST) -fprofile-generate
FLAGS_OPT2 = $(WALL) $(PROF_DIR) $(FLAGS_FAST) 

SAMPLE_DIR=samples
TEST_FILES = $(wildcard $(SAMPLE_DIR)/*.fq)
all:  slimfastq 
opt:  slimfastq.opt

prof-opt.run:
	for l in 1 2 3; do\
		for f in $(TEST_FILES) ; do \
			echo $$f ... ; \
			./slimfastq.prof -f /tmp/mytst -u $$f -O -P -l $$l ; \
			./slimfastq.prof -f /tmp/mytst -u /tmp/mytst.copy -O -d ; \
		done ; \
	done

prof-opt: slimfastq.prof
	make prof-opt.run
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
	rm slimfastq slimfastq.opt slimfastq.prof || true

# slimfastq
# SOURCES = slimfastq.cpp config.cpp fq_qlts.cpp pager.cpp fq_usrs.cpp
# HEADERS = common.hpp config.hpp fq_qlts.hpp pager.hpp fq_usrs.hpp

SOURCES= $(filter-out utest.cpp one.cpp molder.cpp, $(shell ls *.cpp))
HEADERS= $(shell echo *.hpp)
slimfastq: $(SOURCES) $(HEADERS)
	g++ $(FLAGS) -o $@ $(SOURCES)

.PHONY: slimfastq.prof slimfastq.opt slimfastq.run
run: slimfastq.run

slimfastq.run:
	g++ $(FLAGS_OPT2) -o $@ $(SOURCES)

slimfastq.opt:
	mv PROF/*.gcda . || true
	g++ $(FLAGS_OPT)  -o $@ $(SOURCES)
	mv *.gcda PROF || true
slimfastq.prof:
	g++ $(FLAGS_PROF) -o $@ $(SOURCES)

prof:
	g++ -O3 -fstrict-aliasing -ffast-math -pg -o slimfastq.prof $(SOURCES)

molder: molder.cpp pager.cpp pager.hpp
	g++ $(FLAGS) molder.cpp pager.cpp -o $@

tags:
	etags $(SOURCES) $(HEADERS)

UTSRC= $(filter-out one.cpp molder.cpp main.cpp, $(shell ls *.cpp))
UTHDR= $(shell ls *.hpp)
slimfastq.utest: $(UTSRC) $(UTHDR)
	g++ $(FLAGS) $(UTSRC) -o $@

utest: slimfastq.utest
	./slimfastq.utest

# small: all
# 	./slimfastq -f ../data/small.fq -u ../data/small.fq -O
# 	./slimfastq -f ../data/small.fq -u ../data/small.fq.tst -O	-d
# 	../data/mydiff.pl ../data/small.fq ../data/small.fq.tst

# time: prof-opt
# 	time ./slimfastq.opt -f ../data/t -u ../data/s.fastq -O
# 	time ./slimfastq.opt -f ../data/t -u ../data/s.fastq.tst -O -d
# 	cmp ../data/s.fastq ../data/s.fastq.tst

test: all
	for l in 1 2 3 4; do \
		for f in $(TEST_FILES) ; do \
			echo $$f $$l...   ; \
			rm /tmp/mytst.* || true; \
			./slimfastq -u $$f -f /tmp/mytst -O -l $$l && \
			./slimfastq -u /tmp/mytst.fastq -f /tmp/mytst -O -d && \
			tools/mydiff.pl $$f /tmp/mytst.fastq || break ; \
		done || break ; \
	done

tost: all
	for l in 1 ; do \
		for f in $(TEST_FILES) ; do \
			echo $$f $$l...   ; \
			rm /tmp/mytst.* || true; \
			./slimfastq -u $$f -f /tmp/mytst -O -l $$l && \
			./slimfastq -u /tmp/mytst.fastq -f /tmp/mytst -O -d && \
			tools/mydiff.pl $$f /tmp/mytst.fastq || break ; \
		done || break ; \
	done

FQ_FILE_250M=../1000/short.fastq
oprof:
	make run
	sudo rm -R oprofile_data/ || true
	sudo operf ./slimfastq.run -u $(FQ_FILE_250M) -f /tmp/r -O -1
	opreport -o /tmp/prof -s sample -g -l slimfastq.run
	head /tmp/prof

playground:
	@ echo $(filter-out molder.cpp, $(shell echo *.cpp))

