// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include "utl/Logger.h"

namespace utl {

// Redirects stdout to /dev/null while in scope
class SuppressStdout
{
 public:
  SuppressStdout(Logger* logger);
  ~SuppressStdout();

  // Don't copy or move
  SuppressStdout(SuppressStdout const&) = delete;
  SuppressStdout(SuppressStdout&&) = delete;
  SuppressStdout& operator=(SuppressStdout const&) = delete;
  SuppressStdout& operator=(SuppressStdout&&) = delete;

 private:
  int saved_fd_ = -1;
  Logger* logger_ = nullptr;
};

}  // namespace utl
