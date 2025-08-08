This is README file for Physical Design for Nanometer ICs PA3
Author: <B11901029 Jhih-Jie Lee>
=====
SYNOPSIS:

bin/place -aux <input_name.aux>

This program supports placing a set of cells to the desired positions on the chip.
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

During compilation, the linker may produce a warning related to OpenMP and dlopen, such as:
/usr/bin/ld: ... warning: Using 'dlopen' in statically linked applications requires at runtime the shared libraries from the glibc version used for linking
This warning is triggered by OpenMP's use of dynamic loading in static linking contexts.
It does not affect the correctness or runtime behavior of our program, and the application can still run as expected.
======
HOW TO RUN:

	bin/place -aux <input_name.aux>

	For example, under b11901029_pa3:
	bin/place -aux benchmark/ibm01/ibm01-cu85.aux