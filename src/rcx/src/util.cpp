///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2025, Precision Innovations Inc.
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

#include "util.h"

#include <memory>

#include "odb/db.h"
#include "parse.h"

namespace rcx {

bool findSomeNet(odb::dbBlock* block,
                 const char* names,
                 std::vector<odb::dbNet*>& nets,
                 utl::Logger* logger)
{
  if (!names || names[0] == '\0') {
    return false;
  }
  auto parser = std::make_unique<Ath__parser>(logger);
  parser->mkWords(names, nullptr);
  for (int ii = 0; ii < parser->getWordCnt(); ii++) {
    char* netName = parser->get(ii);
    odb::dbNet* net = block->findNet(netName);
    if (!net) {
      uint noid = netName[0] == 'N' ? atoi(&netName[1]) : atoi(&netName[0]);
      net = odb::dbNet::getValidNet(block, noid);
    }
    if (net) {
      nets.push_back(net);
    } else {
      logger->warn(utl::RCX, 46, "Can not find net {}", netName);
    }
  }
  return !nets.empty();
}

}  // namespace rcx
