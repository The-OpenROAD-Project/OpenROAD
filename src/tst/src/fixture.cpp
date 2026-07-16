// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "tst/fixture.h"

#include <memory>
#include <mutex>
#include <string>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/lefin.h"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Sta.hh"
#include "tcl.h"  // IWYU pragma: keep (clang-tidy, you're drunk)

namespace tst {

namespace {
std::once_flag init_sta_flag;
}

Fixture::Fixture()
{
  std::call_once(init_sta_flag, []() { sta::initSta(); });

  interp_ = utl::UniquePtrWithDeleter<Tcl_Interp>(Tcl_CreateInterp(),
                                                  Tcl_DeleteInterp);
  sta_ = std::make_unique<sta::dbSta>(interp_.get(), db_.get(), &logger_);
}

Fixture::~Fixture() = default;

sta::LibertyLibrary* Fixture::readLiberty(const std::string& filename,
                                          sta::Scene* scene,
                                          const sta::MinMaxAll* min_max,
                                          const bool infer_latches)
{
  if (!scene) {
    scene = sta_->findScene("default");
  }
  auto path = getFilePath(filename);
  return sta_->readLiberty(path.c_str(), scene, min_max, infer_latches);
}

odb::dbLib* Fixture::loadLibaryLef(odb::dbTech* tech,
                                   const char* name,
                                   const std::string& lef_file)
{
  odb::lefin lef_reader(
      db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
  auto path = getFilePath(lef_file);
  odb::dbLib* lib = lef_reader.createLib(tech, name, path.c_str());
  sta_->postReadLef(/*tech=*/nullptr, lib);
  return lib;
}

odb::dbLib* Fixture::loadTechAndLib(const char* tech_name,
                                    const char* lib_name,
                                    const std::string& lef_file)
{
  odb::lefin lef_reader(
      db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
  auto path = getFilePath(lef_file);
  odb::dbLib* lib
      = lef_reader.createTechAndLib(tech_name, lib_name, path.c_str());
  sta_->postReadLef(/*tech=*/nullptr, lib);
  return lib;
}

bool Fixture::updateLib(odb::dbLib* lib, const std::string& lef_file)
{
  odb::lefin lef_reader(
      db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
  auto path = getFilePath(lef_file);

  return lef_reader.updateLib(lib, path.c_str());
}

}  // namespace tst
