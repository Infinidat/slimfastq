slimfastq
=========

slimfastq would efficiently compresses/decompresses fastq files. It features:

* High compression ratio
* Relatively low CPU, memory usage
* Truly lossless compression/decompression
* Posix piping friendly (i.e. fastq input/output steam is serialzed during compression/decompression)

Usage
-----
<pre>

% slimfastq *file.fastq* *new-file.sfq*   : compress *file.fastq* to *new-file.sfq*
% slimfastq -1 *file.fastq* *new-file.sfq*: compress *file.fastq* to *new-file.sfq*, using little CPU/memory resources
                                           (-1 to -4 are levels of compression/resources trade-offs, -3 is default)

% slimfastq *file.sfq*                    : decompress *file.sfq* to stdout (format is determined by stamp, not name)
% slimfastq *file.sfq* *file.fastq        : decompress *file.sfq* to *file.fastq*

% slimfastq -h                            : get help

pipe usage:
% gzip -dc *file.fastq.gz* | slimfastq -f *file.sfq* : convert from gzip to sfq format (and save a lot of disk space)
% slimfastq *file.sfq* | md5sum -                    : get the checksum of the decompressed file without create a file

</pre>

The multi threads FAQ
---------------------
The main reason slimfastq is a single thread application is to avoid the overhead of semaphores, L2 flushes, and context
switches. The goal is to focus on the speed of N files compression/decompression instead of a single file.
...


Compile
-------
Simple compilation:
* run "make" or "make test"
(If the compilation fails, please let me know).

Profile optimized compilation:
* Compile slimfastq in a gcc profile generator mode (use 'make slimfastq.prof').
* Compress/decompress some of your fastq files - this will generate some *.gcda files.
* Recompile with optimization flags for the profiler generated data (use 'make slimfastq.opt').
(For whatever it's worth, the author of this page could not notice any significant performance change yield by the optimized compilation.)

Testing
-------
"make test" will compress, decompress and compare all the fastq files in the ./samples dir.
Some testing tips to check your own fastq files:
1) slimfastq is a lossless compression and posix pipes friendly, therefore it's easy to check the integrity of a large file with checksums:
% md5sum large-file.fq
% ./slimfastq largefile.fq -O /tmp/tst.sfq
% ./slimfastq /tmp/tst.sfq | md5sum -
(if md5sum don't match, one can use ./tools/mydiff.pl to the bad line. And I'll be grateful for a bug report)
2) use time. Example:
% /usr/bin/time -f  " IO : io=%I faults=%F\n MEM: max=%M kb Average=%K kb\n CPU: Percentage=%P real=%e sys=%S user=%U"  slimfastq large-file.fq /tmp/a.tst -O
3) Performance wise, a single file compression/decompression is not very interesting. Try to use the scripts tools/slimfastq.dir.compress and
tools/slimfastq.dir.decompress to check performance on a whole direcotry of fastq files. You can easily edit these scripts to adjust them to
other compression softwares (general or fastq specific).
4) Using to dir compression scripts, try to increase/decrease the max number of parallel process to as much as your system can handle efficiently. That would
set a ceiling for the maximum speed expectation.

Install
-------
After compile
* run "make install"
* Alternatively to "make install", copy the "slimfastq" executable and the "tools/slimfastq.*" scripts to any location.

License
-------
The BSD 3-Clause

Platform
--------
slimfastq was developed and optimized for x86_64 GNU/Linux and Darwin OS. For other system's support requests, please contact Josef Ezra.

Contact
-------
Josef Ezra  (jezra at infinidat.com), (jezra at cpan.org)


