# reverse\_ff\_fan\_control

\* WORK IN PROGRESS - ONLY TESTED SBC IS THE ROC\_RK3588S \*

\* NO ORIGINAL FIREFLY SOURCE CODE NOR BINARIES ARE INCLUDED \*

\* DON'T EVEN TRY TO SUE ME \*

This is my attempt at reconstructing the sourcecode from which firefly\_fan\_control was made.

My Reasons:

- TURN THE DARN ROARING FAN OFF!!!
- learn radare2
- retarget for alternative libc
- find bugs, resource leakage, inefficiency, dangerous code

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

Exposed info:

- non-static function names
- global and local-static variable names
- decently clean code constructs

Reconstruction methods:

- static analysis of aarch64 assembly
- comparison with my own compiler output
- decompiler output (for prototyping of functions)

Tools used:

- r2 and rizin (half of the things which are broken in r2 are not in rizin and the same goes the other way around)
- objdump (sparsely)
- readelf (sparsely)
- r2ghidra and rzghidra (sparsely)
- iaito and cutter (sparsely)
