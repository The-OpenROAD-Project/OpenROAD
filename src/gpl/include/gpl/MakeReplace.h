// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include "tcl.h"
namespace utl {
class Logger;
}  // namespace utl

namespace gpl {

class Replace;

void initReplace(Tcl_Interp* tcl_interp);
void initReplaceGraphics(Replace* replace, utl::Logger* log);

}  // namespace gpl
