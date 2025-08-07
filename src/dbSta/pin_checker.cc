#include "dbSta/lef_checker/LefChecker.h"
#include "odb/db.h"

namespace ord {

LefChecker::LefChecker(odb::dbDatabase* db) : db_(db), verbose_(false) {}

LefChecker::CheckResult LefChecker::validateLib(odb::dbLib* lib) {
    CheckResult result;
    result.valid = true;
    
    auto grid_check = checkManufacturingGrid(lib);
    auto pin_check = checkPinAlignment(lib);
    
    result.valid = grid_check.valid && pin_check.valid;
    result.errors.insert(result.errors.end(), grid_check.errors.begin(), grid_check.errors.end());
    result.errors.insert(result.errors.end(), pin_check.errors.begin(), pin_check.errors.end());
    
    return result;
}

LefChecker::CheckResult LefChecker::checkManufacturingGrid(odb::dbLib* lib) {
    CheckResult result;
    double grid = getManufacturingGrid();
    
    for (auto* master : lib->getMasters()) {
        if (!isOnGrid(master->getWidth(), grid)) {
            result.errors.push_back("Master " + master->getName() + 
                                 " width not on manufacturing grid");
        }
        if (!isOnGrid(master->getHeight(), grid)) {
            result.errors.push_back("Master " + master->getName() + 
                                 " height not on manufacturing grid");
        }
    }
    
    result.valid = result.errors.empty();
    return result;
}

LefChecker::CheckResult LefChecker::checkPinAlignment(odb::dbLib* lib) {
    CheckResult result;
    double grid = getManufacturingGrid();
    
    for (auto* master : lib->getMasters()) {
        for (auto* term : master->getMTerms()) {
            for (auto* mpin : term->getMPins()) {
                for (auto* box : mpin->getGeometry()) {
                    if (!isOnGrid(box->xMin(), grid) || 
                        !isOnGrid(box->yMin(), grid)) {
                        result.errors.push_back("Pin " + term->getName() + 
                                              " in master " + master->getName() + 
                                              " not aligned to grid");
                    }
                }
            }
        }
    }
    
    result.valid = result.errors.empty();
    return result;
}

double LefChecker::getManufacturingGrid() const {
    return db_->getTech() ? db_->getTech()->getManufacturingGrid() : 0.001;
}

bool LefChecker::isOnGrid(double value, double grid) const {
    if (grid <= 0) return true;
    double remainder = fmod(value, grid);
    return remainder < 1e-6 || remainder > grid - 1e-6;
}

} // namespace ord
