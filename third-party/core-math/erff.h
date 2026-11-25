// Copyright (c) 2022-2025 Alexei Sibidanov.
// MIT License; see erff.c.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Correctly rounded erf(x) for binary32: the same result on every host,
// unlike libm's erff, whose last bit varies across glibc versions.
float cr_erff(float x);

#ifdef __cplusplus
}
#endif
