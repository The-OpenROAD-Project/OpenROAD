// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace ppl {
class IOPlacer;
}

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}

namespace ord {

ppl::IOPlacer* makeIoplacer();

void initIoplacer(ppl::IOPlacer* placer,
                  odb::dbDatabase* db,
                  utl::Logger* logger,
                  Tcl_Interp* tcl_interp);

void deleteIoplacer(ppl::IOPlacer* ioplacer);

}  // namespace ord
