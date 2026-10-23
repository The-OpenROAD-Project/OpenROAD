// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "bmapWriter.h"

#include <fstream>
#include <string>

#include "odb/db.h"
#include "utl/Logger.h"

namespace odb {

BmapWriter::BmapWriter(utl::Logger* logger) : logger_(logger)
{
}

void BmapWriter::writeFile(const std::string& filename,
                           odb::dbChipRegion* region)
{
  const auto u = region->getDb()->getDbuPerMicron();
  std::ofstream bmap_file(filename);
  if (bmap_file.is_open()) {
    for (odb::dbInst* inst : region->getBumps()) {
      std::string line;
      const auto inst_name = inst->getName();
      const auto cell_type = inst->getMaster()->getName();
      auto point = inst->getOrigin();
      odb::dbBTerm* bterm = inst->getBumpBTerm();
      odb::dbNet* net = inst->getITerm(0U)->getNet();
      line.append(inst_name);
      line.append(" ");
      line.append(cell_type);
      line.append(" ");
      line.append(std::to_string(point.getX() / u));
      line.append(" ");
      line.append(std::to_string(point.getY() / u));
      line.append(" ");
      if (bterm != nullptr) {
        line.append(bterm->getName());
      } else {
        line.append("-");
      }
      line.append(" ");
      if (net != nullptr) {
        line.append(net->getName());
      } else {
        line.append("-");
      }
      line.append("\n");
      bmap_file << line;
    }
    bmap_file.close();
  } else {
    logError("Unable to open file");
  }
}

void BmapWriter::logError(const std::string& message)
{
  logger_->error(utl::ODB, 562, "Write Error: {}", message);
}

}  // namespace odb