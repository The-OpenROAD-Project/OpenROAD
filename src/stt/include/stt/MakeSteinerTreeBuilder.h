// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}

namespace stt {
class SteinerTreeBuilder;

stt::SteinerTreeBuilder* makeSteinerTreeBuilder();

void initSteinerTreeBuilder(stt::SteinerTreeBuilder* stt_builder,
                            odb::dbDatabase* db,
                            utl::Logger* logger,
                            Tcl_Interp* tcl_interp);

void deleteSteinerTreeBuilder(stt::SteinerTreeBuilder* stt_builder);

}  // namespace stt
