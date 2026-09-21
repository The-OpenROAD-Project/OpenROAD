// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#pragma once

namespace utl {
class Logger;
}

namespace wmk {
struct PlacementOptions;
struct CtsOptions;

// Validate before changing the database, parasitics or output files.
void validateOptions(const PlacementOptions& opts,
                     int dbu,
                     utl::Logger* logger);
void validateOptions(const CtsOptions& opts, int dbu, utl::Logger* logger);
}  // namespace wmk
