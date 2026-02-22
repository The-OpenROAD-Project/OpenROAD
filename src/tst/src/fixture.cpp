// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "tst/fixture.h"

#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/geom.h"
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
#ifdef BAZEL_BUILD
  std::string error;
  runfiles_.reset(bazel::tools::cpp::runfiles::Runfiles::CreateForTest(&error));

  if (runfiles_ == nullptr) {
    throw std::runtime_error("Could not create Runfiles object: " + error);
  }
#endif

  std::call_once(init_sta_flag, []() { sta::initSta(); });

  db_ = utl::UniquePtrWithDeleter<odb::dbDatabase>(odb::dbDatabase::create(),
                                                   odb::dbDatabase::destroy);
  db_->setLogger(&logger_);
  interp_ = utl::UniquePtrWithDeleter<Tcl_Interp>(Tcl_CreateInterp(),
                                                  Tcl_DeleteInterp);
  sta_ = std::make_unique<sta::dbSta>(interp_.get(), db_.get(), &logger_);
}

Fixture::~Fixture() = default;

// Takes a relative path and removes leading directories until a valid path is
// found. This is needed by cmake to find a path from the fuller one used by
// bazel.
std::filesystem::path findValidPath(const std::filesystem::path& relative_path)
{
  std::deque<std::filesystem::path> path_components;

  for (const auto& component : relative_path) {
    path_components.push_back(component);
  }

  while (!path_components.empty()) {
    std::filesystem::path current_path;
    for (const auto& part : path_components) {
      current_path /= part;
    }

    if (std::filesystem::exists(current_path)) {
      return current_path;
    }
    path_components.pop_front();
  }

  throw std::runtime_error("Could not find: " + std::string(relative_path));
}

std::string Fixture::getFilePath(const std::string& file_path) const
{
#ifdef BAZEL_BUILD
  return runfiles_->Rlocation(file_path);
#else
  return std::filesystem::canonical(findValidPath(file_path));
#endif
}

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

odb::dbTech* Fixture::loadTechLef(const char* name, const std::string& lef_file)
{
  odb::lefin lef_reader(
      db_.get(), &logger_, /*ignore_non_routing_layers=*/false);
  auto path = getFilePath(lef_file);
  return lef_reader.createTech(name, path.c_str());
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

odb::dbInst* Fixture::makeInst(odb::dbBlock* block,
                               odb::dbMaster* master,
                               const char* name,
                               const InstOptions& options)
{
  odb::dbInst* inst = odb::dbInst::create(block,
                                          master,
                                          name,
                                          options.region,
                                          options.physical_only,
                                          options.parent_module);
  inst->setSourceType(options.type);
  inst->setLocation(options.location.x(), options.location.y());
  inst->setPlacementStatus(options.status);

  for (const InstOptions::ITermInfo& info : options.iterms) {
    odb::dbMTerm* mterm = master->findMTerm(info.term_name);
    EXPECT_NE(mterm, nullptr);
    odb::dbITerm* iterm = inst->getITerm(mterm);
    odb::dbNet* net = block->findNet(info.net_name);
    if (!net) {
      net = odb::dbNet::create(block, info.net_name);
    }
    iterm->connect(net);
  }

  return inst;
}

odb::dbBTerm* Fixture::makeBTerm(odb::dbBlock* block,
                                 const char* name,
                                 const BTermOptions& options)
{
  odb::dbNet* net = block->findNet(name);
  if (!net) {
    net = odb::dbNet::create(block, name);
  }

  odb::dbBTerm* bterm = odb::dbBTerm::create(net, name);
  bterm->setIoType(options.io_type);
  bterm->setSigType(options.sig_type);

  odb::dbTech* tech = block->getTech();
  for (const BTermOptions::BPinInfo& info : options.bpins) {
    odb::dbBPin* pin = odb::dbBPin::create(bterm);
    const odb::Rect rect = info.rect;
    odb::dbTechLayer* layer = tech->findLayer(info.layer_name);
    EXPECT_NE(layer, nullptr);
    odb::dbBox::create(
        pin, layer, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax());
    pin->setPlacementStatus(info.status);
  }

  return bterm;
}

}  // namespace tst
