//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Precision Innovations Inc.
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

#include "sroute.h"

#include <iostream>

#include "domain.h"
#include "grid.h"
#include "pdn/PdnGen.hh"
#include "shape.h"
#include "via.h"
namespace pdn {

using utl::PDN;

SRoute::SRoute(PdnGen* pdngen, odb::dbDatabase* db, utl::Logger* logger)
    : logger_(logger), pdngen_(pdngen), db_(db)
{
}

std::vector<odb::dbSBox*> SRoute::findRingShapes(odb::dbNet* net, uint& Hdy)
{
  // find all 4 strips for the ring
  uint Hdx = 0;
  uint Vdy = 0;
  for (auto* swire : net->getSWires()) {
    for (auto* wire : swire->getWires()) {
      Vdy = std::max(wire->getDY(), Vdy);
      Hdx = std::max(wire->getDX(), Hdx);
    }
  }

  uint Vdx = 0;
  for (auto* swire : net->getSWires()) {
    for (auto* wire : swire->getWires()) {
      if (wire->getDY() == Vdy) {
        Vdx = std::max(wire->getDX(), Vdx);
      }
      if (wire->getDX() == Hdx) {
        Hdy = std::max(wire->getDY(), Hdy);
      }
    }
  }

  std::vector<odb::dbSBox*> shapes;
  for (auto* swire : net->getSWires()) {
    for (auto* wire : swire->getWires()) {
      if (((wire->getDY() == Hdy) && (wire->getDX() == Hdx))
          || ((wire->getDX() == Vdx) && (wire->getDY() == Vdy))) {
        shapes.push_back(wire);
      }
    }
  }

  return shapes;
}

void SRoute::addSrouteInst(odb::dbNet* net,
                           odb::dbInst* inst,
                           const char* iterm_name,
                           const std::vector<odb::dbSBox*>& ring)
{
  odb::dbITerm* iterm = inst->findITerm(iterm_name);
  if (!iterm) {
    logger_->error(PDN,
                   115,
                   "Can't find iterm {} on inst {}",
                   iterm_name,
                   inst->getName());
  }
  iterm->disconnect();
  iterm->connect(net);

  int iterm_x = 0;
  int iterm_y = 0;
  iterm->getAvgXY(&iterm_x, &iterm_y);

  int i = 0;
  int best_i = 0;
  int low = std::numeric_limits<int>::max();
  for (auto* wire : ring) {
    const int wire_x = (wire->xMin() + wire->xMax()) / 2;
    const int wire_y = (wire->yMin() + wire->yMax()) / 2;
    const int dist = std::abs(wire_x - iterm_x) + std::abs(wire_y - iterm_y);
    if (dist < low) {
      best_i = i;
      low = dist;
    }
    i++;
  }

  if (sroute_itermss_.empty()) {
    sroute_itermss_.push_back({});
    sroute_itermss_.push_back({});
    sroute_itermss_.push_back({});
    sroute_itermss_.push_back({});
  }
  sroute_itermss_[best_i].push_back(iterm);
}

void SRoute::createSrouteWires(
    const char* net_name,
    const char* outer_net_name,
    odb::dbTechLayer* layer0,
    odb::dbTechLayer* layer1,
    const int cut_pitch_x,
    const int cut_pitch_y,
    const std::vector<odb::dbTechViaGenerateRule*>& vias,
    const std::vector<odb::dbTechVia*>& techvias,
    const int max_rows,
    const int max_columns,
    const std::vector<odb::dbTechLayer*>& ongrid,
    const std::vector<int>& metalwidths,
    const std::vector<int>& metalspaces,
    const std::vector<odb::dbInst*>& insts)
{
  auto* block = db_->getChip()->getBlock();

  odb::dbNet* net = block->findNet(net_name);
  if (!net) {
    logger_->error(PDN, 116, "Can't find net {}", net_name);
  }

  uint Hdy = 0;
  auto ring = findRingShapes(net, Hdy);

  sroute_itermss_.clear();
  for (auto* inst : insts) {
    addSrouteInst(net, inst, net_name, ring);
  }

  odb::dbNet* outer_net = block->findNet(outer_net_name);

  int index = 0;
  for (const auto& sroute_iterms : sroute_itermss_) {
    if (sroute_iterms.empty()) {
      ++index;
      continue;
    }
    int64_t sum_iterm_x = 0;
    int64_t sum_iterm_y = 0;
    int high_y = std::numeric_limits<int>::min();
    int low_y = std::numeric_limits<int>::max();
    for (auto* iterm : sroute_iterms) {
      int x = 0;
      int y = 0;
      iterm->getAvgXY(&x, &y);
      sum_iterm_x += x;
      sum_iterm_y += y;
      const odb::Rect bbox = iterm->getBBox();
      if (bbox.yMin() < low_y) {
        low_y = bbox.yMin();
      }
      if (bbox.yMax() > high_y) {
        high_y = bbox.yMax();
      }
    }

    int avg_iterm_x = sum_iterm_x / sroute_iterms.size();
    int avg_iterm_y = sum_iterm_y / sroute_iterms.size();

    odb::dbTechLayer* metal_layer = layer0;
    odb::dbTechLayer* stripe_metal_layer;

    odb::dbSBox* pdn_wire = new odb::dbSBox();
    odb::dbSWire* nwsw = odb::dbSWire::create(net, odb::dbWireType::ROUTED);

    bool first = true;
    int direction;
    // find closest metal stripe vertical (horizontal)
    // for vertical power ring connection
    if (ring[index]->getDir() == 0) {
      for (auto* swire : net->getSWires()) {
        for (auto* wire : swire->getWires()) {
          stripe_metal_layer = wire->getTechLayer();
          direction = wire->getDir();
          if (first) {
            if ((direction == 1) && (stripe_metal_layer == metal_layer)
                && (wire->getDY() != Hdy)) {
              first = false;
              pdn_wire = wire;
            }
          } else {
            if ((direction == 1) && (stripe_metal_layer == metal_layer)
                && (wire->getDY() != Hdy)
                && (std::abs(wire->yMin() - avg_iterm_y)
                    < std::abs(pdn_wire->yMin() - avg_iterm_y))) {
              pdn_wire = wire;
            }
          }
        }
      }
      odb::dbSBox* right_pdn_wire = new odb::dbSBox();
      first = true;
      // find closest wire to right of the center point
      for (auto* swire : outer_net->getSWires()) {
        for (auto* wire : swire->getWires()) {
          direction = wire->getDir();
          if (first) {
            if ((direction == 0) && (wire->xMax() > avg_iterm_x)) {
              first = false;
              right_pdn_wire = wire;
            }
          } else {
            if ((direction == 0) && (wire->xMax() < pdn_wire->xMax())
                && (wire->xMax() > avg_iterm_x)) {
              right_pdn_wire = wire;
            }
          }
        }
      }

      // find closest wire to left of the center point
      odb::dbSBox* left_pdn_wire = new odb::dbSBox();
      first = true;
      for (auto* swire : outer_net->getSWires()) {
        for (auto* wire : swire->getWires()) {
          direction = wire->getDir();
          if (first) {
            if ((direction == 0) && (wire->xMin() < avg_iterm_x)) {
              first = false;
              left_pdn_wire = wire;
            }
          } else {
            if ((direction == 0) && (wire->xMin() > pdn_wire->xMin())
                && (wire->xMin() < avg_iterm_x)) {
              left_pdn_wire = wire;
            }
          }
        }
      }

      // if center point is in the middle of two wire
      if ((left_pdn_wire->xMax() < avg_iterm_x)
          && (right_pdn_wire->xMin() > avg_iterm_x)) {
        if ((left_pdn_wire->xMax() + metalspaces[0] + metalwidths[0] / 2)
            > avg_iterm_x) {
          avg_iterm_x
              = left_pdn_wire->xMax() + metalspaces[0] + metalwidths[0] / 2;
        } else if ((right_pdn_wire->xMin() - metalspaces[0]
                    - metalwidths[0] / 2)
                   < avg_iterm_x) {
          avg_iterm_x
              = right_pdn_wire->xMin() - metalspaces[0] - metalwidths[0] / 2;
        }
      }
      // if center point is on the rightwire
      else if (right_pdn_wire->xMin() < avg_iterm_x) {
        avg_iterm_x
            = right_pdn_wire->xMin() - metalspaces[0] - metalwidths[0] / 2;
      }
      // if center point is on the leftwire
      else if (left_pdn_wire->xMax() > avg_iterm_x) {
        avg_iterm_x
            = left_pdn_wire->xMax() + metalspaces[0] + metalwidths[0] / 2;
      }

      odb::dbSBox::create(nwsw,
                          ongrid[0],
                          ring[index]->xMin(),
                          (pdn_wire->yMin() + pdn_wire->yMax()) / 2
                              - metalwidths[metalwidths.size() - 1] / 2,
                          avg_iterm_x,
                          (pdn_wire->yMin() + pdn_wire->yMax()) / 2
                              + metalwidths[metalwidths.size() - 1] / 2,
                          odb::dbWireShapeType::NONE);

      if ((pdn_wire->yMax() > high_y) && (pdn_wire->yMax() > low_y)) {
        odb::dbSBox::create(nwsw,
                            ongrid[ongrid.size() - 1],
                            avg_iterm_x - metalwidths[0],
                            low_y,
                            avg_iterm_x,
                            (pdn_wire->yMin() + pdn_wire->yMax()) / 2
                                + metalwidths[metalwidths.size() - 1] / 2,
                            odb::dbWireShapeType::NONE);
      }
      // middle
      else if ((pdn_wire->yMax() < high_y) && (pdn_wire->yMin() > low_y)) {
        odb::dbSBox::create(nwsw,
                            ongrid[ongrid.size() - 1],
                            avg_iterm_x - metalwidths[0],
                            low_y,
                            avg_iterm_x,
                            high_y,
                            odb::dbWireShapeType::NONE);
      } else {
        odb::dbSBox::create(nwsw,
                            ongrid[ongrid.size() - 1],
                            avg_iterm_x - metalwidths[0],
                            (pdn_wire->yMin() + pdn_wire->yMax()) / 2
                                - metalwidths[metalwidths.size() - 1] / 2,
                            avg_iterm_x,
                            high_y,
                            odb::dbWireShapeType::NONE);
      }

      if (ongrid[ongrid.size() - 1] < ongrid[0]) {
        for (auto* via : techvias) {
          odb::dbSet<odb::dbBox> boxes = via->getBoxes();
          odb::dbBox* box = *boxes.begin();
          int via_width = box->getDX();

          int rows
              = std::min((odb::uint) max_rows,
                         (metalwidths[metalwidths.size() - 1] - cut_pitch_y)
                             / (cut_pitch_y + box->getDY()));
          int cols = std::min(
              (odb::uint) max_columns,
              (ring[index]->xMax() - ring[index]->xMin() - cut_pitch_x)
                  / (cut_pitch_x + box->getDX()));
          int64_t centerX = cols / 2;
          int64_t centerY = rows / 2;
          int64_t row = 0;
          int64_t col = 0;
          if (rows % 2 == 1) {
            row = (pdn_wire->yMin() + pdn_wire->yMax()) / 2
                  - centerY * (via_width + cut_pitch_y) - via_width / 2;
          } else {
            row = (pdn_wire->yMin() + pdn_wire->yMax()) / 2
                  - centerY * (via_width) -cut_pitch_y / 2;
          }
          for (int r = 0; r < rows; r++) {
            if (cols % 2 == 1) {
              col = (ring[index]->xMin() + ring[index]->xMax()) / 2
                    - centerX * (via_width + cut_pitch_x) - via_width / 2;
            } else {
              col = (ring[index]->xMin() + ring[index]->xMax()) / 2
                    - centerX * (via_width) -std::max(centerX - 1, int64_t(0))
                          * cut_pitch_x
                    - cut_pitch_x / 2;
            }
            for (int c = 0; c < cols; c++) {
              odb::dbSBox::create(nwsw,
                                  via,
                                  col + via_width / 2,
                                  row + via_width / 2,
                                  odb::dbWireShapeType::NONE);
              col += cut_pitch_x + via_width;
            }
            row += cut_pitch_y + via_width;
          }
        }
      }

      for (int i = 1; i < ongrid.size() - 1; i++) {
        odb::dbSBox::create(nwsw,
                            ongrid[i],
                            avg_iterm_x - metalwidths[0],
                            (pdn_wire->yMin() + pdn_wire->yMax()) / 2
                                - metalwidths[metalwidths.size() - 1] / 2,
                            avg_iterm_x,
                            (pdn_wire->yMin() + pdn_wire->yMax()) / 2
                                + metalwidths[metalwidths.size() - 1] / 2,
                            odb::dbWireShapeType::NONE);
      }
      DbVia::ViaLayerShape shapes;
      for (auto* via : techvias) {
        odb::dbSet<odb::dbBox> boxes = via->getBoxes();
        odb::dbBox* box = *(boxes.begin());
        int via_width = box->getDX();
        int rows = std::min((odb::uint) max_rows,
                            (metalwidths[metalwidths.size() - 1] - cut_pitch_y)
                                / (cut_pitch_y + box->getDY()));
        int cols = std::min(
            (odb::uint) max_columns,
            (metalwidths[0] - cut_pitch_x) / (cut_pitch_x + box->getDX()));
        int64_t centerX = cols / 2;
        int64_t centerY = rows / 2;
        int64_t row = 0;
        int64_t col = 0;
        if (rows % 2 == 1) {
          row = (pdn_wire->yMin() + pdn_wire->yMax()) / 2
                - centerY * (via_width + cut_pitch_y) - via_width / 2;
        } else {
          row = (pdn_wire->yMin() + pdn_wire->yMax()) / 2
                - centerY * (via_width) -cut_pitch_y / 2;
        }
        for (int r = 0; r < rows; r++) {
          if (cols % 2 == 1) {
            col = avg_iterm_x - metalwidths[0] / 2
                  - centerX * (via_width + cut_pitch_x) - via_width / 2;
          } else {
            col = avg_iterm_x - metalwidths[0] / 2
                  - centerX * (via_width) -std::max(centerX - 1, int64_t(0))
                        * cut_pitch_x
                  - cut_pitch_x / 2;
          }
          for (int c = 0; c < cols; c++) {
            odb::dbSBox::create(nwsw,
                                via,
                                col + via_width / 2,
                                row + via_width / 2,
                                odb::dbWireShapeType::NONE);
            col += cut_pitch_x + via_width;
          }
          row += cut_pitch_y + via_width;
        }
      }
      for (auto* iterm : sroute_iterms) {
        odb::Rect bbox = iterm->getBBox();
        if (bbox.xMin() > avg_iterm_x - metalwidths[0]) {
          odb::dbSBox::create(nwsw,
                              ongrid[0],
                              avg_iterm_x - metalwidths[0],
                              bbox.yMin(),
                              bbox.xMin(),
                              bbox.yMax(),
                              odb::dbWireShapeType::NONE);
        } else if (bbox.xMax() < avg_iterm_x) {
          odb::dbSBox::create(nwsw,
                              ongrid[0],
                              bbox.xMax(),
                              bbox.yMin(),
                              avg_iterm_x,
                              bbox.yMax(),
                              odb::dbWireShapeType::NONE);
        } else {
          odb::dbSBox::create(nwsw,
                              ongrid[0],
                              avg_iterm_x - metalwidths[0],
                              bbox.yMin(),
                              avg_iterm_x,
                              bbox.yMax(),
                              odb::dbWireShapeType::NONE);
        }

        for (auto* via : techvias) {
          odb::dbSet<odb::dbBox> boxes = via->getBoxes();
          odb::dbBox* box = *boxes.begin();
          int via_width = box->getDX();

          int rows = std::min((odb::uint) max_rows,
                              (bbox.yMax() - bbox.yMin() - cut_pitch_y)
                                  / (cut_pitch_y + box->getDY()));
          int cols = std::min(
              (odb::uint) max_columns,
              (metalwidths[0] - cut_pitch_x) / (cut_pitch_x + box->getDX()));
          int64_t centerX = cols / 2;
          int64_t centerY = rows / 2;
          int64_t row = 0;
          int64_t col = 0;
          if (rows % 2 == 1) {
            row = (bbox.yMax() + bbox.yMin()) / 2
                  - centerY * (via_width + cut_pitch_y) - via_width / 2;
          } else {
            row = (bbox.yMax() + bbox.yMin()) / 2
                  - centerY * (via_width) -cut_pitch_y / 2;
          }
          for (int r = 0; r < rows; r++) {
            if (cols % 2 == 1) {
              col = avg_iterm_x - metalwidths[0] / 2
                    - centerX * (via_width + cut_pitch_x) - via_width / 2;
            } else {
              col = avg_iterm_x - metalwidths[0] / 2
                    - centerX * (via_width) -std::max(centerX - 1, int64_t(0))
                          * cut_pitch_x
                    - cut_pitch_x / 2;
            }
            for (int c = 0; c < cols; c++) {
              odb::dbSBox::create(nwsw,
                                  via,
                                  col + via_width / 2,
                                  row + via_width / 2,
                                  odb::dbWireShapeType::NONE);
              col += cut_pitch_x + via_width;
            }
            row += cut_pitch_y + via_width;
          }
        }
      }

    } else {
      pdn_wire = ring[index];

      // check to see if center point is too far
      if ((pdn_wire->xMax() - 1000) < avg_iterm_x) {
        std::cout << "xmax is " << pdn_wire->xMax() << std::endl;
        odb::dbSBox::create(nwsw,
                            ongrid[ongrid.size() - 1],
                            pdn_wire->xMax() - metalwidths[0],
                            pdn_wire->yMin(),
                            pdn_wire->xMax(),
                            high_y,
                            odb::dbWireShapeType::NONE);
        avg_iterm_x = pdn_wire->xMax() - metalwidths[0];
      } else if ((pdn_wire->xMin() + metalwidths[0]) > avg_iterm_x) {
        odb::dbSBox::create(nwsw,
                            ongrid[ongrid.size() - 1],
                            pdn_wire->xMin(),
                            pdn_wire->yMin(),
                            pdn_wire->xMin() + metalwidths[0],
                            high_y,
                            odb::dbWireShapeType::NONE);
        avg_iterm_x = pdn_wire->xMin() + metalwidths[0];
      } else {
        odb::dbSBox::create(nwsw,
                            ongrid[ongrid.size() - 1],
                            avg_iterm_x - metalwidths[0] / 2,
                            pdn_wire->yMin(),
                            avg_iterm_x + metalwidths[0] / 2,
                            high_y,
                            odb::dbWireShapeType::NONE);
      }

      for (int i = 1; i < ongrid.size() - 1; i++) {
        odb::dbSBox::create(nwsw,
                            ongrid[i],
                            avg_iterm_x - metalwidths[0] / 2,
                            pdn_wire->yMin(),
                            avg_iterm_x + metalwidths[0] / 2,
                            pdn_wire->yMax(),
                            odb::dbWireShapeType::NONE);
      }

      for (auto* via : techvias) {
        odb::dbSet<odb::dbBox> boxes = via->getBoxes();
        odb::dbBox* box = *boxes.begin();
        int via_width = box->getDX();

        int rows = std::min((odb::uint) max_rows,
                            (pdn_wire->yMax() - pdn_wire->yMin() - cut_pitch_y)
                                / (cut_pitch_y + box->getDY()));
        int cols = std::min(
            (odb::uint) max_columns,
            (metalwidths[0] - cut_pitch_x) / (cut_pitch_x + box->getDX()));
        int64_t centerX = cols / 2;
        int64_t centerY = rows / 2;
        int64_t row = 0;
        int64_t col = 0;
        if (rows % 2 == 1) {
          row = (pdn_wire->yMin() + pdn_wire->yMax()) / 2
                - centerY * (via_width + cut_pitch_y) - via_width / 2;
        } else {
          row = (pdn_wire->yMin() + pdn_wire->yMax()) / 2
                - centerY * (via_width) -cut_pitch_y / 2;
        }
        for (int r = 0; r < rows; r++) {
          if (cols % 2 == 1) {
            col = avg_iterm_x - metalwidths[0] / 2
                  - centerX * (via_width + cut_pitch_x) - via_width / 2;
          } else {
            col = avg_iterm_x - metalwidths[0] / 2
                  - centerX * (via_width) -std::max(centerX - 1, int64_t(0))
                        * cut_pitch_x
                  - cut_pitch_x / 2;
          }
          for (int c = 0; c < cols; c++) {
            odb::dbSBox::create(nwsw,
                                via,
                                col + via_width / 2,
                                row + via_width / 2,
                                odb::dbWireShapeType::NONE);
            col += cut_pitch_x + via_width;
          }
          row += cut_pitch_y + via_width;
        }
      }

      for (auto* iterm : sroute_iterms) {
        odb::Rect bbox = iterm->getBBox();
        if (bbox.xMin() > avg_iterm_x + metalwidths[0]) {
          odb::dbSBox::create(nwsw,
                              ongrid[0],
                              avg_iterm_x - metalwidths[0],
                              bbox.yMin(),
                              bbox.xMin(),
                              bbox.yMax(),
                              odb::dbWireShapeType::NONE);
        } else if (bbox.xMax() < avg_iterm_x - metalwidths[0]) {
          odb::dbSBox::create(nwsw,
                              ongrid[0],
                              bbox.xMax(),
                              bbox.yMin(),
                              avg_iterm_x + metalwidths[0],
                              bbox.yMax(),
                              odb::dbWireShapeType::NONE);
        } else {
          odb::dbSBox::create(nwsw,
                              ongrid[0],
                              avg_iterm_x - metalwidths[0] / 2,
                              bbox.yMin(),
                              avg_iterm_x + metalwidths[0] / 2,
                              bbox.yMax(),
                              odb::dbWireShapeType::NONE);
        }

        for (auto* via : techvias) {
          odb::dbSet<odb::dbBox> boxes = via->getBoxes();
          odb::dbBox* box = *boxes.begin();
          int via_width = box->getDX();

          int rows = std::min((odb::uint) max_rows,
                              (bbox.yMax() - bbox.yMin() - cut_pitch_y)
                                  / (cut_pitch_y + box->getDY()));
          int cols = std::min(
              (odb::uint) max_columns,
              (metalwidths[0] - cut_pitch_x) / (cut_pitch_x + box->getDX()));
          int64_t centerX = cols / 2;
          int64_t centerY = rows / 2;
          int64_t row = 0;
          int64_t col = 0;
          if (rows % 2 == 1) {
            row = (bbox.yMax() + bbox.yMin()) / 2
                  - centerY * (via_width + cut_pitch_y) - via_width / 2;
          } else {
            row = (bbox.yMax() + bbox.yMin()) / 2
                  - centerY * (via_width) -cut_pitch_y / 2;
          }
          for (int r = 0; r < rows; r++) {
            if (cols % 2 == 1) {
              col = avg_iterm_x - metalwidths[0] / 2
                    - centerX * (via_width + cut_pitch_x) - via_width / 2;
            } else {
              col = avg_iterm_x - metalwidths[0] / 2
                    - centerX * (via_width) -std::max(centerX - 1, int64_t(0))
                          * cut_pitch_x
                    - cut_pitch_x / 2;
            }
            for (int c = 0; c < cols; c++) {
              odb::dbSBox::create(nwsw,
                                  via,
                                  col + via_width / 2,
                                  row + via_width / 2,
                                  odb::dbWireShapeType::NONE);
              col += cut_pitch_x + via_width;
            }
            row += cut_pitch_y + via_width;
          }
        }
      }
    }
    std::map<odb::dbNet*, odb::dbSWire*> net_map;

    net_map[net] = nwsw;
    auto domains = getDomains();

    // collect the the SWires from the block
    ShapeVectorMap obstructions_vec;
    Shape::populateMapFromDb(net, obstructions_vec);
    const Shape::ObstructionTreeMap obstructions
        = Shape::convertVectorToObstructionTree(obstructions_vec);

    for (auto* domain : domains) {
      for (const auto& grid : domain->getGrids()) {
        grid->writeToDb(net_map, false, obstructions);
        grid->makeRoutingObstructions(db_->getChip()->getBlock());
      }
    }
    index++;
  }
}

std::vector<VoltageDomain*> SRoute::getDomains() const
{
  return pdngen_->getDomains();
}

}  // namespace pdn
