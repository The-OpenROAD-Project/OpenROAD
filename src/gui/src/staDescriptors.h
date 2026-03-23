// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <any>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace sta {
class dbSta;
class Clock;
class Pin;
}  // namespace sta

namespace gui {

// Descriptor classes for sta Liberty objects.

class LibertyLibraryDescriptor : public Descriptor
{
 public:
  LibertyLibraryDescriptor(sta::dbSta* sta);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  Properties getProperties(const std::any& object) const override;
  Selected makeSelected(const std::any& object) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 private:
  sta::dbSta* sta_;
};

class LibertyCellDescriptor : public Descriptor
{
 public:
  LibertyCellDescriptor(sta::dbSta* sta);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  Properties getProperties(const std::any& object) const override;
  Selected makeSelected(const std::any& object) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 private:
  sta::dbSta* sta_;
};

class LibertyPortDescriptor : public Descriptor
{
 public:
  LibertyPortDescriptor(sta::dbSta* sta);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  Properties getProperties(const std::any& object) const override;
  Selected makeSelected(const std::any& object) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 private:
  sta::dbSta* sta_;
};

class SceneDescriptor : public Descriptor
{
 public:
  SceneDescriptor(sta::dbSta* sta);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  Properties getProperties(const std::any& object) const override;
  Selected makeSelected(const std::any& object) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 private:
  sta::dbSta* sta_;
};

class StaInstanceDescriptor : public Descriptor
{
 public:
  StaInstanceDescriptor(sta::dbSta* sta);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  Properties getProperties(const std::any& object) const override;
  Selected makeSelected(const std::any& object) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 private:
  sta::dbSta* sta_;

  static constexpr int kFloatPrecision = 2;
};

class ClockDescriptor : public Descriptor
{
 public:
  ClockDescriptor(sta::dbSta* sta);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  Properties getProperties(const std::any& object) const override;
  Selected makeSelected(const std::any& object) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 private:
  sta::dbSta* sta_;

  std::set<const sta::Pin*> getClockPins(sta::Clock* clock) const;
};

};  // namespace gui
