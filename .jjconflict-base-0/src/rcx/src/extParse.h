// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "parse.h"
#include "utl/Logger.h"

namespace rcx {

class extParser : public Ath__parser
{
 public:
  // Constructor
  extParser(utl::Logger* logger) : Ath__parser(logger) {}
  void setDbg(int v) { _dbg = v; }

 private:
  int _dbg;
};

}  // namespace rcx
