#pragma once

#include <string>
#include <vector>

#include "odb/3dblox.h"
#include "odb/geom.h"
namespace odb {
class dbChip;
class dbMarkerCategory;
class dbChipInst;
struct UnfoldedChip
{
  std::string getName() const;
  std::vector<dbChipInst*> chip_inst_path;
  Cuboid cuboid;
};
class Checker
{
 public:
  Checker(utl::Logger* logger);
  ~Checker() = default;
  void check(odb::dbChip* chip);

 private:
  void checkFloatingChips(odb::dbMarkerCategory* category);
  void checkOverlappingChips(odb::dbMarkerCategory* category);
  void unfoldChip(odb::dbChipInst* chip_inst, UnfoldedChip unfolded_chip);
  utl::Logger* logger_ = nullptr;
  std::vector<UnfoldedChip> unfolded_chips_;
};
}  // namespace odb