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

namespace utl {
class Logger;
}

namespace par {

class PartitionMgr;

par::PartitionMgr* makePartitionMgr();

void initPartitionMgr(par::PartitionMgr* partitioner,
                      odb::dbDatabase* db,
                      sta::dbNetwork* db_network,
                      sta::dbSta* sta,
                      utl::Logger* logger,
                      Tcl_Interp* tcl_interp);

void deletePartitionMgr(par::PartitionMgr* partitionmgr);

}  // namespace par
