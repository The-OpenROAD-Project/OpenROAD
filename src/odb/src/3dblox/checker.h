#pragma once

#include "odb/3dblox.h"

namespace odb {
class dbChip;
class dbMarkerCategory;
class Checker
{
 public:
  Checker(utl::Logger* logger);
  ~Checker() = default;
  void check(odb::dbChip* chip);

 private:
  void checkFloatingChips(odb::dbChip* chip, odb::dbMarkerCategory* category);
  void checkOverlappingChips(odb::dbChip* chip,
                             odb::dbMarkerCategory* category);
  utl::Logger* logger_ = nullptr;
};
}  // namespace odb