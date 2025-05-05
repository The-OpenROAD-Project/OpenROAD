// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

#include "odb/db.h"
#include "utl/Logger.h"

namespace sta {
class dbSta;
}

namespace stt {
class SteinerTreeBuilder;
}

namespace grt {
class GlobalRouter;
}

namespace dpl {
class Opendp;
}

namespace rsz {

class Resizer;

rsz::Resizer* makeResizer();

void deleteResizer(rsz::Resizer* resizer);

void initResizer(rsz::Resizer* resizer,
                 Tcl_Interp* tcl_interp,
                 utl::Logger* logger,
                 odb::dbDatabase* db,
                 sta::dbSta* sta,
                 stt::SteinerTreeBuilder* stt_builder,
                 grt::GlobalRouter* global_router,
                 dpl::Opendp* dp);

}  // namespace rsz
