///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
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

// Generator Code Begin Cpp
#include <iostream>
#include "dbPGpin.h"
#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTypes.h"
// User Code Begin Includes
#include <algorithm>

#include "dbBPin.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbITerm.h"
#include "dbLib.h"
#include "dbMPin.h"
#include "dbMaster.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechVia.h"
#include "dbVia.h"
#include "utl/Logger.h"
#include <vector>
#include <string>

// User Code End Includes
namespace odb {

_dbPGpin::_dbPGpin(_dbDatabase* db)
{
  // User Code Begin Constructor
  // User Code End Constructor
}

_dbPGpin::~_dbPGpin()
{
  // User Code Begin Destructor
  // User Code End Destructor
}

static int snapToGrid (int number, dbTech *db_tech) {
    // This will round "number" to a value that respects the manufacturing grid
    // For example, if the manufacturing grid is 5 microns and number = 20333,
    // the procedure will output 20335.
    auto grid = db_tech->getManufacturingGrid();
    return round(number / double(grid) * grid);
}

void dbPGpin::create_PGpin (dbBlock* block, dbTech* tech, const char * source_net_name, int num_connection_points, Position position) {

  utl::Logger* logger;
  dbNet* create_pin_net = block->findNet(source_net_name);
  dbTechLayer* PGpin_metal_layer; 
  dbSet<dbSWire> swires = create_pin_net->getSWires();
  dbSBox* pdn_wire = new dbSBox();

  bool first = true;
  int direction;
  //find highest metal layer
  for(auto swire : swires){
    for (auto wire : swire->getWires()) {
      if (first){
        first = false;
        PGpin_metal_layer = wire->getTechLayer();
        pdn_wire = wire;
      }
      else {
        if (wire->getTechLayer() > PGpin_metal_layer ) {
          PGpin_metal_layer = wire->getTechLayer();
          pdn_wire = wire;
        }
        else if (wire->getTechLayer() == PGpin_metal_layer ) {
          direction = wire->getDir();
          if (direction == 1) {
            if ((pdn_wire->getDY()) < (wire->getDY())) {
              pdn_wire = wire;
            }
          }
          else if (direction == 0) {
            if ((pdn_wire->getDX()) < (wire->getDX())) {
              pdn_wire = wire;
            }
          }
        }
      }
    }
  }
  
  
  std::string r_pin = "pg_" + std::string(source_net_name);
  dbNet* r_net = dbNet::create(block, r_pin.c_str(), true);
  int xMin[num_connection_points];
  int yMin[num_connection_points];
  int xMax[num_connection_points];
  int yMax[num_connection_points];
  
  // create PG_pin in all of the highest metal wires
  
  for (int n = 0; n < num_connection_points; n++) {
    
    dbBTerm * r_bterm = dbBTerm::create(r_net, (r_pin+ "_" + std::to_string(n)).c_str());
    dbBPin * r_bpin = dbBPin::create(r_bterm);
    r_bpin->setPlacementStatus("FIRM");

    direction = pdn_wire->getDir();
    if (direction == 1) {
      // for horizontal
      if (position == Position::LEFT) {
        int dx = snapToGrid((pdn_wire->getDX())/num_connection_points/2, tech);
        xMin[n] = pdn_wire->xMin() + n*dx;
        xMax[n] = xMin[n] + dx;


      } else if (position == Position::RIGHT) {
        int dx = snapToGrid((pdn_wire->getDX())/num_connection_points/2, tech);
        xMin[n] = pdn_wire->xMin() + (pdn_wire->getDX())/2 + n*dx;
        xMax[n] = xMin[n] + dx;


      } else if (position == Position::MIDDLE) {
        int dx = snapToGrid((pdn_wire->getDX())/num_connection_points/2, tech);
        xMin[n] = pdn_wire->xMin() + (pdn_wire->getDX())/4 + n*dx;
        xMax[n] = xMin[n] + dx;

      } else if (position == Position::DEFAULT) {
        int dx = snapToGrid((pdn_wire->getDX())/num_connection_points, tech);
        xMin[n] = pdn_wire->xMin() + n*dx;
        xMax[n] = xMin[n] + dx;

      } else {
        logger->error(utl::ODB,
                       1,
                       "wrong position.");
        break;
      }
      dbBox::create(r_bpin, pdn_wire->getTechLayer(), xMin[n], pdn_wire->yMin(), xMax[n], pdn_wire->yMax()); //create physical box for net
    } 
    else if (direction == 0) {
      //for vertical
      // for horizontal
      if (position == Position::LEFT) {
        int dy = snapToGrid((pdn_wire->getDY())/num_connection_points/2, tech);
        yMin[n] = pdn_wire->yMin() + n*dy;
        yMax[n] = yMin[n] + dy;

      } else if (position == Position::RIGHT) {
        int dy = snapToGrid((pdn_wire->getDY())/num_connection_points/2, tech);
        yMin[n] = pdn_wire->yMin() + (pdn_wire->getDY())/2 + n*dy;
        yMax[n] = yMin[n] + dy;

      } else if (position == Position::MIDDLE) {
        int dy = snapToGrid((pdn_wire->getDY())/num_connection_points/2, tech);
        yMin[n] = pdn_wire->yMin() + (pdn_wire->getDY())/4 + n*dy;
        yMax[n] = yMin[n] + dy;

      } else if (position == Position::DEFAULT) {
        int dy = snapToGrid((pdn_wire->getDY())/num_connection_points, tech);
        yMin[n] = pdn_wire->yMin() + n*dy;
        yMax[n] = yMin[n] + dy;

      } else {
        logger->error(utl::ODB,
                       1,
                       "wrong position.");
        break;
      }
      dbBox::create(r_bpin, pdn_wire->getTechLayer(), pdn_wire->xMin(), yMin[n], pdn_wire->xMax(), yMax[n]); //create physical box for net

    } else {
      logger->error(utl::ODB,
                       1,
                       "wrong existed direction.");
    }
  }
  
}

void dbPGpin::create_custom_connections (dbBlock* block, const char* net, const char* inst, const char* iterm) {
  dbNet* net_ = block->findNet(net);

  dbInst* inst_ = block->findInst(inst);
  dbITerm* iterm_ = inst_->findITerm(iterm);
  iterm_->connect(net_);

}

}  // namespace odb
