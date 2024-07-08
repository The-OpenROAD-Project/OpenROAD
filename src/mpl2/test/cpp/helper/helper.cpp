#include "helper.h"

#include "odb/db.h"
#include "utl/Logger.h"

namespace odb {

dbDatabase* createSimpleDB()
{
  utl::Logger* logger = new utl::Logger();
  dbDatabase* db_ = dbDatabase::create();
  db_->setLogger(logger);
  
  dbChip* chip_ = dbChip::create(db_);
  dbTech* tech_ = dbTech::create(db_, "tech");
  dbLib* lib_ = dbLib::create(db_, "lib", tech_, ',');
  dbTechLayer::create(tech_, "L1", dbTechLayerType::MASTERSLICE);
  return db_;
}

}  // namespace odb