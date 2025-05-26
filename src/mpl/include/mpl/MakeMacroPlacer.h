// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace sta {
class dbNetwork;
}

namespace odb {
class dbDatabase;
}

namespace sta {
class dbSta;
}

namespace utl {
class Logger;
}

namespace par {
class PartitionMgr;
}

namespace mpl {

class MacroPlacer;

mpl::MacroPlacer* makeMacroPlacer();

void initMacroPlacer(mpl::MacroPlacer* macro_placer,
                     sta::dbNetwork* network,
                     odb::dbDatabase* db,
                     sta::dbSta* sta,
                     utl::Logger* logger,
                     par::PartitionMgr* tritonpart,
                     Tcl_Interp* tcl_interp);

void deleteMacroPlacer(mpl::MacroPlacer* macro_placer);

}  // namespace mpl
