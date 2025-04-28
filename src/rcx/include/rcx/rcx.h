// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <array>

namespace rcx {

// CoupleOptions seriously needs to be rewriten to use a class with named
// members. -cherry 05/09/2021
using CoupleOptions = std::array<int, 21>;
using CoupleAndCompute = void (*)(CoupleOptions&, void*);

}  // namespace rcx
