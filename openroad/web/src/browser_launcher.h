// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace web {

// Runs `argv`, the browser launcher and then its arguments, with no shell,
// stderr to `stderr_path`, and in a session of its own off macOS.  One still
// running after `wait` counts as launched; "" on success, else why not.
std::string runBrowserLauncher(std::vector<std::string> argv,
                               const std::string& stderr_path,
                               std::chrono::milliseconds wait
                               = std::chrono::seconds(5));

}  // namespace web
