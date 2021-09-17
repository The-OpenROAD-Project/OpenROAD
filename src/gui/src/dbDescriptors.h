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
}

namespace gui {

// Descriptor classes for OpenDB objects.  Eventually these should
// become part of the database code generation.

class DbInstDescriptor : public Descriptor
{
 public:
  std::string getName(std::any object) const override;
  std::string getTypeName(std::any object) const override;
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

 private:
  void makeMasterOptions(odb::dbMaster* master, std::vector<EditorOption>& options) const;
  void makePlacementStatusOptions(std::vector<EditorOption>& options) const;
  void makeOrientationOptions(std::vector<EditorOption>& options) const;
  bool setNewLocation(odb::dbInst* inst, std::any value, bool is_x) const;
};

class DbMasterDescriptor : public Descriptor
{
 public:
  std::string getName(std::any object) const override;
  std::string getTypeName(std::any object) const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;

  static void getMasterEquivalent(odb::dbMaster* master, std::set<odb::dbMaster*>& masters);

 private:
  void getInstances(odb::dbMaster* master, std::set<odb::dbInst*>& insts) const;
};

class DbNetDescriptor : public Descriptor
{
 public:
  std::string getName(std::any object) const override;
  std::string getTypeName(std::any object) const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  bool isNet(std::any object) const override;

  Properties getProperties(std::any object) const override;
  Editors getEditors(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;

 private:
  static const int max_iterms_ = 10000;
};

class DbITermDescriptor : public Descriptor
{
 public:
  std::string getName(std::any object) const override;
  std::string getTypeName(std::any object) const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;
};

class DbBTermDescriptor : public Descriptor
{
 public:
  std::string getName(std::any object) const override;
  std::string getTypeName(std::any object) const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  Properties getProperties(std::any object) const override;
  Editors getEditors(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;
};

class DbBlockageDescriptor : public Descriptor
{
 public:
  std::string getName(std::any object) const override;
  std::string getTypeName(std::any object) const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  Properties getProperties(std::any object) const override;
  Editors getEditors(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;
};

class DbObstructionDescriptor : public Descriptor
{
 public:
  std::string getName(std::any object) const override;
  std::string getTypeName(std::any object) const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  Properties getProperties(std::any object) const override;
  Actions getActions(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;
};

class DbTechLayerDescriptor : public Descriptor
{
 public:
  std::string getName(std::any object) const override;
  std::string getTypeName(std::any object) const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;
};

};  // namespace gui
