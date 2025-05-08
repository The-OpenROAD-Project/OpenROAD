// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}

namespace dst {
class Distributed;
}

namespace stt {
class SteinerTreeBuilder;
}

namespace drt {

class TritonRoute;

drt::TritonRoute* makeTritonRoute();

void deleteTritonRoute(drt::TritonRoute* router);

void initTritonRoute(drt::TritonRoute* router,
                     odb::dbDatabase* db,
                     utl::Logger* logger,
                     dst::Distributed* dist,
                     stt::SteinerTreeBuilder* stt_builder,
                     Tcl_Interp* tcl_interp);

}  // namespace drt
