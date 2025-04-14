// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <functional>
#include <utility>

namespace odb {

class dbBlock;
class dbDatabase;
class dbLib;
class dbTech;

class dbDatabaseObserver
{
 public:
  virtual ~dbDatabaseObserver()
  {
    if (unregister_observer_) {
      unregister_observer_();
    }
  }

  // Either pointer could be null
  virtual void postReadLef(odb::dbTech* tech, odb::dbLib* library) = 0;
  virtual void postReadDef(odb::dbBlock* block) = 0;
  virtual void postReadDb(odb::dbDatabase* db) = 0;

  void setUnregisterObserver(std::function<void()> unregister_observer)
  {
    unregister_observer_ = std::move(unregister_observer);
  }

 private:
  std::function<void()> unregister_observer_;
};

}  // namespace odb
