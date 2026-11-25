# CORE-MATH (vendored)

Source: <https://gitlab.inria.fr/core-math/core-math>, file
`src/binary32/erf/erff.c` at commit
`567e1efc10401e672029eeab01a295471324ad3a` (the file last changed in
`8ea8ea35c518d06b72de4cf1d528e7a2d16f104c`).
License: MIT (in the header of `erff.c`).

`cr_erff` is a correctly rounded single-precision error function. It uses
only double-precision arithmetic and exact builtins, so it returns the
same bits on every host. libm's `erff` does not: glibc 2.39 is off by one
ulp on about 2% of inputs, while glibc 2.43 is correctly rounded, so a
libm-based `erff` makes placement depend on the glibc the binary runs on.

`erff.c` is unmodified; `erff.h` declares `cr_erff`.
