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

namespace pdn {

class PdnGen;

void initPdnGen(pdn::PdnGen* pdngen,
                odb::dbDatabase* db,
                utl::Logger* logger,
                Tcl_Interp* tcl_interp);

pdn::PdnGen* makePdnGen();

void deletePdnGen(pdn::PdnGen* pdngen);

}  // namespace pdn
