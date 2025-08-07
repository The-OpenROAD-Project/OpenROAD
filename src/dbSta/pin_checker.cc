#include "pin_checker.h"
#include "odb/db.h"

namespace ord {

LefChecker::LefChecker(odb::dbDatabase* db) : db_(db), verbose_(false) {}

int LefChecker::getManufacturingGridInDbu() const {
    if (db_->getTech()) {
        return db_->getTech()->getManufacturingGrid(); // Returns DBU directly
    }
    return 1; // Default 1 DBU if no tech
}

bool LefChecker::isOnGrid(int value_dbu, int grid_dbu) const {
    if (grid_dbu == 0) return true; // Avoid division by zero
    return (value_dbu % grid_dbu) == 0; // Integer modulus operation
}

LefChecker::CheckResult LefChecker::validateLib(odb::dbLib* lib) {
    CheckResult result{true, {}, {}};
    // Add validation logic using integer DBU
    return result;
}

LefChecker::CheckResult LefChecker::checkManufacturingGrid(odb::dbLib* lib) {
    CheckResult result{true, {}, {}};
    const int grid_dbu = getManufacturingGridInDbu();
    // Implementation using grid_dbu
    return result;
}

LefChecker::CheckResult LefChecker::checkPinAlignment(odb::dbLib* lib) {
    CheckResult result{true, {}, {}};
    const int grid_dbu = getManufacturingGridInDbu();
    
    for (auto* master : lib->getMasters()) {
        for (auto* pin : master->getMTerms()) {
            if (pin->getSigType() == odb::dbSigType::POWER ||
                pin->getSigType() == odb::dbSigType::GROUND) {
                continue;
            }
            
            // Example pin check using DBU
            int x_dbu = pin->getX(); // Already in DBU
            if (!isOnGrid(x_dbu, grid_dbu)) {
                result.errors.push_back("Pin " + pin->getName() + " X not on grid");
                result.valid = false;
            }
        }
    }
    return result;
}

} // namespace ord
