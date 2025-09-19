// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace utl {
class Logger;
}

namespace odb {

class dbBlock;

void orderWires(utl::Logger* logger, dbBlock* b);

}  // namespace odb
