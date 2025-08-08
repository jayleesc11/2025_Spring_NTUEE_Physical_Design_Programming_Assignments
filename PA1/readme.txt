This is README file for Physical Design for Nanometer ICs PA1
Author: <B11901029 Jhih-Jie Lee>
=====
SYNOPSIS:

bin/fm <input_file_name> <output_file_name>

This program supports partitioning a set of cells into two disjoint, balanced groups, while minimizing cut size.
=====
DIRECTORY:

src/ 	        source C++ codes
bin/	        executable binary
Makefile        makefile
readme.txt      this file
report.pdf      report
======
HOW TO COMPILE:

To compile the demo, simply follow the following steps

	make
======
HOW TO RUN:

	bin/fm <input_file_name> <output_file_name>

	For example, under b11901029_pa1:
	bin/fm inputs/input_0.dat outputs/output_0.dat