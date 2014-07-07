slimfastq
=========

slimfastq is a cli application that compresses/decompresses fastq files.
It features:

* High compression ratio
* Relatively low cpu, memory usage
* Truly lossless compression/decompression


Usage
-----

<pre>

% slimfastq *file.fastq* *new-file.sfq*   : compress *file.fastq* to *new-file.sfq* 
% slimfastq -1 *file.fastq* *new-file.sfq*: compress *file.fastq* to *new-file.sfq*, using little cpu/memory resources
                                           (-1 to -4 are levels of compression/resources tradeoffs, -3 is default)

% slimfastq *file.sfq*                    : decompress *file.sfq* to stdout (format is determined by stamp, not name)
% slimfastq *file.sfq* *file.fastq        : decompress *file.sfq* to *file.fastq*

% slimfastq -h                            : get help

% gzip -dc *file.fastq.gz* | slimfastq -f *file.sfq* : convert from gzip to sfq format (and save a lot of disk space)

</pre>


Install
-------
Compile via 'make', copy the executable "slimfastq" to any location.
(If the compilation fails, please let me know)

License
-------
The BSD 3-Clause

Platform
--------
slimfastq was developed and optimized for x86_64 GNU/Linux. For other system's support requests, please contact Josef Ezra. 

Contact
-------
Josef Ezra  (jezra at infinidat.com), (jezra at cpan.org)


