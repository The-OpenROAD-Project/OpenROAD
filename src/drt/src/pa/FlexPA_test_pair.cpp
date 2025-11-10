// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "db/infra/frTime.h"
#include "db/obj/frAccess.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frInst.h"
#include "db/obj/frVia.h"
#include "distributed/PinAccessJobDescription.h"
#include "distributed/frArchive.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "omp.h"
#include "pa/FlexPA.h"
#include "serialization.h"
#include "utl/Logger.h"
#include "utl/exception.h"

using namespace drt;

frAccessPoint* FlexPA::findAp(frInstTerm* iterm, int x, int y)
{
  auto mterm = iterm->getTerm();
  int paIdx = iterm->getInst()->getPinAccessIdx();
  frAccessPoint* ap = nullptr;

  for (auto& pin : mterm->getPins()) {
    if (!pin->hasPinAccess()) {
      continue;
    }
    for (auto& cur : pin->getPinAccess(paIdx)->getAccessPoints()) {
      if (x == cur->getPoint().x() && y == cur->getPoint().y()) {
        ap = cur.get();
        break;
      }
    }
  }

  return ap;
}

std::unique_ptr<frVia> FlexPA::buildVia(frInstTerm* iterm,
              int x,
              int y,
              std::vector<std::pair<frConnFig*, frBlockObject*>>& objs)
{
  odb::dbTransform xform = iterm->getInst()->getNoRotationTransform();
  const frAccessPoint* ap_1 = findAp(iterm, x, y);

  
  if (ap_1->hasAccess(frDirEnum::U)) {
    odb::Point pt1(ap_1->getPoint());
    xform.apply(pt1);
    std::unique_ptr<frVia> via1 = std::make_unique<frVia>(ap_1->getViaDef(), pt1);
    via1->setOrigin(pt1);
    if (iterm->hasNet()) {
      objs.emplace_back(via1.get(), iterm->getNet());
    } else {
      objs.emplace_back(via1.get(), iterm);
    }

    return via1;
  }
  
  return nullptr;
}

int FlexPA::getEdgeCostCE(frInstTerm* itermA,
                        int pinA_X,
                        int pinA_Y,
                        frInstTerm* itermB,
                        int pinB_X,
                        int pinB_Y)
{
  int edge_cost = 0;
  bool has_vio = false;

  auto ap_A = findAp(itermA, pinA_X, pinA_Y);
  auto ap_B = findAp(itermB, pinB_X, pinB_Y);

  // check DRC
  std::vector<std::pair<frConnFig*, frBlockObject*>> objs;
  if(ap_A == nullptr || ap_B == nullptr) {
    return -1;
  }
  
  auto via1 = buildVia(itermA, pinA_X, pinA_Y, objs);
  auto via2 = buildVia(itermB, pinB_X, pinB_Y, objs);
  has_vio = !genPatternsGC({itermA->getInst(), itermB->getInst()}, objs, Edge);

  if (!has_vio) {
      const int prev_node_cost = ap_A->getCost();
      const int curr_node_cost = ap_B->getCost();
      edge_cost = (prev_node_cost + curr_node_cost) / 2;
    } else {
    edge_cost = 1000;
  }
  
  return edge_cost;
}