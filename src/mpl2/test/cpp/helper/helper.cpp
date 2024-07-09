#include "helper.h"

#include "odb/db.h"
#include "utl/Logger.h"

namespace mpl2 {

///
/// Create simple database with simple/default dbChip, dbTech, dbLib
///
odb::dbDatabase* createSimpleDB()
{
  utl::Logger* logger = new utl::Logger();
  odb::dbDatabase* db_ = odb::dbDatabase::create();
  db_->setLogger(logger);
  
  odb::dbChip* chip_ = odb::dbChip::create(db_);
  odb::dbTech* tech_ = odb::dbTech::create(db_, "tech");
  odb::dbLib* lib_ = odb::dbLib::create(db_, "lib", tech_, ',');
  odb::dbTechLayer::create(tech_, "L1", odb::dbTechLayerType::MASTERSLICE);
  return db_;
}

///
/// Create dbMaster with 1 input and 1 output pin configuration
///
odb::dbMaster* createSimpleMaster(
                          odb::dbLib* lib,
                          const char* name,
                          uint width,
                          uint height,
                          odb::dbMasterType type)
{
  odb::dbMaster* master = odb::dbMaster::create(lib, name);
  master->setWidth(width);
  master->setHeight(height);
  master->setType(type);

  odb::dbMTerm::create(
      master, "in", odb::dbIoType::INPUT, odb::dbSigType::SIGNAL);
  odb::dbMTerm::create(
      master, "out", odb::dbIoType::OUTPUT, odb::dbSigType::SIGNAL);

  master->setFrozen();
  return master;
}

}  // namespace mpl2