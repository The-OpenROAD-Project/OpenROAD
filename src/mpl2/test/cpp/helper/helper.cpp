#include "helper.h"

#include "odb/db.h"
#include "utl/Logger.h"

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
                                 odb::dbIoType io_type,
                                 odb::dbSigType sig_type,
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

}  // namespace mpl2