// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "gui/gui.h"

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

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  sta::dbSta* sta_;
};

class LibertyCellDescriptor : public Descriptor
{
 public:
  LibertyCellDescriptor(sta::dbSta* sta);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  sta::dbSta* sta_;
};

class LibertyPortDescriptor : public Descriptor
{
 public:
  LibertyPortDescriptor(sta::dbSta* sta);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  sta::dbSta* sta_;
};

class LibertyPgPortDescriptor : public Descriptor
{
 public:
  LibertyPgPortDescriptor(sta::dbSta* sta);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbMTerm* getMTerm(const std::any& object) const;

  sta::dbSta* sta_;
};

class CornerDescriptor : public Descriptor
{
 public:
  CornerDescriptor(sta::dbSta* sta);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  sta::dbSta* sta_;
};

class StaInstanceDescriptor : public Descriptor
{
 public:
  StaInstanceDescriptor(sta::dbSta* sta);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  sta::dbSta* sta_;

  static constexpr int float_precision_ = 2;
};

class ClockDescriptor : public Descriptor
{
 public:
  ClockDescriptor(sta::dbSta* sta);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  sta::dbSta* sta_;

  std::set<const sta::Pin*> getClockPins(sta::Clock* clock) const;
};

};  // namespace gui
