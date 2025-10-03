#include "pin_checker.h"
#include "odb/db.h"
#include <vector>

namespace ord {

// Existing LefChecker implementation (unchanged)
LefChecker::LefChecker(odb::dbDatabase* db) : db_(db), verbose_(false) {}

int LefChecker::getManufacturingGridInDbu() const {
    if (db_->getTech()) {
        return db_->getTech()->getManufacturingGrid();
    }
    return 1;
}

bool LefChecker::isOnGrid(int value_dbu, int grid_dbu) const {
    if (grid_dbu == 0) return true;
    return (value_dbu % grid_dbu) == 0;
}

LefChecker::CheckResult LefChecker::validateLib(odb::dbLib* lib) {
    CheckResult result{true, {}, {}};
    return result;
}

LefChecker::CheckResult LefChecker::checkManufacturingGrid(odb::dbLib* lib) {
    CheckResult result{true, {}, {}};
    const int grid_dbu = getManufacturingGridInDbu();
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
            
            int x_dbu = pin->getX();
            if (!isOnGrid(x_dbu, grid_dbu)) {
                result.errors.push_back("Pin " + pin->getName() + " X not on grid");
                result.valid = false;
            }
        }
    }
    return result;
}

// New Tcl interface wrappers
namespace {
LefChecker* getChecker() {
    static LefChecker checker(nullptr); // Initialize with actual db pointer if needed
    return &checker;
}
} // namespace

int isPinValid(const char* pin_name) {
    // Implementation note: Need db access for real implementation
    return 1; // Stub - replace with actual pin validation
}

int checkPinAlignment(const char* lib_name) {
    auto* db = getChecker()->getDb();
    if (auto lib = db->findLib(lib_name)) {
        return getChecker()->checkPinAlignment(lib).errors.size();
    }
    return -1; // Error: Library not found
}

int getManufacturingGrid() {
    return getChecker()->getManufacturingGridInDbu();
}

} // namespace ord
