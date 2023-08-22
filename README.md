# reverse\_ff\_fan\_control

\* WORK IN PROGRESS - ONLY TESTED SBC IS THE ROC\_RK3588S \*

\* NO ORIGINAL FIREFLY SOURCE CODE NOR BINARIES ARE INCLUDED \*

\* PLEASE DON'T SUE ME! \*

This is my attempt at reconstructing the approximate C code from which firefly\_fan\_control was made.

First I just wanted to stare at strace output till I knew how to turn my roaring fan off, but since I learned about r2 exactly at this time, I quickly decided to go for full reversal.

The purpose of this first was just to make it retargetable for other libc's and learning the in and outs of r2.

But as I quickly realized, the primary purpose of this is to reconstruct awful proprietary code and laugh at it.

Feel free to enjoy commented issues in the code, often regarding basic good practice, resource leakage and waste, and decisions which might be dangerous to hardware.

About the binary: It was not stripped and compiled without optimizations on gcc-9.4.0-1.against glibc-2.17 on a Ubuntu-20.04.1 machine, in multiple compilation units, compiled seperately, than linked ofc without lto.

This allows me to read names of non-static functions, global variables(aswell as some "local" static variables) and see decently clean code constructs.

I might not always get everything right, but Im doing my best to get as close to original as possible.

Most of this is made through static analysis of aarch64 assembly and comparison with reconstruction compiler output.

Tools used:

- r2 (rizin, half of the things which are broken in r2 are not in rizin and the other way around)
- objdump (sparsely)
- readelf (sparsely)
- r2ghidra (rzghidra) (sparsely)
- iaito (cutter) (sparsely)
