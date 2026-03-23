// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

// Implements DescriptorRegistry::initDescriptors().
// This is in a separate file from descriptor_registry.cpp so that
// gui_descriptors (the lightweight CMake library used by web) does not
// pull in dbDescriptors / staDescriptors and their heavy dependencies.

#include <set>

#include "bufferTreeDescriptor.h"
#include "dbDescriptors.h"
#include "db_sta/dbSta.hh"
#include "gui/descriptor_registry.h"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/Liberty.hh"
#include "sta/Scene.hh"
#include "staDescriptors.h"

namespace gui {

// Defined here (rather than in descriptor_registry.cpp) because the "Zoom to"
// action references Gui::get() / Gui::zoomTo(), which are not available in the
// lightweight gui_descriptors CMake library.
Descriptor::Actions Selected::getActions() const
{
  auto actions = descriptor_->getActions(object_);

  odb::Rect bbox;
  if (getBBox(bbox)) {
    actions.push_back({"Zoom to", [this, bbox]() -> Selected {
                         auto gui = Gui::get();
                         if (gui) {
                           gui->zoomTo(bbox);
                         }
                         return *this;
                       }});
  }

  return actions;
}

void DescriptorRegistry::initDescriptors(odb::dbDatabase* db, sta::dbSta* sta)
{
  // Initialize BufferTree with STA so that DbNetDescriptor::getDBProperties()
  // can call BufferTree::isAggregate() safely.
  BufferTree::setSTA(sta);

  // Static empty sets for DbNetDescriptor — in the full GUI build,
  // MainWindow::init() re-registers with real widget-owned sets.
  static const std::set<odb::dbNet*> empty_net_set;

  registerDescriptor<odb::dbInst*>(new DbInstDescriptor(db, sta));
  registerDescriptor<odb::dbMaster*>(new DbMasterDescriptor(db, sta));
  registerDescriptor<odb::dbNet*>(new DbNetDescriptor(
      db, sta, empty_net_set, empty_net_set, empty_net_set));
  registerDescriptor<DbNetDescriptor::NetWithSink>(new DbNetDescriptor(
      db, sta, empty_net_set, empty_net_set, empty_net_set));
  registerDescriptor<odb::dbWire*>(new DbWireDescriptor(db));
  registerDescriptor<odb::dbSWire*>(new DbSWireDescriptor(db));
  registerDescriptor<odb::dbITerm*>(
      new DbITermDescriptor(db, []() { return false; }));
  registerDescriptor<odb::dbMTerm*>(
      new DbMTermDescriptor(db, []() { return false; }));
  registerDescriptor<odb::dbBTerm*>(new DbBTermDescriptor(db));
  registerDescriptor<odb::dbBPin*>(new DbBPinDescriptor(db));
  registerDescriptor<odb::dbVia*>(new DbViaDescriptor(db));
  registerDescriptor<odb::dbBlockage*>(new DbBlockageDescriptor(db));
  registerDescriptor<odb::dbObstruction*>(new DbObstructionDescriptor(db));
  registerDescriptor<odb::dbTechLayer*>(new DbTechLayerDescriptor(db));
  registerDescriptor<DbTermAccessPoint>(new DbTermAccessPointDescriptor(db));
  registerDescriptor<odb::dbGroup*>(new DbGroupDescriptor(db));
  registerDescriptor<odb::dbRegion*>(new DbRegionDescriptor(db));
  registerDescriptor<odb::dbModule*>(new DbModuleDescriptor(db));
  registerDescriptor<odb::dbModBTerm*>(new DbModBTermDescriptor(db));
  registerDescriptor<odb::dbModITerm*>(new DbModITermDescriptor(db));
  registerDescriptor<odb::dbModInst*>(new DbModInstDescriptor(db));
  registerDescriptor<odb::dbModNet*>(new DbModNetDescriptor(db));
  registerDescriptor<odb::dbTechVia*>(new DbTechViaDescriptor(db));
  registerDescriptor<odb::dbTechViaRule*>(new DbTechViaRuleDescriptor(db));
  registerDescriptor<odb::dbTechViaLayerRule*>(
      new DbTechViaLayerRuleDescriptor(db));
  registerDescriptor<odb::dbTechViaGenerateRule*>(
      new DbGenerateViaDescriptor(db));
  registerDescriptor<odb::dbTechNonDefaultRule*>(
      new DbNonDefaultRuleDescriptor(db));
  registerDescriptor<odb::dbTechLayerRule*>(new DbTechLayerRuleDescriptor(db));
  registerDescriptor<odb::dbTechSameNetRule*>(
      new DbTechSameNetRuleDescriptor(db));
  registerDescriptor<odb::dbSite*>(new DbSiteDescriptor(db));
  registerDescriptor<DbSiteDescriptor::SpecificSite>(new DbSiteDescriptor(db));
  registerDescriptor<odb::dbRow*>(new DbRowDescriptor(db));
  registerDescriptor<odb::dbBlock*>(new DbBlockDescriptor(db));
  registerDescriptor<odb::dbTech*>(new DbTechDescriptor(db));
  registerDescriptor<odb::dbMetalWidthViaMap*>(
      new DbMetalWidthViaMapDescriptor(db));
  registerDescriptor<odb::dbMarkerCategory*>(
      new DbMarkerCategoryDescriptor(db));
  registerDescriptor<odb::dbMarker*>(new DbMarkerDescriptor(db));
  registerDescriptor<odb::dbScanInst*>(new DbScanInstDescriptor(db));
  registerDescriptor<odb::dbScanList*>(new DbScanListDescriptor(db));
  registerDescriptor<odb::dbScanPartition*>(new DbScanPartitionDescriptor(db));
  registerDescriptor<odb::dbScanChain*>(new DbScanChainDescriptor(db));
  registerDescriptor<odb::dbBox*>(new DbBoxDescriptor(db));
  registerDescriptor<odb::dbSBox*>(new DbSBoxDescriptor(db));
  registerDescriptor<DbBoxDescriptor::BoxWithTransform>(
      new DbBoxDescriptor(db));
  registerDescriptor<odb::dbMasterEdgeType*>(
      new DbMasterEdgeTypeDescriptor(db));
  registerDescriptor<odb::dbCellEdgeSpacing*>(
      new DbCellEdgeSpacingDescriptor(db));

  // STA descriptors
  registerDescriptor<sta::Scene*>(new SceneDescriptor(sta));
  registerDescriptor<sta::LibertyLibrary*>(new LibertyLibraryDescriptor(sta));
  registerDescriptor<sta::LibertyCell*>(new LibertyCellDescriptor(sta));
  registerDescriptor<sta::LibertyPort*>(new LibertyPortDescriptor(sta));
  registerDescriptor<sta::Instance*>(new StaInstanceDescriptor(sta));
  registerDescriptor<sta::Clock*>(new ClockDescriptor(sta));

  // Note: RulerDescriptor, LabelDescriptor, and BufferTreeDescriptor are
  // GUI-only and are registered in MainWindow::init().
}

}  // namespace gui
