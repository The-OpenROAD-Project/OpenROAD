// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace dst {
class Distributed;
}

namespace utl {
class Logger;
}

namespace ord {

dst::Distributed* makeDistributed();

void deleteDistributed(dst::Distributed* dstr);

void initDistributed(dst::Distributed* distributer,
                     utl::Logger* logger,
                     Tcl_Interp* tcl_interp);

}  // namespace ord
