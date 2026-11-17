// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Metamorphic tests: global placement of gcd, compared with placement of
// the same design changed in a way whose effect is known. gpl is not exactly
// symmetric (floating-point sums change with order and magnitude), so
// results compare within a tolerance.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <functional>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "gpl/Replace.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "odb/geom.h"
#include "odb/lefin.h"
#include "tst/fixture.h"
#include "utl/deleter.h"

namespace gpl {
namespace {

// The changed designs' HPWLs differ from the original's by 0.012-0.034%. A
// density force 1.5x stronger to the right moves the mirrored HPWL by 0.7%.
constexpr double kHpwlTolerance = 0.002;

class MetamorphicTest : public tst::Fixture
{
 protected:
  static constexpr const char* kLef = "_main/src/gpl/test/nangate45.lef";

  // Loads gcd into the fixture's database.
  void loadGcd()
  {
    loadTechAndLib("tech", "lib", kLef);
    block_ = readDef(gcdPath());
    recordInput();
  }

  // Loads def into db, a database other than the fixture's, without
  // telling STA about it.
  void loadInto(odb::dbDatabase* db, const std::string& def)
  {
    odb::lefin lef_reader(db, &logger_, /*ignore_non_routing_layers=*/false);
    odb::dbLib* lib
        = lef_reader.createTechAndLib("tech", "lib", getFilePath(kLef).c_str());
    ASSERT_NE(lib, nullptr);
    odb::dbChip* chip = odb::dbChip::create(db, lib->getTech());
    std::vector<odb::dbLib*> libs{lib};
    odb::defin reader(db, &logger_);
    ASSERT_TRUE(reader.readChip(libs, def.c_str(), chip));
    block_ = chip->getBlock();
    recordInput();
  }

  void recordInput()
  {
    input_.clear();
    for (odb::dbInst* inst : block_->getInsts()) {
      input_.emplace_back(inst, inst->getLocation());
    }
  }

  std::string gcdPath() const
  {
    return getFilePath("_main/src/gpl/test/gcd.def");
  }

  PlaceResult place(odb::dbDatabase* db = nullptr)
  {
    Replace replace{
        db ? db : db_.get(), sta_.get(), nullptr, nullptr, nullptr, &logger_};
    return replace.doPlace(1, PlaceOptions{});
  }

  std::map<std::string, odb::Point> centers() const
  {
    std::map<std::string, odb::Point> result;
    for (odb::dbInst* inst : block_->getInsts()) {
      result[inst->getName()] = inst->getBBox()->getBox().center();
    }
    return result;
  }

  // Puts every cell back where the input had it, unplaced, moved by shift.
  void restoreInput(const odb::Point shift = {0, 0})
  {
    for (const auto& [inst, location] : input_) {
      inst->setLocation(location.x() + shift.x(), location.y() + shift.y());
      inst->setPlacementStatus(odb::dbPlacementStatus::NONE);
    }
  }

  // Replaces every IO pin shape with move(shape).
  void moveIoPins(const std::function<odb::Rect(odb::Rect)>& move)
  {
    for (odb::dbBTerm* bterm : block_->getBTerms()) {
      for (odb::dbBPin* pin : bterm->getBPins()) {
        std::vector<std::pair<odb::dbTechLayer*, odb::Rect>> shapes;
        for (odb::dbBox* box : pin->getBoxes()) {
          shapes.emplace_back(box->getTechLayer(), box->getBox());
        }
        for (auto it = pin->getBoxes().begin(); it != pin->getBoxes().end();) {
          odb::dbBox* box = *it;
          ++it;
          odb::dbBox::destroy(box);
        }
        for (const auto& [layer, shape] : shapes) {
          const odb::Rect r = move(shape);
          odb::dbBox::create(
              pin, layer, r.xMin(), r.yMin(), r.xMax(), r.yMax());
        }
      }
    }
  }

  odb::dbBlock* block_ = nullptr;
  std::vector<std::pair<odb::dbInst*, odb::Point>> input_;
};

// Mirroring the IO pins left to right gives a nearly mirrored problem: the
// cells' pin offsets are not mirrored, so the match is measured, not exact.
// The HPWL matches to within the tolerance. The cells are not each mirrored:
// gcd has interchangeable cells, and a few land on the other side.
TEST_F(MetamorphicTest, MirroredPinsGiveTheSameHpwl)
{
  loadGcd();
  const PlaceResult original = place();

  restoreInput();
  const odb::Rect die = block_->getDieArea();
  const int sum = die.xMin() + die.xMax();
  moveIoPins([sum](const odb::Rect r) {
    return odb::Rect(sum - r.xMax(), r.yMin(), sum - r.xMin(), r.yMax());
  });
  const PlaceResult mirrored = place();

  EXPECT_NEAR(mirrored.hpwl, original.hpwl, kHpwlTolerance * original.hpwl);
}

// The order instances are listed in does not change the placement's
// quality.
TEST_F(MetamorphicTest, InstanceOrderDoesNotChangeHpwl)
{
  loadGcd();
  const std::string first_inst = (*block_->getInsts().begin())->getName();
  const PlaceResult original = place();

  // gcd.def with its COMPONENTS entries shuffled.
  std::ifstream in(gcdPath());
  std::stringstream buffer;
  buffer << in.rdbuf();
  const std::string text = buffer.str();
  const std::size_t first = text.find("\n- ", text.find("\nCOMPONENTS"));
  const std::size_t end = text.find("\nEND COMPONENTS");
  ASSERT_NE(first, std::string::npos);
  ASSERT_NE(end, std::string::npos);
  std::vector<std::string> entries;
  for (std::size_t start = first; start < end;) {
    const std::size_t next = std::min(text.find("\n- ", start + 1), end);
    entries.push_back(text.substr(start, next - start));
    start = next;
  }
  std::mt19937 rng(1);
  std::ranges::shuffle(entries, rng);
  std::string shuffled = text.substr(0, first);
  for (const std::string& entry : entries) {
    shuffled += entry;
  }
  shuffled += text.substr(end);
  const std::string path = testing::TempDir() + "/gcd_shuffled.def";
  std::ofstream(path) << shuffled;

  // A second database. The fixture's STA does not see it, which is safe
  // only because PlaceOptions{} turns off timing-driven, power-driven and
  // virtual CTS modes, the ones that read the netlist through STA.
  utl::UniquePtrWithDeleter<odb::dbDatabase> db(odb::dbDatabase::create(),
                                                odb::dbDatabase::destroy);
  db->setLogger(&logger_);
  ASSERT_NO_FATAL_FAILURE(loadInto(db.get(), path));
  ASSERT_NE((*block_->getInsts().begin())->getName(), first_inst);
  const PlaceResult permuted = place(db.get());

  EXPECT_NEAR(permuted.hpwl, original.hpwl, kHpwlTolerance * original.hpwl);
}

// Moving the whole design moves every cell by the same offset, up to
// rounding.
TEST_F(MetamorphicTest, TranslatedDesignPlacesTheSame)
{
  loadGcd();
  const PlaceResult original = place();
  const std::map<std::string, odb::Point> placed = centers();

  const odb::Point shift(1000000, 2000000);
  restoreInput(shift);
  odb::Rect die = block_->getDieArea();
  die.moveDelta(shift.x(), shift.y());
  block_->setDieArea(die);
  const std::vector<odb::dbRow*> rows(block_->getRows().begin(),
                                      block_->getRows().end());
  for (odb::dbRow* row : rows) {
    const std::string name = row->getName();
    odb::dbSite* site = row->getSite();
    const odb::Point origin = row->getOrigin();
    const odb::dbOrientType orient = row->getOrient();
    const odb::dbRowDir direction = row->getDirection();
    const int sites = row->getSiteCount();
    const int spacing = row->getSpacing();
    odb::dbRow::destroy(row);
    odb::dbRow::create(block_,
                       name.c_str(),
                       site,
                       origin.x() + shift.x(),
                       origin.y() + shift.y(),
                       orient,
                       direction,
                       sites,
                       spacing);
  }
  block_->setCoreArea(block_->computeCoreArea());
  moveIoPins([shift](odb::Rect r) {
    r.moveDelta(shift.x(), shift.y());
    return r;
  });
  const PlaceResult translated = place();

  EXPECT_NEAR(translated.hpwl, original.hpwl, kHpwlTolerance * original.hpwl);
  // Rounding at the larger coordinates moves cells by about 50 DBU on
  // average and 1510 DBU at most, against a 61940 DBU core.
  double total = 0;
  double worst = 0;
  for (const auto& [name, center] : centers()) {
    const odb::Point expected = placed.at(name);
    const double distance = std::hypot(center.x() - shift.x() - expected.x(),
                                       center.y() - shift.y() - expected.y());
    total += distance;
    worst = std::max(worst, distance);
  }
  const int core_width = block_->getCoreArea().dx();
  EXPECT_LT(total / placed.size(), 0.005 * core_width);
  EXPECT_LT(worst, 0.05 * core_width);
}

}  // namespace
}  // namespace gpl
