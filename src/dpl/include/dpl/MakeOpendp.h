// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace dpl {
class Opendp;
}

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}

namespace ord {

dpl::Opendp* makeOpendp();
void initOpendp(dpl::Opendp* dpl,
                odb::dbDatabase* db,
                utl::Logger* logger,
                Tcl_Interp* tcl_interp);
void deleteOpendp(dpl::Opendp* opendp);

}  // namespace ord
