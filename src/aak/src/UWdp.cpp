/////////////////////////////////////////////////////////////////////////////
// Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu
// Woo(mwoo@eng.ucsd.edu)
//          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
// Rewrite by James Cherry, Parallax Software, Inc.
//
// Copyright (c) 2019, The Regents of the University of California
// Copyright (c) 2018, SangGi Do and Mingyu Woo
// All rights reserved.
//
// BSD 3-Clause License
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
///////////////////////////////////////////////////////////////////////////////


// TODO:
// network creation - need to properly set position, orientation and power
//  information.  need to properly set pin offsets, size, etc.
// architecture creation - need to properly set power information on rows.
// other - need to properly figure out edge spacing information for cell gaps.




#include "aak/UWdp.h"

#include <iostream>
#include <cfloat>
#include <cmath>
#include <limits>
#include <map>
#include <odb/db.h>
#include <boost/format.hpp>

#include "utl/Logger.h"
#include "ord/OpenRoad.hh"  // closestPtInRect

// My stuff.
#include "network.h"
#include "architecture.h"
#include "router.h"
#include "detailed_manager.h"
#include "legalize.h"
#include "detailed.h"

namespace aak {

using std::round;
using std::string;

using utl::AAK;

using odb::dbBlock;
using odb::dbBox;
using odb::dbBPin;
using odb::dbBTerm;
using odb::dbITerm;
using odb::dbMasterType;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbNet;
using odb::dbPlacementStatus;
using odb::Rect;
using odb::dbSigType;
using odb::dbSet;
using odb::dbRegion;
using odb::dbSWire;
using odb::dbSBox;
using odb::dbWireType;
using odb::dbTechLayer;


static bool
swapWidthHeight(dbOrientType orient);


////////////////////////////////////////////////////////////////
UWdp::UWdp():
    nw_(nullptr),
    arch_(nullptr),
    rt_(nullptr)
{
}

////////////////////////////////////////////////////////////////
UWdp::~UWdp()
{
    if( nw_ ) delete nw_;
    if( arch_ ) delete arch_;
    if( rt_ ) delete rt_;
}

////////////////////////////////////////////////////////////////
void
UWdp::init(dbDatabase *db,
             Logger *logger)
{
  db_ = db;
  logger_ = logger;
}

////////////////////////////////////////////////////////////////
void
UWdp::improvePlacement()
{
  logger_->report( "UW legalization and detailed placement." );

  import();

  aak::DetailedMgr mgr( arch_, nw_, rt_ );
  aak::LegalizeParams lgParams;
  // The following will result in the legalizer taking whatever
  // it is given and do nothing other than snap the cells to
  // rows (segments) and do a cluming shift to remove overlap.
  // In other words, if the placement is already legal, my
  // legalizer should really do nothing other than set up the
  // data structures for use by the detailed placer.
  lgParams.m_skipLegalization = true;
  aak::Legalize lg( lgParams );
  lg.legalize( mgr );

  // The following is the detailed placer.  Currently it has
  // a hard-coded optimization string which will do:
  //
  // 1. Passes of independent set matching to reduce displacement.
  //
  // 2.Passes of random moves and swaps using a variety of techniques 
  // to reduce displacement as well as wire length.
  //
  // IMPORTANT:
  // At present, it is likely to not do very much since 
  // the targets for displacement are the original placement.
  // Therefore, any movement away from these positions is 
  // likely going to be "bad".  
  // 
  // One possible thing to do is to remove the displacement
  // objective.  In this case, it will focus on only improving
  // wire length.  I give an example of this below.  
  //
  // I will add more "stuff" soon; e.g., some sort of maximum
  // displacement penalty, a density control, drc penalties,
  // etc.

  // XXX: Perform detailed improvement.
  aak::DetailedParams dtParams;
  dtParams.m_script = "";
  // Set matching for displacement.
  //dtParams.m_script += "mis -p 10 -t 0.01 -d"; // Not hpwl, but displacement.
  dtParams.m_script += "mis -p 10 -t 0.01"; // Not displacement, but wire length.
  // Needed for the next command.
  dtParams.m_script += ";";
  // Random moves and swaps with cost = hpwl*(1.0+disp).
  //dtParams.m_script += "default -p 5 -f 20 -gen rng:disp -obj hpwl:disp -cost (hpwl)(1.0)(disp(+)(*))";
  dtParams.m_script += "default -p 5 -f 20 -gen rng -obj hpwl -cost (hpwl)"; // Only hpwl.
  aak::Detailed dt( dtParams );
  dt.improve( mgr );

  // Write solution back.
  updateDbInstLocations();
}
////////////////////////////////////////////////////////////////
void
UWdp::import()
{
  logger_->report( "import()" );

  nw_ = new Network;
  arch_ = new Architecture;
  rt_ = new RoutingParams;

  initEdgeTypes();
  initCellSpacingTable();
  createLayerMap();
  createNdrMap();
  createNetwork();
  createArchitecture();
  createRouteGrid();
  setUpNdrRules();
  setUpPlacementRegions();

  arch_->create_extra_nodes( nw_, rt_ ); // Should fix/remove this.
}
////////////////////////////////////////////////////////////////
void
UWdp::updateDbInstLocations()
{
  std::unordered_map<odb::dbInst*, Node*>::iterator it_n;
  dbBlock* block = db_->getChip()->getBlock();
  dbSet<dbInst> insts = block->getInsts();
  for(dbInst* inst : insts) 
  {
    auto type = inst->getMaster()->getType();
    if(!type.isCore() && !type.isBlock()) 
    {
      continue;
    }
    it_n = instMap_.find(inst);
    if( instMap_.end() != it_n )
    {
      Node* nd = it_n->second;
      // Need to figure out orientation.
      int lx = (int)(nd->getX() - 0.5 * nd->getWidth());
      int yb = (int)(nd->getY() - 0.5 * nd->getHeight());
      inst->setLocation( lx, yb );
    }
  }
}
////////////////////////////////////////////////////////////////
void
UWdp::initEdgeTypes()
{
  logger_->report( "initEdgeType() - not yet implemented, but needed." );

  arch_->init_edge_type();

  // XXX: This information is in the "LEF58_EDGETYPE",
  // at least in the ICCAD14, 15 and 17 contests.  
  // Where is it here??

  // This is required to ensure proper spacing between cells.
  ;
}
////////////////////////////////////////////////////////////////
void
UWdp::initCellSpacingTable()
{
  logger_->report( "initCellSpacingTable() - not yet implemented, but needed." );

  // XXX: This information is in the "LEF58_CELLEDGESPACINGTABLE",
  // at least in the ICCAD14, 15 and 17 contests.  
  // Where is it here?

  // This is required to ensure proper spacing between cells.
  ;
}
////////////////////////////////////////////////////////////////
void
UWdp::createLayerMap()
{
  logger_->report( "createLayerMap() - not yet implemented, maybe not needed yet." );

  // Relates to pin blockages, etc. Maybe not needed right now.
  ;
}
////////////////////////////////////////////////////////////////
void
UWdp::createNdrMap()
{
  logger_->report( "createNdrMap() - not yet implemented, maybe not needed yet." );

  // Relates to pin blockages, etc. Maybe not needed right now.
  ;
}
////////////////////////////////////////////////////////////////
void
UWdp::createNetwork()
{
  logger_->report( "createNetwork() - only partially done." );

  std::unordered_map<odb::dbInst*, Node*>::iterator it_n;
  std::unordered_map<odb::dbNet*, Edge*>::iterator it_e;
  std::unordered_map<odb::dbBTerm*, Node*>::iterator it_p;

  dbBlock* block = db_->getChip()->getBlock();

  pwrLayers_.clear();
  gndLayers_.clear();

  // I allocate some things statically, so first I need to do some counting.

  dbSet<dbInst> insts = block->getInsts();
  int nNodes = 0;
  int nEdges = 0;
  int nPins = 0;
  int nIPins = 0;
  int nBPins = 0;
  for(dbInst* inst : insts) 
  {
    auto type = inst->getMaster()->getType();
    if(!type.isCore() && !type.isBlock()) 
    {
      continue;
    }

    ++nNodes;
  }
  // Should the IO terminals come from looking at the nets, or are
  // they dbBTerm found on the block?  Or, either?

  dbSet<dbNet> nets = block->getNets();
  for(dbNet* net : nets) 
  {
    dbSigType netType = net->getSigType();
    ++nEdges;

    for(dbITerm* iTerm : net->getITerms()) 
    {
      ++nIPins;
      ++nPins;
    }
    for(dbBTerm* bTerm : net->getBTerms()) 
    {
      ++nBPins;
      ++nPins;
    }
  }

  // I need fakes nodes for the boundary pins.
  nw_->m_nodeNames.resize( nNodes + nBPins );
  nw_->m_nodes.resize( nNodes + nBPins );
  nw_->m_shapes.resize( nNodes + nBPins );
  for( size_t i = 0; i < nw_->m_shapes.size(); i++ )
  {
    nw_->m_shapes[i] = std::vector<Node*>();
  }
  nw_->m_edgeNames.resize( nEdges );
  nw_->m_edges.resize( nEdges );
  nw_->m_pins.resize( nPins );
  nw_->m_edgePins.resize( nPins );
  nw_->m_nodePins.resize( nPins );

  // ...  Need to consider orientation and symmetry a bit more within
  // detailed placement...

  int errors = 0;
  // Populate nodes.
  int n = 0;
  for(dbInst* inst : insts) 
  {
    auto type = inst->getMaster()->getType();
    if(!type.isCore() && !type.isBlock()) 
    {
      continue;
    }
    instMap_[inst] = &(nw_->m_nodes[n]);

    // Return everything to R0.  I think this is easier, although it means
    // I need to fix this somewhere to ensure orientations are, in fact,
    // correct at the end of detailed placement.
    inst->setLocationOrient( dbOrientType::R0 );

    nw_->m_nodeNames[n] = inst->getName();

    // Fill in data.
    nw_->m_nodes[n].setType( NodeType_CELL );
    nw_->m_nodes[n].setId( n );
    nw_->m_nodes[n].setFixed( NodeFixed_NOT_FIXED );
    if( inst->isFixed() )
    {
        nw_->m_nodes[n].setFixed( NodeFixed_FIXED_XY );
    }
    nw_->m_nodes[n].setAttributes( NodeAttributes_EMPTY );

    // I reset the inst orientiation to R0 which, I think, corresponds
    // to my Orientation_N.  Should I also update to account for R90,
    // etc.???
    unsigned orientations = Orientation_N;
    if( inst->getMaster()->getSymmetryX() && inst->getMaster()->getSymmetryY() )
    {
        orientations |= Orientation_FN;
        orientations |= Orientation_FS;
        orientations |= Orientation_S;
    }
    else if( inst->getMaster()->getSymmetryX() )
    {
        orientations |= Orientation_FS;
    }
    else if( inst->getMaster()->getSymmetryY() )
    {
        orientations |= Orientation_FN;
    }
    // else...  Account for R90?
    nw_->m_nodes[n].setAvailOrient( orientations );
    nw_->m_nodes[n].setCurrOrient( Orientation_N );
    nw_->m_nodes[n].setHeight( inst->getMaster()->getHeight() );
    nw_->m_nodes[n].setWidth( inst->getMaster()->getWidth() );
    // I use cell centers, unfortunately.
    double xc = inst->getBBox()->xMin() + 0.5 * inst->getMaster()->getWidth();
    double yc = inst->getBBox()->yMin() + 0.5 * inst->getMaster()->getHeight();
    nw_->m_nodes[n].setOrigX( xc );
    nw_->m_nodes[n].setOrigY( yc );
    nw_->m_nodes[n].setX( xc );
    nw_->m_nodes[n].setY( yc );
    // Not correct.
    nw_->m_nodes[n].setRightEdgeType( EDGETYPE_DEFAULT );
    nw_->m_nodes[n].setLeftEdgeType( EDGETYPE_DEFAULT );

    // Need to determine power at the top and bottom of the inst.  Grab
    // the master and look at the MTerms.  Figure our whether power is 
    // above or below ground and then set according to that.
    nw_->m_nodes[n].setBottomPower( RowPower_UNK );
    nw_->m_nodes[n].setTopPower( RowPower_UNK );
    dbMaster* master = inst->getMaster();
    double maxPwr = -std::numeric_limits<double>::max();
    double minPwr =  std::numeric_limits<double>::max();
    double maxGnd = -std::numeric_limits<double>::max();
    double minGnd =  std::numeric_limits<double>::max();
    bool isVdd = false;
    bool isGnd = false;
    for(dbMTerm* mterm : master->getMTerms())
    {
      // Do I need to look at the individual ports; i.e., the MTerms?
      // to see which one is on top, or is the surrounding box enough?
      if( mterm->getSigType() == dbSigType::POWER )
      {
        isVdd = true;
        for(dbMPin* mpin : mterm->getMPins())
        {
          // Geometry or box?
          double y = 0.5 * (mpin->getBBox().yMin() + mpin->getBBox().yMax());
          minPwr = std::min( minPwr, y );
          maxPwr = std::max( maxPwr, y );


          for(dbBox* mbox : mpin->getGeometry())
          {
            dbTechLayer* layer = mbox->getTechLayer();
            pwrLayers_.insert( layer );
          }
        }
      }
      else if( mterm->getSigType() == dbSigType::GROUND )
      {
        isGnd = true;
        for(dbMPin* mpin : mterm->getMPins())
        {
          // Geometry or box?
          double y = 0.5 * (mpin->getBBox().yMin() + mpin->getBBox().yMax());
          minGnd = std::min( minGnd, y );
          maxGnd = std::max( maxGnd, y );

          for(dbBox* mbox : mpin->getGeometry())
          {
            dbTechLayer* layer = mbox->getTechLayer();
            gndLayers_.insert( layer );
          }
        }
      }
    }
    if( isVdd && isGnd )
    {
      nw_->m_nodes[n].setBottomPower( minPwr < minGnd ? RowPower_VDD : RowPower_VSS  );
      nw_->m_nodes[n].setTopPower( maxPwr > maxGnd ? RowPower_VDD : RowPower_VSS );
    }

    // Set later.
    nw_->m_nodes[n].setRegionId( 0 ); 


    ++n; // Next node.
  }
  for(dbNet* net : nets) 
  {
    for(dbBTerm* bTerm : net->getBTerms()) 
    {
      pinMap_[bTerm] = &(nw_->m_nodes[n]);

      nw_->m_nodeNames[n] = "FakeForExtPins" + std::to_string(n);

      // Fill in data.
      nw_->m_nodes[n].setId( n );
      nw_->m_nodes[n].setType( NodeType_TERMINAL_NI );
      nw_->m_nodes[n].setFixed( NodeFixed_FIXED_XY );
      nw_->m_nodes[n].setAttributes( NodeAttributes_EMPTY );
      nw_->m_nodes[n].setAvailOrient( Orientation_N );
      nw_->m_nodes[n].setCurrOrient( Orientation_N );

      double ww = (bTerm->getBBox().xMax() - bTerm->getBBox().xMin());
      double hh = (bTerm->getBBox().yMax() - bTerm->getBBox().yMax());
      double xx = (bTerm->getBBox().xMax() + bTerm->getBBox().xMin()) * 0.5;
      double yy = (bTerm->getBBox().yMax() + bTerm->getBBox().yMax()) * 0.5;

      nw_->m_nodes[n].setHeight( hh );
      nw_->m_nodes[n].setWidth( ww );

      // Correct?  I do not think so...
      double xc = bTerm->getBlock()->getBBox()->xMin() + xx;
      double yc = bTerm->getBlock()->getBBox()->yMin() + yy;

      nw_->m_nodes[n].setOrigX( xc );
      nw_->m_nodes[n].setOrigY( yc );
      nw_->m_nodes[n].setX( xc );
      nw_->m_nodes[n].setY( yc );

      // Not correct.
      nw_->m_nodes[n].setRightEdgeType( EDGETYPE_DEFAULT );
      nw_->m_nodes[n].setLeftEdgeType( EDGETYPE_DEFAULT );

      nw_->m_nodes[n].setBottomPower( RowPower_UNK );
      nw_->m_nodes[n].setTopPower( RowPower_UNK );
      nw_->m_nodes[n].setRegionId( 0 ); // Set in another routine.


      ++n; // Next node.
    }
  }
  if( n != nNodes+nBPins )
  {
    ++errors;
  }

  // Populate edges.
  int e = 0;
  int p = 0;
  for(dbNet* net : nets) 
  {
    // Skip globals and pre-routes?
    //dbSigType netType = net->getSigType();

    nw_->m_edgeNames[e] = net->getName(); 

    nw_->m_edges[e].setId( e );

    netMap_[net] = &(nw_->m_edges[e]);
    for(dbITerm* iTerm : net->getITerms()) 
    {
      it_n = instMap_.find(iTerm->getInst());
      if( instMap_.end() != it_n )
      {
        n = it_n->second->getId(); // which node.
        nw_->m_pins[p].setId( p );
        nw_->m_pins[p].setEdgeId( e );
        nw_->m_pins[p].setNodeId( n );

        // Is this correct? 
        dbMTerm* mTerm = iTerm->getMTerm();
        dbMaster* master = mTerm->getMaster();
        // Due to old bookshelf, my offsets are from the 
        // center of the cell whereas in DEF, it's from
        // the bottom corner.
        double ww = (mTerm->getBBox().xMax() - mTerm->getBBox().xMin());
        double hh = (mTerm->getBBox().yMax() - mTerm->getBBox().yMax());
        double xx = (mTerm->getBBox().xMax() + mTerm->getBBox().xMin()) * 0.5;
        double yy = (mTerm->getBBox().yMax() + mTerm->getBBox().yMax()) * 0.5;
        double dx = xx - ((double)master->getWidth() / 2.); 
        double dy = yy - ((double)master->getHeight() / 2.); 

        nw_->m_pins[p].setOffsetX( dx );
        nw_->m_pins[p].setOffsetY( dy );
        nw_->m_pins[p].setPinHeight( hh );
        nw_->m_pins[p].setPinWidth( ww );

        // Not done...
        nw_->m_pins[p].setPinLayer( 0 );


        ++p; // next pin.
      }
      else
      {
        ++errors;
      }
    }
    for(dbBTerm* bTerm : net->getBTerms()) 
    {
      it_p = pinMap_.find(bTerm);
      if( pinMap_.end() != it_p )
      {
        n = it_p->second->getId(); // which node.
        nw_->m_pins[p].setId( p );
        nw_->m_pins[p].setEdgeId( e );
        nw_->m_pins[p].setNodeId( n );

        // Boundary pins don't need offsets and size.
        nw_->m_pins[p].setOffsetX( 0.0 );
        nw_->m_pins[p].setOffsetY( 0.0 );
        nw_->m_pins[p].setPinHeight( 0.0 );
        nw_->m_pins[p].setPinWidth( 0.0 );

        // Not done...
        nw_->m_pins[p].setPinLayer( 0 );


        ++p; // next pin.
      }
      else
      {
        ++errors;
      }
    }


    ++e; // next edge.
  }
  if( e != nEdges )
  {
    ++errors;
  }
  if( p != nPins )
  {
    ++errors;
  }

  // Connectivity information.
  {
    nw_->m_nodePins.resize( nw_->m_pins.size() );
    nw_->m_edgePins.resize( nw_->m_pins.size() );
    for( size_t i = 0; i < nw_->m_pins.size(); i++ )
    {
      nw_->m_nodePins[i] = &(nw_->m_pins[i]);
      nw_->m_edgePins[i] = &(nw_->m_pins[i]);
    }
    std::stable_sort( 
        nw_->m_nodePins.begin(), nw_->m_nodePins.end(), 
        Network::comparePinsByNodeId() 
        );
    p = 0;
    for( n = 0; n < nw_->m_nodes.size(); n++ )
    {
      Node& nd = nw_->m_nodes[n];

      nd.m_firstPin = p;
      while( p < nw_->m_nodePins.size() && nw_->m_nodePins[p]->getNodeId() == n )
        ++p;
      nd.m_lastPin = p;
    }

    std::stable_sort( 
        nw_->m_edgePins.begin(), nw_->m_edgePins.end(), 
        Network::comparePinsByEdgeId() 
        );
    p = 0;
    for( e = 0; e < nw_->m_edges.size(); e++ )
    {
      Edge& ed = nw_->m_edges[e];

      ed.m_firstPin = p;
      while( p < nw_->m_edgePins.size() && nw_->m_edgePins[p]->getEdgeId() == e )
        ++p;
      ed.m_lastPin = p;
    }
  }

  if( errors != 0 )
  {
    logger_->error( AAK, 1, "something went wrong creating network for legalization." );
  }
  else
  {
    logger_->info( AAK, 100, "network inst {}, edges {}, pins {}",
        nw_->m_nodes.size(), 
        nw_->m_edges.size(),
        nw_->m_pins.size() 
        );
  }
}
////////////////////////////////////////////////////////////////
void
UWdp::createArchitecture()
{
  logger_->report( "createArchitecture() - only partially done." );

  dbBlock* block = db_->getChip()->getBlock();

  dbSet<dbRow> rows = block->getRows();

  odb::Rect coreRect;
  block->getCoreArea(coreRect);
  odb::Rect dieRect;
  block->getDieArea(dieRect);

  for(dbRow* row : rows) 
  {
    if( row->getDirection() != odb::dbRowDir::HORIZONTAL )
    {
      // error.
      continue;
    }
    dbSite* site = row->getSite();
    int originX;
    int originY;
    row->getOrigin( originX, originY );

    Architecture::Row* archRow = new Architecture::Row;

    archRow->m_rowLoc           = (double)originY;
    archRow->m_rowHeight        = (double)site->getHeight();
    archRow->m_siteWidth        = (double)site->getWidth();
    archRow->m_siteSpacing      = (double)row->getSpacing();
    archRow->m_subRowOrigin     = (double)originX;
    archRow->m_numSites         = row->getSiteCount();

    // Is this correct?  No...
    archRow->m_powerBot         = RowPower_UNK; 
    archRow->m_powerTop         = RowPower_UNK; 
    // Is this correct?  No...
    archRow->m_siteOrient       = Orientation_N;
    // Is this correct?  No...
    archRow->m_siteSymmetry     = Symmetry_UNKNOWN;

    arch_->m_rows.push_back( archRow );
  }
  // The post processing sets up a box around all of the rows.  It checks to 
  // see if we do have colinear subrows (not sure all the code works in this
  // case).
  arch_->post_process();

  // Set up the architecture area to be the DIE area.
  if( arch_->m_xmin != (double) dieRect.xMin() ||
      arch_->m_xmax != (double) dieRect.xMax() ||
      arch_->m_ymin != (double) dieRect.yMin() ||
      arch_->m_ymax != (double) dieRect.yMax() 
      )
  {
    arch_->m_xmin = dieRect.xMin();
    arch_->m_xmax = dieRect.xMax();
  }

  for( int r = 0; r < arch_->m_rows.size(); r++ )
  {
    int numSites        = arch_->m_rows[r]->m_numSites;
    double originX      = arch_->m_rows[r]->m_subRowOrigin;
    double siteSpacing  = arch_->m_rows[r]->m_siteSpacing;

    double lx = originX;
    double rx = originX + numSites * siteSpacing;
    if( lx < arch_->m_xmin || rx > arch_->m_xmax )
    {
      if( lx < arch_->m_xmin )
      {
        originX = arch_->m_xmin;
      }
      rx = originX + numSites * siteSpacing;
      if( rx > arch_->m_xmax )
      {
        numSites = (int)((arch_->m_xmax - originX) / siteSpacing);
      }

      if( arch_->m_rows[r]->m_subRowOrigin != originX )
      {
        arch_->m_rows[r]->m_subRowOrigin = originX;
      }
      if( arch_->m_rows[r]->m_numSites != numSites )
      {
        arch_->m_rows[r]->m_numSites = numSites;
      }
    }
  }

  // Need the power running across the bottom and top of each
  // row.  I think the way to do this is to look for power 
  // and ground nets and then look at the special wires.
  // Not sure, though, of the best way to pick those that
  // actually touch the cells (i.e., which layer?).
  for (dbNet *net : block->getNets()) 
  {
    if( !net->isSpecial() )
    {
      continue;
    }
    if( !(net->getSigType() == dbSigType::POWER || net->getSigType() == dbSigType::GROUND) )
    {
      continue;
    }
    int pwr = (net->getSigType() == dbSigType::POWER) ? RowPower_VDD : RowPower_VSS;
    for(dbSWire* swire : net->getSWires())
    {
      if( swire->getWireType() != dbWireType::ROUTED )
      {
        continue;
      }

      for(dbSBox* sbox : swire->getWires())
      {
        if( sbox->getDirection() != dbSBox::HORIZONTAL )
        {
          continue;
        }
        if( sbox->isVia() )
        {
          continue;
        }
        dbTechLayer* layer = sbox->getTechLayer();
        if( pwr == RowPower_VDD )
        {
          if( pwrLayers_.end() == pwrLayers_.find( layer ) )
          {
            continue;
          }
        }
        else if( pwr == RowPower_VSS )
        {
          if( gndLayers_.end() == gndLayers_.find( layer ) )
          {
            continue;
          }
        }

        Rect rect;
        sbox->getBox(rect);
        for( size_t r = 0; r < arch_->m_rows.size(); r++ )
        {
          double yb = arch_->m_rows[r]->getY();
          double yt = yb + arch_->m_rows[r]->getH();

          if( yb >= rect.yMin() && yb <= rect.yMax() )
          {
            arch_->m_rows[r]->m_powerBot = pwr;
          }
          if( yt >= rect.yMin() && yt <= rect.yMax() )
          {
            arch_->m_rows[r]->m_powerTop = pwr;
          }
        }
      }
    }
  }
}
////////////////////////////////////////////////////////////////
void
UWdp::createRouteGrid()
{
  logger_->report( "createRouteGrid() - not yet implemented, maybe not needed yet." );

  // Relates to pin blockages, etc. Maybe not needed right now.
  ;
}
////////////////////////////////////////////////////////////////
void
UWdp::setUpNdrRules()
{
  logger_->report( "setUpNdrRules - not yet implemented, maybe not needed yet." );

  // Relates to pin blockages, etc. Maybe not needed right now.
  ;
}
////////////////////////////////////////////////////////////////
void
UWdp::setUpPlacementRegions()
{
  logger_->report( "setUpPlacementRegions - implemented." );

  double xmin, xmax, ymin, ymax;
  int r = 0;
  xmin = arch_->getMinX();
  xmax = arch_->getMaxX();
  ymin = arch_->getMinY();
  ymax = arch_->getMaxY();

  dbBlock* block = db_->getChip()->getBlock();
  dbSet<dbRegion> regions = block->getRegions();

  std::unordered_map<odb::dbInst*, Node*>::iterator it_n;
  Architecture::Region* rptr = nullptr;

  // Default region.
  rptr = new Architecture::Region;
  rptr->m_rects.push_back( Rectangle( xmin, ymin, xmax, ymax ) );
  rptr->m_xmin = xmin;
  rptr->m_xmax = xmax;
  rptr->m_ymin = ymin;
  rptr->m_ymax = ymax;
  rptr->m_id   = arch_->m_regions.size();
  arch_->m_regions.push_back( rptr );
  
  // Hmm.  I noticed a comment in the OpenDP interface that
  // the OpenDB represents groups as regions.  I'll follow
  // the same approach and hope it is correct.
  // DEF GROUP => dbRegion with instances, no boundary, parent->region
  // DEF REGION => dbRegion no instances, boundary, parent = null
  auto db_regions = block->getRegions();
  for (auto db_region : db_regions) 
  {
    dbRegion *parent = db_region->getParent();
    if (parent) 
    {
      rptr = new Architecture::Region;
      rptr->m_id = arch_->m_regions.size();
      arch_->m_regions.push_back( rptr );
      
      // Assuming these are the rectangles making up the region...
      auto boundaries = db_region->getParent()->getBoundaries();
      for (dbBox *boundary : boundaries) 
      {
        Rect box;
        boundary->getBox(box);

        xmin = std::max( arch_->getMinX(), (double)box.xMin() );
        xmax = std::min( arch_->getMaxX(), (double)box.xMax() );
        ymin = std::max( arch_->getMinY(), (double)box.yMin() );
        ymax = std::min( arch_->getMaxY(), (double)box.yMax() );

        rptr->m_rects.push_back( Rectangle( xmin, ymin, xmax, ymax ) );
        rptr->m_xmin = std::min( xmin, rptr->m_xmin );
        rptr->m_xmax = std::max( xmax, rptr->m_xmax );
        rptr->m_ymin = std::min( ymin, rptr->m_ymin );
        rptr->m_ymax = std::max( ymax, rptr->m_ymax );
      }

      // The instances within this region.
      for (auto db_inst : db_region->getRegionInsts()) 
      {
        it_n = instMap_.find( db_inst );
        if( instMap_.end() != it_n )
        {
            Node* nd = it_n->second;
            if( nd->getRegionId() == 0 )
            {
              nd->setRegionId( rptr->m_id );
            }
        }
        // else error?
      }
    }
  }
  // Compute counts of the number of nodes in each region.
  arch_->m_numNodesInRegion.resize( arch_->m_regions.size() );
  std::fill( arch_->m_numNodesInRegion.begin(), arch_->m_numNodesInRegion.end(), 0 );
  for( size_t i = 0; i < nw_->m_nodes.size(); i++ )
  {
    Node* ndi = &(nw_->m_nodes[i]);
    int regId = ndi->getRegionId();
    if( regId >= arch_->m_numNodesInRegion.size() )
    {
      continue; // error.
    }
    ++arch_->m_numNodesInRegion[regId];
  }
  
  logger_->info( AAK, 101, "regions {}", arch_->m_regions.size() );
  for( size_t r = 0; r < arch_->m_regions.size(); r++ )
  {
    rptr = arch_->m_regions[r];
  //  std::cout << boost::format( "Region %d, %.2lf %.2lf %.2lf %.2lf" ) 
  //      % r % rptr->m_xmin % rptr->m_ymin % rptr->m_xmax % rptr->m_ymax << std::endl;
  //  for( size_t b = 0; b < rptr->m_rects.size(); b++ )
  //  {
  //    Rectangle& rect = rptr->m_rects[b];
  //    std::cout << boost::format( "  Rect %d, %.2lf %.2lf %.2lf %.2lf" ) 
  //        % b % rect.xmin() % rect.ymin() % rect.xmax() % rect.ymax() << std::endl;
  //  }
    logger_->info( AAK, 102, "region {}, instances is {}", 
        r, arch_->m_numNodesInRegion[r] );
  }
}






}  // namespace 

