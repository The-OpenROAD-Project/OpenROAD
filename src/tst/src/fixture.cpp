// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "tst/fixture.h"

#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "odb/lefin.h"
#include "sta/MinMax.hh"

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

std::string Fixture::getFilePath(const std::string& file_path)
{
#ifdef BAZEL_BUILD
  return runfiles_->Rlocation(file_path);
#else
  return std::filesystem::canonical(findValidPath(file_path));
#endif
}

sta::LibertyLibrary* Fixture::readLiberty(const std::string& filename,
                                          sta::Corner* corner,
                                          const sta::MinMaxAll* min_max,
                                          const bool infer_latches)
{
  if (!corner) {
    corner = sta_->findCorner("default");
  }
  auto path = getFilePath(filename);
  return sta_->readLiberty(path.c_str(), corner, min_max, infer_latches);
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

}  // namespace tst
