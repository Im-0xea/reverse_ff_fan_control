# reverse\_ff\_fan\_control

My attempt at reversing the approximate C code from which firefly\_fan\_control was made.

This includes all bugs and horrible programming practice, which I ofc commented on extensively in the code.

\* This repo does not include any original source code from firefly, nor does it binaries.\*

\* The binaries can be found in glibc based distrubtions and a sketchy download on yandex. \*

The binary was not stripped and compiled with -O0 on gcc9 against glibc 2.17.

This gives me the ability to determine nearly the exact way it was written, making it a nice exercise, while also not too tedious.

Tools used:

- radare2
- nm
- xxd
- readelf
- filemagic
