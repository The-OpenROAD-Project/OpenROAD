// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ClockTree.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "odb/db.h"

namespace wmk {

using odb::dbBlock;
using odb::dbInst;
using odb::dbITerm;
using odb::dbNet;

odb::dbNet* singleOutputNet(dbInst* inst)
{
  dbNet* out = nullptr;
  int count = 0;
  for (dbITerm* iterm : inst->getITerms()) {
    if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
      if (++count > 1) {
        break;
      }
      out = iterm->getNet();
    }
  }
  return count == 1 ? out : nullptr;
}

bool isSequentialClockSink(dbITerm* iterm)
{
  if (iterm->getIoType() != odb::dbIoType::INPUT) {
    return false;
  }
  odb::dbMTerm* mterm = iterm->getMTerm();
  if (mterm == nullptr) {
    return false;
  }
  if (mterm->getSigType() == odb::dbSigType::CLOCK) {
    return true;
  }
  dbInst* inst = iterm->getInst();
  if (inst == nullptr || !inst->getMaster()->isSequential()) {
    return false;
  }
  std::string name = mterm->getName();
  std::ranges::transform(name, name.begin(), [](unsigned char c) {
    return static_cast<char>(std::toupper(c));
  });
  return name == "CP" || name == "CLK" || name == "CK" || name == "CLOCK";
}

int seqFanout(dbInst* lcb)
{
  dbNet* net = singleOutputNet(lcb);
  if (net == nullptr) {
    return 0;
  }
  int count = 0;
  for (dbITerm* iterm : net->getITerms()) {
    if (isSequentialClockSink(iterm)) {
      ++count;
    }
  }
  return count;
}

std::vector<dbInst*> findLeafClockBuffers(dbBlock* block)
{
  std::vector<dbInst*> lcbs;
  for (dbInst* inst : block->getInsts()) {
    dbNet* out = singleOutputNet(inst);
    if (out == nullptr || out->getSigType() != odb::dbSigType::CLOCK) {
      continue;
    }
    if (seqFanout(inst) > 0) {
      lcbs.push_back(inst);
    }
  }
  // Name order, so the pairing below does not depend on database order.
  std::ranges::sort(
      lcbs, [](dbInst* a, dbInst* b) { return a->getName() < b->getName(); });
  return lcbs;
}

}  // namespace wmk
