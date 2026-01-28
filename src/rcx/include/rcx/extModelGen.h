// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstdio>
#include <list>
#include <string>

#include "odb/db.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

namespace rcx {

class extModelGen : public extRCModel
{
 public:
  extModelGen(uint32_t layerCnt, const char* name, utl::Logger* logger)
      : extRCModel(layerCnt, name, logger)
  {
  }
  uint32_t ReadRCDB(odb::dbBlock* block,
                    uint32_t widthCnt,
                    uint32_t diagOption,
                    char* logFile);
  void writeRules(FILE* fp, bool binary, uint32_t m, int corner = -1);
  FILE* InitWriteRules(const char* name,
                       std::list<std::string> corner_list,
                       const char* comment,
                       bool binary,
                       uint32_t modelCnt);
  static std::list<std::string> GetCornerNames(const char* filename,
                                               double& version,
                                               utl::Logger* logger);

  // ----------------------------------- DKF 09212024
  // ---------------------------------------
  uint32_t GenExtModel(const char* out_file,
                       const char* comment,
                       const char* version,
                       int pattern);
  // --------------------------------------------------------------------------------------------
};

}  // namespace rcx
