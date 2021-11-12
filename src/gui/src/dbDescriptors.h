///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "gui/gui.h"

namespace odb {
class dbMaster;
} // namespace odb

namespace sta {
class dbSta;
} // namespace sta

namespace gui {

// Descriptor classes for OpenDB objects.  Eventually these should
// become part of the database code generation.

class DbInstDescriptor : public Descriptor
{
 public:
  DbInstDescriptor(odb::dbDatabase* db, sta::dbSta* sta);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  bool isInst(std::any object) const override;

  Properties getProperties(std::any object) const override;
  Actions getActions(std::any object) const override;
  Editors getEditors(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  void makeMasterOptions(odb::dbMaster* master, std::vector<EditorOption>& options) const;
  void makePlacementStatusOptions(std::vector<EditorOption>& options) const;
  void makeOrientationOptions(std::vector<EditorOption>& options) const;
  bool setNewLocation(odb::dbInst* inst, std::any value, bool is_x) const;

  odb::dbDatabase* db_;
  sta::dbSta* sta_;
};

class DbMasterDescriptor : public Descriptor
{
 public:
  DbMasterDescriptor(odb::dbDatabase* db, sta::dbSta* sta);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

  static void getMasterEquivalent(sta::dbSta* sta, odb::dbMaster* master, std::set<odb::dbMaster*>& masters);

 private:
  void getInstances(odb::dbMaster* master, std::set<odb::dbInst*>& insts) const;

  odb::dbDatabase* db_;
  sta::dbSta* sta_;
};

class DbNetDescriptor : public Descriptor
{
 public:
  DbNetDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  bool isNet(std::any object) const override;

  Properties getProperties(std::any object) const override;
  Editors getEditors(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;

  static const int max_iterms_ = 10000;
};

class DbITermDescriptor : public Descriptor
{
 public:
  DbITermDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbBTermDescriptor : public Descriptor
{
 public:
  DbBTermDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  Properties getProperties(std::any object) const override;
  Editors getEditors(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbBlockageDescriptor : public Descriptor
{
 public:
  DbBlockageDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  Properties getProperties(std::any object) const override;
  Editors getEditors(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbObstructionDescriptor : public Descriptor
{
 public:
  DbObstructionDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  Properties getProperties(std::any object) const override;
  Actions getActions(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbTechLayerDescriptor : public Descriptor
{
 public:
  DbTechLayerDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

};  // namespace gui
