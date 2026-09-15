// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

namespace odb {
class dbDatabase;
}

namespace sta {
class dbSta;
}

namespace utl {
class Logger;
}

namespace syn {

class Graph;

void exportToOdb(Graph& g,
                 odb::dbDatabase* db,
                 sta::dbSta* sta,
                 utl::Logger* logger);

}  // namespace syn
