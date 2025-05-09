// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <memory>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/lefin.h"
#include "utl/Logger.h"

// TODO: not needed after fully switching to bazel
#ifndef DATA_PREFIX
#define DATA_PREFIX ""
#endif

namespace odb {
class Nangate45TestFixture : public ::testing::Test
{
 protected:
  template <class T>
  using OdbUniquePtr = std::unique_ptr<T, void (*)(T*)>;

  void SetUp() override
  {
    db_ = OdbUniquePtr<odb::dbDatabase>(odb::dbDatabase::create(),
                                        &odb::dbDatabase::destroy);
    db_->setLogger(&logger_);
    odb::lefin lef_reader(
        db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
    lib_ = OdbUniquePtr<odb::dbLib>(
        lef_reader.createTechAndLib(
            "ng45", "ng45", DATA_PREFIX "Nangate45/Nangate45.lef"),
        &odb::dbLib::destroy);
  }

  utl::Logger logger_;
  OdbUniquePtr<odb::dbDatabase> db_{nullptr, &odb::dbDatabase::destroy};
  OdbUniquePtr<odb::dbLib> lib_{nullptr, &odb::dbLib::destroy};
  OdbUniquePtr<odb::dbChip> chip_{nullptr, &odb::dbChip::destroy};
  OdbUniquePtr<odb::dbBlock> block_{nullptr, &odb::dbBlock::destroy};
};
}  // namespace odb
