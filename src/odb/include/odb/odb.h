// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cassert>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>

#include "utl/Logger.h"

namespace odb {

using uint = unsigned int;
using uchar = unsigned char;

#ifndef SWIG
using utl::format_as;
#endif

class dbITerm;
class dbModBTerm;
using ITMap = std::map<dbITerm*, dbITerm*>;
using modBTMap = std::map<dbModBTerm*, dbModBTerm*>;

}  // namespace odb
