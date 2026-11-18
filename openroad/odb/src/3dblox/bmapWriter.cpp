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
    for (auto bump : region->getChipBumps()) {
      std::string line;
      const auto inst_name = bump->getInst()->getName();
      const auto cell_type = bump->getInst()->getMaster()->getName();
      auto point = bump->getInst()->getOrigin();
      line.append(inst_name);
      line.append(" ");
      line.append(cell_type);
      line.append(" ");
      line.append(std::to_string(point.getX() / u));
      line.append(" ");
      line.append(std::to_string(point.getY() / u));
      line.append(" ");
      if (bump->getBTerm() != nullptr) {
        line.append(bump->getBTerm()->getName());
      } else {
        line.append("-");
      }
      line.append(" ");
      if (bump->getNet() != nullptr) {
        line.append(bump->getNet()->getName());
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