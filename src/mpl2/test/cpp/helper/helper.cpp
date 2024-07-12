#include "helper.h"

#include "odb/db.h"
#include "utl/Logger.h"

#include "../../../src/hier_rtlmp.h"
#include "mpl2/rtl_mp.h"

namespace mpl2 {

///
/// Create simple database with simple/default dbChip, dbTech, dbLib
/// and layer of type dbTechLayerType::MASTERSLICE
///
odb::dbDatabase* createSimpleDB()
{
  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db_ = odb::dbDatabase::create();
  db_->setLogger(logger);

  odb::dbChip::create(db_);
  odb::dbTech* tech_ = odb::dbTech::create(db_, "tech");
  odb::dbLib::create(db_, "lib", tech_, ',');
  odb::dbTechLayer::create(tech_, "L1", odb::dbTechLayerType::MASTERSLICE);
  odb::dbTechLayer::create(tech_, "L2", odb::dbTechLayerType::MASTERSLICE);
  return db_;
}

///
/// Create dbMaster with 1 input and 1 output pin configuration
///
odb::dbMaster* createSimpleMaster(odb::dbLib* lib,
                                  const char* name,
                                  uint width,
                                  uint height,
                                  const odb::dbMasterType& type,
                                  odb::dbTechLayer* layer)
{
  odb::dbMaster* master = odb::dbMaster::create(lib, name);
  master->setWidth(width);
  master->setHeight(height);
  master->setType(type);
  return master;
}

odb::dbMPin* createMPinWithMTerm(odb::dbMaster* master,
                                 const char* mterm_name,
                                 const odb::dbIoType& io_type,
                                 const odb::dbSigType& sig_type,
                                 odb::dbTechLayer* layer,
                                 odb::Rect mpin_position)
{
  odb::dbMTerm* mterm_
      = odb::dbMTerm::create(master, mterm_name, io_type, sig_type);
  odb::dbMPin* mpin_ = odb::dbMPin::create(mterm_);
  odb::dbBox::create(mpin_,
                     layer,
                     mpin_position.xMin(),
                     mpin_position.yMin(),
                     mpin_position.xMax(),
                     mpin_position.yMax());

  return mpin_;
}

///
/// Create track-grid with same origin, line_count, and step for both X and Y
/// grid patterns
///
odb::dbTrackGrid* createSimpleTrack(odb::dbBlock* block,
                                    odb::dbTechLayer* layer,
                                    int origin,
                                    int line_count,
                                    int step,
                                    int manufacturing_grid)
{
  odb::dbTrackGrid* track = odb::dbTrackGrid::create(block, layer);

  odb::dbGCellGrid::create(block);

  track->addGridPatternX(origin, line_count, step);
  track->addGridPatternY(origin, line_count, step);

  block->getTech()->setManufacturingGrid(manufacturing_grid);

  return track;
}

/*
  odb::dbInst* inst1 = odb::dbInst::create(block, master, "leaf_macro1");
  HardMacro* macro1 = new HardMacro(inst1, 1, 1);
  hard_macros.push_back(macro1);
  cluster->addLeafMacro(inst1);
  metrics->addMetrics(
      Metrics(0,
              1,
              0.0,
              block->dbuToMicrons(inst1->getBBox()->getBox().dx())
                  * block->dbuToMicrons(inst1->getBBox()->getBox().dy())));
*/

std::map<HardMacro*, Metrics*> createMacroWithMetrics(odb::dbMaster* master,
                                                      odb::dbBlock* block,
                                                      const char* name,
                                                      bool isHardMacro, // if false, SoftMacro
                                                      float halo_width,
                                                      float halo_height
                                                      ) {
  std::map<HardMacro*, Metrics*> macro_info;

  odb::dbInst* inst = odb::dbInst::create(block, master, name);
  //if (isHardMacro) {
    HardMacro* macro = new HardMacro(inst, halo_width, halo_height);
  //}
  
  macro_info[macro] = new Metrics(0,
                                  1,
                                  0.0,
                                  block->dbuToMicrons(inst->getBBox()->getBox().dx())
                                      * block->dbuToMicrons(inst->getBBox()->getBox().dy()));

  return macro_info;
}

}  // namespace mpl2