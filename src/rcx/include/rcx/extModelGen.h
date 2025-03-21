///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

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
