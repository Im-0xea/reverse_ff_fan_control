# reverse\_ff\_fan\_control

My attempt at reversing the approximate C code from which firefly\_fan\_control was made.

This includes all bugs and horrible programming practice, which I ofc commented on extensively in the code.

\* This repo does not include any original source code from firefly, nor does it binaries.\*

\* The binaries can be found in glibc based distrubtions and a sketchy download on yandex. \*

About the binary: it was not stripped and compiled with -O0 on gcc-9.4.0-1.against glibc-2.17, on a Ubuntu-20.04.1 machine.

This gives me the ability to read names of non-static functions and global variables and see clean code constructs which are easy to translate to C

Tools used:

- radare2 / rizin
- cutter
