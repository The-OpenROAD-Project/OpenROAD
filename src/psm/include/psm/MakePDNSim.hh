// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
}

namespace sta {
class dbSta;
}

namespace est {
class EstimateParasitics;
}

namespace dpl {
class Opendp;
}

namespace psm {

class PDNSim;

psm::PDNSim* makePDNSim();

void initPDNSim(psm::PDNSim* pdnsim,
                utl::Logger* logger,
                odb::dbDatabase* db,
                sta::dbSta* sta,
                est::EstimateParasitics* estimate_parasitics,
                dpl::Opendp* opendp,
                Tcl_Interp* tcl_interp);

void deletePDNSim(psm::PDNSim* pdnsim);

}  // namespace psm
