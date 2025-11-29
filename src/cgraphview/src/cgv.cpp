// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "cgv/cgv.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "observer.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace cgv {

// These need to be defined in the cpp due to the smart pointer in
// example.h to Observer which is only declared there.
CGV::CGV(odb::dbDatabase* db, utl::Logger* logger) : db_(db), logger_(logger)
{
}

CGV::~CGV() = default;

enum class csv
{
  iterm1 = 0,
  iterm2 = 1,
  apA_x = 2,
  apA_y = 3,
  apB_x = 4,
  apB_y = 5,
  cost = 6
};

// The example operation.
void CGV::displayCSV(const char* name)
{
  odb::dbChip* chip = db_->getChip();
  if (!chip) {
    logger_->error(utl::EXA, 2, "No chip exists.");
  }

  odb::dbBlock* block = chip->getBlock();
  if (!block) {
    logger_->error(utl::EXA, 3, "No block exists.");
  }

  std::vector<std::pair<odb::Point, odb::Point>> edges;

  std::ifstream file(name);
  std::string line;
  bool firstLine = true;
  while (std::getline(file, line)) {
    if (firstLine) {
      firstLine = false;
      continue;
    }

    std::stringstream ss(line);
    std::string col;
    int col_num = 0;

    odb::dbInst* instA;
    odb::dbInst* instB;
    odb::Point apA;
    odb::Point apB;
    int cost = 0;

    while (std::getline(ss, col, ',')) {
      switch (csv(col_num)) {
        case csv::iterm1:
          instA = block->findITerm(col.c_str())->getInst();
          break;
        case csv::iterm2:
          instB = block->findITerm(col.c_str())->getInst();
          break;
        case csv::apA_x:
          apA.setX(std::stoi(col));
          break;
        case csv::apA_y:
          apA.setY(std::stoi(col));
          break;
        case csv::apB_x:
          apB.setX(std::stoi(col));
          break;
        case csv::apB_y:
          apB.setY(std::stoi(col));
          break;

        case csv::cost:
          cost = std::stoi(col);
      }
      col_num++;
    }
    if (col_num != 7) {
      std::cout << "Malformed edge " << line;
      continue;
    }

    if (instA->getName() != instB->getName()) {
      std::cout << fmt::format(
          "Read intra {} {} \n", instA->getName(), instB->getName());
    }

    odb::dbTransform xform_A(instA->getLocation());
    xform_A.setOrient(odb::dbOrientType::R0);
    xform_A.apply(apA);

    odb::dbTransform xform_B(instB->getLocation());
    xform_B.setOrient(odb::dbOrientType::R0);
    xform_B.apply(apB);

    if (cost != 1000) {
      continue;
    }

    edges.emplace_back(apA, apB);
  }
  std::cout << fmt::format("Have {} edges\n", edges.size());

  // Notify the observer that something interesting has happened.
  if (debug_observer_) {
    debug_observer_->addEdges(edges);
  } else {
    std::cout << "No observer!\n";
  }
}

void CGV::setDebug(std::unique_ptr<Observer>& observer)
{
  debug_observer_ = std::move(observer);
}

}  // namespace cgv
