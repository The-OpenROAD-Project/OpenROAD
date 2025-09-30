// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "tst/fixture.h"

#include <filesystem>

#include "odb/lefin.h"

namespace tst {

namespace {
std::once_flag init_sta_flag;
}

Fixture::Fixture()
{
  std::call_once(init_sta_flag, []() { sta::initSta(); });

  db_ = utl::UniquePtrWithDeleter<odb::dbDatabase>(odb::dbDatabase::create(),
                                                   odb::dbDatabase::destroy);
  db_->setLogger(&logger_);
  interp_ = utl::UniquePtrWithDeleter<Tcl_Interp>(Tcl_CreateInterp(),
                                                  Tcl_DeleteInterp);
  sta_ = std::make_unique<sta::dbSta>(interp_.get(), db_.get(), &logger_);
}

Fixture::~Fixture() = default;

sta::LibertyLibrary* Fixture::readLiberty(const char* filename,
                                          sta::Corner* corner,
                                          const sta::MinMaxAll* min_max,
                                          const bool infer_latches)
{
  if (!corner) {
    corner = sta_->findCorner("default");
  }
  auto path = std::filesystem::canonical(filename);
  return sta_->readLiberty(path.c_str(), corner, min_max, infer_latches);
}

odb::dbTech* Fixture::loadTechLef(const char* name, const char* lef_file)
{
  odb::lefin lef_reader(
      db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
  auto path = std::filesystem::canonical(lef_file);
  return lef_reader.createTech(name, path.c_str());
}

odb::dbLib* Fixture::loadLibaryLef(odb::dbTech* tech,
                                   const char* name,
                                   const char* lef_file)
{
  odb::lefin lef_reader(
      db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
  auto path = std::filesystem::canonical(lef_file);
  odb::dbLib* lib = lef_reader.createLib(tech, name, path.c_str());
  sta_->postReadLef(/*tech=*/nullptr, lib);
  return lib;
}

odb::dbLib* Fixture::loadTechAndLib(const char* tech_name,
                                    const char* lib_name,
                                    const char* lef_file)
{
  odb::lefin lef_reader(
      db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
  auto path = std::filesystem::canonical(lef_file);
  odb::dbLib* lib
      = lef_reader.createTechAndLib(tech_name, lib_name, path.c_str());
  sta_->postReadLef(/*tech=*/nullptr, lib);
  return lib;
}

}  // namespace tst
