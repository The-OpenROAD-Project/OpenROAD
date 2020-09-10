/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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
//
///////////////////////////////////////////////////////////////////////////////

#include "AntennaRepair.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Box.h"
#include "Coordinate.h"
#include "Pin.h"
#include "Net.h"
#include "fastroute/GlobalRouter.h"
#include "openroad/Error.hh"

namespace FastRoute {

using ord::error;

AntennaRepair::AntennaRepair(GlobalRouter *grouter,
                 antenna_checker::AntennaChecker* arc,
                 opendp::Opendp* opendp, odb::dbDatabase* db)
	: _grouter(grouter), _arc(arc), _opendp(opendp), _db(db)
	{
		_block = _db->getChip()->getBlock();
	}

int AntennaRepair::checkAntennaViolations(NetRouteMap& routing,
					  int maxRoutingLayer)
{
  std::vector<odb::dbNet*> dirtyNets;
  odb::dbTech* tech = _db->getTech();

  _arc->load_antenna_rules();

  std::map<int, odb::dbTechVia*> defaultVias = _grouter->getDefaultVias(maxRoutingLayer);

  for (auto net_route : routing) {
    odb::dbNet* db_net = net_route.first;
    GRoute &route = net_route.second;
    odb::dbWire* wire = odb::dbWire::create(db_net);
    odb::dbWireEncoder wireEncoder;
    wireEncoder.begin(wire);
    odb::dbWireType wireType = odb::dbWireType::ROUTED;

    for (FastRoute::GSegment &seg : route) {
      if (std::abs(seg.initLayer - seg.finalLayer) > 1) {
        error("Global route segment not valid\n");
      }
      int x1 = seg.initX;
      int y1 = seg.initY;
      int x2 = seg.finalX;
      int y2 = seg.finalY;
      int l1 = seg.initLayer;
      int l2 = seg.finalLayer;

      odb::dbTechLayer* currLayer = tech->findRoutingLayer(l1);

      if (l1 == l2) {  // Add wire
        if (x1 == x2 && y1 == y2)
          continue;
        wireEncoder.newPath(currLayer, wireType);
        wireEncoder.addPoint(x1, y1);
        wireEncoder.addPoint(x2, y2);
      } else {  // Add via
        int bottomLayer = (l1 < l2) ? l1 : l2;
        wireEncoder.newPath(currLayer, wireType);
        wireEncoder.addPoint(x1, y1);
        wireEncoder.addTechVia(defaultVias[bottomLayer]);
      }
    }
    wireEncoder.end();

    odb::orderWires(db_net, false, false);

    std::vector<VINFO> netViol = _arc->get_net_antenna_violations(db_net);
    if (netViol.size() > 0) {
      _antennaViolations[db_net] = netViol;
      dirtyNets.push_back(db_net);
    }
    if (wire != nullptr) {
      odb::dbWire::destroy(wire);
    }
  }

  _grouter->setDirtyNets(dirtyNets);

  std::cout << "[INFO] #Antenna violations: " << _antennaViolations.size()
            << "\n";
  return _antennaViolations.size();
}

void AntennaRepair::fixAntennas(odb::dbMTerm* diodeMTerm)
{
  int siteWidth = -1;
  int cnt = 0;
  r_tree fixedInsts;
  getFixedInstances(fixedInsts);

  auto rows = _block->getRows();
  for (odb::dbRow* db_row : rows) {
    odb::dbSite* site = db_row->getSite();
    int site_width = site->getWidth();
    if (siteWidth == -1) {
      siteWidth = site_width;
    }

    if (siteWidth != site_width) {
      std::cout << "[WARNING] Design has rows with different site width\n";
    }
  }

  for (auto const& violation : _antennaViolations) {
    odb::dbNet* net = violation.first;
    for (int i = 0; i < violation.second.size(); i++) {
      for (odb::dbITerm* sinkITerm : violation.second[i].iterms) {
        odb::dbInst* sinkInst = sinkITerm->getInst();
        std::string antennaInstName = "ANTENNA_" + std::to_string(cnt);
        insertDiode(net,
                    diodeMTerm,
                    sinkInst,
                    sinkITerm,
                    antennaInstName,
                    siteWidth,
                    fixedInsts);
        cnt++;
      }
    }
  }
}

void AntennaRepair::legalizePlacedCells()
{
  _opendp->detailedPlacement(0);
  _opendp->checkPlacement(false);
}

void AntennaRepair::insertDiode(odb::dbNet* net,
                            odb::dbMTerm* diodeMTerm,
                            odb::dbInst* sinkInst,
                            odb::dbITerm* sinkITerm,
                            std::string antennaInstName,
                            int siteWidth,
                            r_tree& fixedInsts)
{
  bool legallyPlaced = false;
  bool placeAtLeft = true;
  int leftOffset = 0;
  int rightOffset = 0;
  int offset;

  std::string netName = net->getConstName();

  odb::dbMaster* antennaMaster = diodeMTerm->getMaster();

  int instLocX, instLocY, instWidth;
  odb::dbBox* sinkBBox = sinkInst->getBBox();
  instLocX = sinkBBox->xMin();
  instLocY = sinkBBox->yMin();
  instWidth = sinkBBox->xMax() - sinkBBox->xMin();
  odb::dbOrientType instOrient = sinkInst->getOrient();

  odb::dbInst* antennaInst
      = odb::dbInst::create(_block, antennaMaster, antennaInstName.c_str());
  odb::dbITerm* antennaITerm = antennaInst->findITerm(diodeMTerm->getConstName());
  odb::dbBox* antennaBBox = antennaInst->getBBox();
  int antennaWidth = antennaBBox->xMax() - antennaBBox->xMin();

  // Use R-tree to check if diode will not overlap or cause 1-site spacing with
  // other cells
  std::vector<value> overlapInsts;
  while (!legallyPlaced) {
    if (placeAtLeft) {
      offset = -(antennaWidth + leftOffset * siteWidth);
      leftOffset++;
      placeAtLeft = false;
    } else {
      offset = instWidth + rightOffset * siteWidth;
      rightOffset++;
      placeAtLeft = true;
    }

    antennaInst->setOrient(instOrient);
    antennaInst->setLocation(instLocX + offset, instLocY);

    odb::dbBox* instBox = antennaInst->getBBox();
    box box(point(instBox->xMin() - (2 * siteWidth) + 1, instBox->yMin() + 1),
            point(instBox->xMax() + (2 * siteWidth) - 1, instBox->yMax() - 1));
    fixedInsts.query(bgi::intersects(box), std::back_inserter(overlapInsts));

    if (overlapInsts.empty()) {
      legallyPlaced = true;
    }
    overlapInsts.clear();
  }

  antennaInst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  sinkInst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  odb::dbITerm::connect(antennaITerm, net);

  // Add diode to the R-tree of fixed instances
  int fixedInstId = fixedInsts.size();
  odb::dbBox* instBox = antennaInst->getBBox();
  box b(point(instBox->xMin(), instBox->yMin()),
        point(instBox->xMax(), instBox->yMax()));
  value v(b, fixedInstId);
  fixedInsts.insert(v);
  fixedInstId++;
}

void AntennaRepair::getFixedInstances(r_tree& fixedInsts)
{
  odb::dbTech* tech = _db->getTech();

  int fixedInstId = 0;
  for (odb::dbInst* inst : _block->getInsts()) {
    if (inst->getPlacementStatus() == odb::dbPlacementStatus::FIRM) {
      odb::dbBox* instBox = inst->getBBox();
      box b(point(instBox->xMin(), instBox->yMin()),
            point(instBox->xMax(), instBox->yMax()));
      value v(b, fixedInstId);
      fixedInsts.insert(v);
      fixedInstId++;
    }
  }
}

}
