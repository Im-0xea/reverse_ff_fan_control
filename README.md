# reverse\_ff\_fan\_control

\* WORK IN PROGRESS - ONLY TESTED SBC IS THE ROC\_RK3588S \*

\* NO ORIGINAL FIREFLY SOURCE CODE NOR BINARIES ARE INCLUDED \*

\* DON'T EVEN TRY TO SUE ME \*

This is my attempt at reconstructing the approximate sourcecode for firefly\_fan\_control.

My Reasons:

- TURN THE DARN ROARING FAN OFF!!!
- learn radare2
- retarget for alternative libc
- find bugs, exploits, resource leakage, inefficiency, dangerous code

Reconstruction methods:

- static analysis of aarch64 assembly
- comparison with my own compiler output
- decompiler output (for prototyping of functions)

Tools used:

- radare2/rizin (half of the things which are broken in r2 are not in rizin and the same goes the other way around)
- objdump (for cases where radare2/rizin are broken)
- r2ghidra/rzghidra (prototyping)

About the binary:

- Ubuntu-20.04.1 system
- C programming language
- AARCH64
- GCC-9.4.0-1
- GLIBC-2.17
- No optimizations
- Several Compilation units
- No LTO nor combined compilation
- No stripping
- included build ID -> "ae294f03589848da41fb42044a26b84286f39842"
- potiential source -> https://github.com/armbian/build/tree/main/packages/blobs/station

Exposed info:

- non-static function names
- global and local-static variable names
- decently clean code constructs

Issues found:

- buffer overflows through non-sensical 1000 bytes float parsing into buffers which are not even used
- CS\_R1\_3399JD4 makes this overflow and injecting invalid temp, accessible on /tmp/fan\_temperature
- defaulting to 50C when reading from temperature sensors fails
- missing fail checks
- no termination for undefined boards and undefined RK3588S versions(the latter causes zeroing of fan)
- popen() echo > / cat, for reading and writing from files
- popen() closed with fclose() instead of pclose(), or not closed at all
- not breaking on failed open()
- messy and incorrect logging
- childish static argument parsing
- usage has ./main as program name and doesn't explain anything only shows some possible calls
- usage example calls have incorrect board names for CS_R\*\_3399JD4
- constant reinit of an fd\_set
- looping usleep several times instead of making the value bigger
- duplicate cases everywhere
- including -MAIN into board name
- redudant pthreads
- pthreads are not necassary at all
- pthread error messages with non-sensical numbering in them
- unused functions
- unused variables
