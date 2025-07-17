#pragma once

namespace ord {
class dbSta;
}

namespace sta {
class dbNetwork;
}

namespace odb {
class dbTech;
}

namespace dbSta {

class LefChecker {
public:
  LefChecker(ord::dbSta* sta);
  bool isPinValid(int pin_dbu, int grid_dbu) const;

private:
  ord::dbSta* sta_;
  odb::dbTech* tech_;
};

} // namespace dbSta