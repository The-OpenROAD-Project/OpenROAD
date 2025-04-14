// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "util.h"

#include <memory>
#include <vector>

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
