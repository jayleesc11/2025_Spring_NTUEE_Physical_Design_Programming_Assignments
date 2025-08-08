This is README file for Physical Design for Nanometer ICs PA2
Author: <B11901029 Jhih-Jie Lee>
=====
SYNOPSIS:

bin/fp <alpha_value> <input.block_name> <input.net_name> <output_file_name>

This program supports floorplanning a set of hard macros within a rectangular outline without overlaps.
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

	bin/fp <alpha_value> <input.block_name> <input.net_name> <output_file_name>

	For example, under b11901029_pa2:
	bin/fp 0.5 inputs/ami33.block inputs/ami33.nets outputs/ami33.rpt