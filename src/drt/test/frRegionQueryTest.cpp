// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <array>
#include <memory>
#include <string>

#include "db/obj/frGuide.h"
#include "db/obj/frNet.h"
#include "db/obj/frShape.h"
#include "fixture.h"
#include "frRegionQuery.h"
#include "gtest/gtest.h"
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
  frNet* n1 = makeNet("n1");
  frNet* n2 = makeNet("n2");
  frGuide* g1 = makeGuide(n1, /*layer_num=*/2, {500, 500}, {1500, 500});
  frGuide* g2 = makeGuide(n2, /*layer_num=*/2, {500, 500}, {1500, 500});
  g1->setId(0);
  g2->setId(1);

  initGuideQuery();

  frRegionQuery::Objects<frGuide> result;
  design->getRegionQuery()->queryGuide(
      odb::Rect(0, 0, 2000, 2000), /*layerNum=*/2, result);

  ASSERT_EQ(result.size(), 2);
  EXPECT_LT(result[0].second->getId(), result[1].second->getId());
  EXPECT_EQ(result[0].second->getNet()->getName(), "n1");
  EXPECT_EQ(result[1].second->getNet()->getName(), "n2");
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

  frRegionQuery::Objects<frNet> result;
  design->getRegionQuery()->queryOrigGuide(
      odb::Rect(0, 0, num_nets * 1000, 2000), /*layerNum=*/2, result);

  ASSERT_EQ(result.size(), num_nets + 1);
  EXPECT_EQ(result[0].second->getId(), 0);
  EXPECT_EQ(result[0].first, odb::Rect(0, 1000, 1000, 2000));
  EXPECT_EQ(result[1].second->getId(), 0);
  EXPECT_EQ(result[1].first, odb::Rect(16000, 0, 17000, 1000));
  for (int i = 1; i < num_nets; i++) {
    EXPECT_EQ(result[i + 1].second->getId(), i);
    EXPECT_EQ(result[i + 1].second->getName(), "n" + std::to_string(i));
  }
}

}  // namespace drt
