// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace odb {
class dbDatabase;
}

namespace sta {
class dbNetwork;
}

namespace sta {
class dbSta;
}

namespace stt {
class SteinerTreeBuilder;
}

namespace rsz {
class Resizer;
}

namespace utl {
class Logger;
}

namespace cts {
class TritonCTS;

cts::TritonCTS* makeTritonCts();

void initTritonCts(cts::TritonCTS* cts,
                   odb::dbDatabase* db,
                   sta::dbNetwork* network,
                   sta::dbSta* sta,
                   stt::SteinerTreeBuilder* stt_builder,
                   rsz::Resizer* resizer,
                   utl::Logger* logger,
                   Tcl_Interp* tcl_interp);

void deleteTritonCts(cts::TritonCTS* tritoncts);

}  // namespace cts
