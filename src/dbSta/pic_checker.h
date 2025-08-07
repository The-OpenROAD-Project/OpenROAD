#pragma once
#include "odb/db.h"
#include <vector>
#include <string>

namespace ord {

class LefChecker {
public:
    struct CheckResult {
        bool valid;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
    };

    explicit LefChecker(odb::dbDatabase* db);
    
    CheckResult validateLib(odb::dbLib* lib);
    CheckResult checkManufacturingGrid(odb::dbLib* lib);
    CheckResult checkPinAlignment(odb::dbLib* lib);
    
    void setVerbose(bool verbose) { verbose_ = verbose; }

private:
    odb::dbDatabase* db_;
    bool verbose_;
    
    double getManufacturingGrid() const;
    bool isOnGrid(double value, double grid) const;
};

} // namespace ord
