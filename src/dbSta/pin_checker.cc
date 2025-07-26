#include "pin_checker.h"
#include "odb/db.h"
#include "dbSta/dbSta.hh"
#include "dbNetwork.hh"

namespace dbSta {

LefChecker::LefChecker(ord::dbSta* sta) : sta_(sta) {
  tech_ = sta_->getDbNetwork()->getDb()->getTech();
}

bool LefChecker::isPinValid(int pin_dbu, int grid_dbu) const {
  if (grid_dbu == 0) return false;  // Avoid division by zero
  return (pin_dbu % grid_dbu) == 0;
}

} // namespace dbSta
