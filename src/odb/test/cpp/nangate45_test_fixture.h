// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <memory>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/lefin.h"
#include "tst/fixture.h"
#include "utl/Logger.h"

namespace odb {

class Nangate45TestFixture : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    lib_ = loadTechAndLib(
        "ng45", "ng45", "_main/src/odb/test/Nangate45/Nangate45.lef");
  }

  odb::dbLib* lib_;
  odb::dbChip* chip_;
  odb::dbBlock* block_;
};

}  // namespace odb
