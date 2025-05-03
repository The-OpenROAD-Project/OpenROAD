// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace rcx {
class Ext;
}  // namespace rcx

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}

namespace ord {

rcx::Ext* makeOpenRCX();

void deleteOpenRCX(rcx::Ext* extractor);

void initOpenRCX(rcx::Ext* extractor,
                 odb::dbDatabase* db,
                 utl::Logger* logger,
                 const char* spef_version,
                 Tcl_Interp* tcl_interp);

}  // namespace ord
