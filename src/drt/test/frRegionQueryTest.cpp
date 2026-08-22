// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>

#include "db/obj/frGuide.h"
#include "db/obj/frNet.h"
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
};

TEST_F(RegionQueryFixture, guide_query_uses_guide_id_order)
{
  frNet* n1 = makeNet("n1");
  frNet* n2 = makeNet("n2");
  frGuide* g1
      = makeGuide(n1, /*layer_num=*/2, {500, 500}, {1500, 500});
  frGuide* g2
      = makeGuide(n2, /*layer_num=*/2, {500, 500}, {1500, 500});
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

}  // namespace drt
