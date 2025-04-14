// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "extRCap.h"

namespace rcx {

class extModelGen : public extRCModel
{
 public:
  extModelGen(uint layerCnt, const char* name, Logger* logger)
      : extRCModel(layerCnt, name, logger)
  {
  }
  uint ReadRCDB(dbBlock* block, uint widthCnt, uint diagOption, char* logFile);
  void writeRules(FILE* fp, bool binary, uint m, int corner = -1);
  FILE* InitWriteRules(const char* name,
                       std::list<std::string> corner_list,
                       const char* comment,
                       bool binary,
                       uint modelCnt);
  static std::list<std::string> GetCornerNames(const char* filename,
                                               double& version,
                                               Logger* logger);

  // ----------------------------------- DKF 09212024
  // ---------------------------------------
  uint GenExtModel(const char* out_file,
                   const char* comment,
                   const char* version,
                   int pattern);
  // --------------------------------------------------------------------------------------------
};

}  // namespace rcx
