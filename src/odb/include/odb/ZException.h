// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb.h"

namespace odb {

// See http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert
// Avoids unused variable warnings.
#ifdef NDEBUG
#define ZASSERT(x)    \
  do {                \
    (void) sizeof(x); \
  } while (0)
#else
#define ZASSERT(x) assert(x)
#endif

}  // namespace odb
