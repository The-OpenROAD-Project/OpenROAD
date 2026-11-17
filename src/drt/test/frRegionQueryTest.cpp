// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "db/obj/frBTerm.h"
#include "db/obj/frGuide.h"
#include "db/obj/frInst.h"
#include "db/obj/frInstTerm.h"
#include "db/obj/frMaster.h"
#include "db/obj/frNet.h"
#include "db/obj/frShape.h"
#include "fixture.h"
#include "frRegionQuery.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace drt {

struct RegionQueryFixture : public Fixture
{
  frGuide* makeGuide(frNet* net,
                     frLayerNum layer_num,
                     const odb::Point& begin,
                     const odb::Point& end)
  {
    auto guide = std::make_unique<frGuide>(begin, layer_num, end);
    auto* ptr = guide.get();
    net->addGuide(std::move(guide));
    return ptr;
  }

  void initGuideQuery()
  {
    initRegionQuery();
    design->getRegionQuery()->initGuide();
  }

  void initOrigGuideQuery(frOrderedIdMap<frNet*, std::vector<frRect>>& guides)
  {
    initRegionQuery();
    design->getRegionQuery()->initOrigGuide(guides);
  }
};

TEST_F(RegionQueryFixture, guide_query_uses_guide_id_order)
{
  // Exceed one R-tree leaf, with insertion/spatial order opposite to IDs.
  constexpr int num_guides = 17;
  std::array<frGuide*, num_guides> guides;
  frNet* net = makeNet("n1");
  for (int i = 0; i < num_guides; i++) {
    const int id = num_guides - i - 1;
    const int x = i * 1000;
    guides[id] = makeGuide(net, /*layer_num=*/2, {x, 500}, {x + 500, 500});
    guides[id]->setId(id);
  }

  initGuideQuery();
  const odb::Rect query_box(0, 0, num_guides * 1000, 2000);
  const odb::Rect prefix_box(-1000, -1000, -500, -500);
  auto* query = design->getRegionQuery();

  // Existing results deliberately do not belong at the start of a full sort.
  frRegionQuery::Objects<frGuide> result{{prefix_box, guides.back()}};
  query->queryGuide(query_box, /*layerNum=*/2, result);

  ASSERT_EQ(result.size(), num_guides + 1);
  EXPECT_EQ(result.front().first, prefix_box);
  EXPECT_EQ(result.front().second, guides.back());
  for (int i = 0; i < num_guides; i++) {
    EXPECT_EQ(result[i + 1].second, guides[i]);
  }

  const std::vector<frGuide*> prefix{guides.back(), guides[num_guides - 2]};
  auto expected = prefix;
  expected.insert(expected.end(), guides.begin(), guides.end());
  auto layer_result = prefix;
  query->queryGuide(query_box, /*layerNum=*/2, layer_result);
  EXPECT_EQ(layer_result, expected);

  auto all_layers_result = prefix;
  query->queryGuide(query_box, all_layers_result);
  EXPECT_EQ(all_layers_result, expected);
}

TEST_F(RegionQueryFixture, guide_query_orders_across_layers)
{
  auto* tech = design->getTech();
  addLayer(tech, "v1", odb::dbTechLayerType::CUT);
  addLayer(
      tech, "m2", odb::dbTechLayerType::ROUTING, odb::dbTechLayerDir::VERTICAL);
  frNet* net = makeNet("n1");
  auto* lower = makeGuide(net, /*layer_num=*/2, {500, 500}, {1500, 500});
  auto* upper = makeGuide(net, /*layer_num=*/4, {500, 500}, {500, 1500});
  lower->setId(1);
  upper->setId(0);
  initGuideQuery();

  std::vector<frGuide*> result{lower};
  design->getRegionQuery()->queryGuide(odb::Rect(0, 0, 2000, 2000), result);
  const std::vector<frGuide*> expected{lower, upper, lower};
  EXPECT_EQ(result, expected);
}

TEST_F(RegionQueryFixture, original_guide_query_uses_canonical_order)
{
  constexpr int num_nets = 17;
  std::array<frNet*, num_nets> nets;
  frOrderedIdMap<frNet*, std::vector<frRect>> guides;
  for (int i = 0; i < num_nets; i++) {
    nets[i] = makeNet(("n" + std::to_string(i)).c_str());
    const int x = (num_nets - i - 1) * 1000;
    guides[nets[i]].emplace_back(
        x, 0, x + 1000, 1000, /*layer_num=*/2, nullptr);
  }
  guides[nets[0]].emplace_back(0, 1000, 1000, 2000, /*layer_num=*/2, nullptr);

  initOrigGuideQuery(guides);

  const odb::Rect prefix_box(-1000, -1000, -500, -500);
  frRegionQuery::Objects<frNet> result{{prefix_box, nets.back()}};
  design->getRegionQuery()->queryOrigGuide(
      odb::Rect(0, 0, num_nets * 1000, 2000), /*layerNum=*/2, result);

  ASSERT_EQ(result.size(), num_nets + 2);
  EXPECT_EQ(result.front().first, prefix_box);
  EXPECT_EQ(result.front().second, nets.back());
  EXPECT_EQ(result[1].second->getId(), 0);
  EXPECT_EQ(result[1].first, odb::Rect(0, 1000, 1000, 2000));
  EXPECT_EQ(result[2].second->getId(), 0);
  EXPECT_EQ(result[2].first, odb::Rect(16000, 0, 17000, 1000));
  for (int i = 1; i < num_nets; i++) {
    EXPECT_EQ(result[i + 2].second, nets[i]);
  }
}

TEST_F(RegionQueryFixture, gr_pin_query_uses_type_instance_and_terminal_order)
{
  auto [master, db_master] = makeMacro("cell", 0, 0, 1000, 1000);
  makeMacroPin(master, "a", 0, 0, 100, 100);
  makeMacroPin(master, "b", 200, 0, 300, 100);
  auto* db_block = db_master->getDb()->getChip()->getBlock();
  frInst first_inst(
      "u0", master, odb::dbInst::create(db_block, db_master, "u0"));
  frInst second_inst(
      "u1", master, odb::dbInst::create(db_block, db_master, "u1"));
  first_inst.setId(0);
  second_inst.setId(1);
  frInstTerm first_a(&first_inst, master->getTerms()[0].get());
  frInstTerm first_b(&first_inst, master->getTerms()[1].get());
  frInstTerm second_a(&second_inst, master->getTerms()[0].get());
  frInstTerm second_b(&second_inst, master->getTerms()[1].get());
  // Pin IDs oppose instance and master-terminal order.
  first_a.setId(3);
  first_b.setId(2);
  second_a.setId(1);
  second_b.setId(0);

  constexpr int num_bterms = 17;
  std::array<std::unique_ptr<frBTerm>, num_bterms> bterms;
  std::vector<frBlockObject*> ordered;
  for (int i = 0; i < num_bterms; i++) {
    bterms[i] = std::make_unique<frBTerm>("b" + std::to_string(i));
    bterms[i]->setId(100 + i);
    ordered.push_back(bterms[i].get());
  }
  ordered.insert(ordered.end(), {&first_a, &first_b, &second_a, &second_b});

  std::vector<std::pair<frBlockObject*, odb::Point>> pins;
  int x = 0;
  for (auto it = ordered.rbegin(); it != ordered.rend(); ++it) {
    pins.emplace_back(*it, odb::Point(x, 500));
    x += 1000;
  }
  initRegionQuery();
  auto* query = design->getRegionQuery();
  query->initGRPin(pins);

  std::vector<frBlockObject*> result{&second_b, &first_a};
  auto expected = result;
  expected.insert(expected.end(), ordered.begin(), ordered.end());
  query->queryGRPin(odb::Rect(0, 0, x, 1000), result);
  EXPECT_EQ(result, expected);
}

}  // namespace drt
