// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace odb {
class dbBlock;
class dbNet;
};  // namespace odb

namespace utl {
class Logger;
}

namespace rcx {

///
/// Find a set of nets. Each name can be real name, or Nxxx, or xxx,
/// where xxx is the net oid.
///
bool findSomeNet(odb::dbBlock* block,
                 const char* names,
                 std::vector<odb::dbNet*>& nets,
                 utl::Logger* logger);

}  // namespace rcx
