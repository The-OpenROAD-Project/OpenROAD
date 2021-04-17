
#include "pdn/PdnGen.hh"

#include "openroad/OpenRoad.hh"
#include "utl/Logger.h"

#include <regex>

namespace pdn {

using utl::Logger;
using utl::IFP;

using odb::dbBlock;
using odb::dbInst;
using odb::dbITerm;
using odb::dbMTerm;
using odb::dbMaster;
using odb::dbNet;
using odb::dbBox;

using std::regex;
using std::cmatch;

void
setSpecialITerms(dbNet *net) {
  for (dbITerm* iterm : net->getITerms()) {
    iterm->setSpecial();
  }
}

void
globalConnect(dbBlock* block, const char* instPattern, const char* pinPattern, dbNet *net) {
  dbBox* bbox = nullptr;
  globalConnectRegion(block, bbox, instPattern, pinPattern, net);
}

void
globalConnectRegion(dbBlock* block, const char* regionName, const char* instPattern, const char* pinPattern, dbNet *net) {
  for (dbBox* box : block->findRegion(regionName)->getBoundaries()) {
    globalConnectRegion(block, box, instPattern, pinPattern, net);
  }
}

void
globalConnectRegion(dbBlock* block, dbBox* region, const char* instPattern, const char* pinPattern, dbNet *net) {
  const regex pinRegex(pinPattern);

  std::unique_ptr<std::vector<dbInst*>> insts = findInstsInArea(block, region, instPattern);
  std::unique_ptr<std::map<dbMaster*, std::vector<dbMTerm*>>> masterpins = buildMasterPinMatchingMap(block, pinPattern);

  cmatch match;
  for (dbInst* inst : *insts) {
    dbMaster* master = inst->getMaster();

    auto masterpin = masterpins->find(master);
    if (masterpin != masterpins->end()) {
      for (dbMTerm* mterm : masterpin->second) {
        dbITerm::connect(inst, net, mterm);
      }
    }
  }
}

std::unique_ptr<std::vector<dbInst*>>
findInstsInArea(dbBlock* block, dbBox* region, const char* instPattern) {
  std::unique_ptr<std::vector<dbInst*>> insts = std::make_unique<std::vector<dbInst*>>();
  const regex instRegex(instPattern);

  cmatch match;
  for (dbInst* inst : block->getInsts()) {
    if (std::regex_match(inst->getName().c_str(), match, instRegex)) {
      if (region == nullptr) {
        insts->push_back(inst);
      } else {
        dbBox* box = inst->getBBox();

        if (region->yMin() <= box->yMin() &&
            region->xMin() <= box->xMin() &&
            region->xMax() >= box->xMax() &&
            region->yMax() >= box->yMax()) {
          insts->push_back(inst);
        }
      }
    }
  }

  return insts;
}

std::unique_ptr<std::map<dbMaster*, std::vector<dbMTerm*>>>
buildMasterPinMatchingMap(dbBlock* block, const char* pinPattern) {
  const regex pinRegex(pinPattern);

  std::unique_ptr<std::map<dbMaster*, std::vector<dbMTerm*>>> masterpins = std::make_unique<std::map<dbMaster*, std::vector<dbMTerm*>>>();

  std::vector<dbMaster*> masters;
  block->getMasters(masters);

  cmatch match;
  for (dbMaster* master : masters) {
    masterpins->emplace(master, std::vector<dbMTerm*>());

    for (dbMTerm* mterm : master->getMTerms()) {
      if (std::regex_match(mterm->getName().c_str(), match, pinRegex)) {
        masterpins->at(master).push_back(mterm);
      }
    }
  }

  return masterpins;
}

} // namespace
