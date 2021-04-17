
#pragma once

#include <memory>
#include <map>

#include "opendb/db.h"

namespace pdn {

using odb::dbMaster;
using odb::dbNet;
using odb::dbBlock;
using odb::dbInst;
using odb::dbBox;
using odb::dbMTerm;

void
setSpecialITerms(dbNet *net);

void
globalConnect(dbBlock* block, const char* instPattern, const char* pinPattern, dbNet* net);

void
globalConnectRegion(dbBlock* block, const char* region, const char* instPattern, const char* pinPattern, dbNet* net);

void
globalConnectRegion(dbBlock* block, dbBox* region, const char* instPattern, const char* pinPattern, dbNet* net);

std::unique_ptr<std::vector<dbInst*>>
findInstsInArea(dbBlock* block, dbBox* region, const char* instPattern);

std::unique_ptr<std::map<dbMaster*, std::vector<dbMTerm*>>>
buildMasterPinMatchingMap(dbBlock* block, const char* pinPattern);

} // namespace
