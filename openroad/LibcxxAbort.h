// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>

namespace utl {
class Logger;
}

namespace ord {

[[noreturn]] void logLibcxxFailure(utl::Logger& logger,
                                   const std::string& trace,
                                   const std::string& message);

}  // namespace ord
