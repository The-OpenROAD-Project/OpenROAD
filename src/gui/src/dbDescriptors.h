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

#include <map>
#include <set>
#include <vector>

#include "gui/gui.h"
#include "odb/dbWireGraph.h"

namespace odb {
class dbMaster;
}  // namespace odb

namespace sta {
class dbSta;
}  // namespace sta

namespace gui {

// Descriptor classes for OpenDB objects.  Eventually these should
// become part of the database code generation.
class DbTechDescriptor : public Descriptor
{
 public:
  DbTechDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbBlockDescriptor : public Descriptor
{
 public:
  DbBlockDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbInstDescriptor : public Descriptor
{
 public:
  enum Type
  {
    BLOCK,
    PAD,
    PAD_INPUT,
    PAD_OUTPUT,
    PAD_INOUT,
    PAD_POWER,
    PAD_SPACER,
    PAD_AREAIO,
    ENDCAP,
    FILL,
    TAPCELL,
    BUMP,
    COVER,
    ANTENNA,
    TIE,
    LEF_OTHER,
    STD_CELL,
    STD_BUFINV,
    STD_BUFINV_CLK_TREE,
    STD_BUFINV_TIMING_REPAIR,
    STD_CLOCK_GATE,
    STD_LEVEL_SHIFT,
    STD_SEQUENTIAL,
    STD_PHYSICAL,
    STD_COMBINATIONAL,
    STD_OTHER
  };
  DbInstDescriptor(odb::dbDatabase* db, sta::dbSta* sta);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool isInst(std::any object) const override;

  Properties getProperties(std::any object) const override;
  Actions getActions(std::any object) const override;
  Editors getEditors(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

  Type getInstanceType(odb::dbInst* inst) const;
  std::string getInstanceTypeText(Type type) const;

 private:
  void makeMasterOptions(odb::dbMaster* master,
                         std::vector<EditorOption>& options) const;
  void makePlacementStatusOptions(std::vector<EditorOption>& options) const;
  void makeOrientationOptions(std::vector<EditorOption>& options) const;
  bool setNewLocation(odb::dbInst* inst,
                      const std::any& value,
                      bool is_x) const;

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

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

  static void getMasterEquivalent(sta::dbSta* sta,
                                  odb::dbMaster* master,
                                  std::set<odb::dbMaster*>& masters);

 private:
  void getInstances(odb::dbMaster* master, std::set<odb::dbInst*>& insts) const;

  odb::dbDatabase* db_;
  sta::dbSta* sta_;
};

class DbNetDescriptor : public Descriptor
{
 public:
  struct NetWithSink
  {
    odb::dbNet* net;
    odb::dbObject* sink;
  };

  DbNetDescriptor(odb::dbDatabase* db,
                  sta::dbSta* sta,
                  const std::set<odb::dbNet*>& focus_nets,
                  const std::set<odb::dbNet*>& guide_nets,
                  const std::set<odb::dbNet*>& tracks_nets);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;
  bool isSlowHighlight(std::any object) const override;

  bool isNet(std::any object) const override;

  Properties getProperties(std::any object) const override;
  Editors getEditors(std::any object) const override;
  Actions getActions(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
  sta::dbSta* sta_;

  using Node = odb::dbWireGraph::Node;
  using NodeList = std::set<const Node*>;
  using NodeMap = std::map<const Node*, NodeList>;
  using GraphTarget = std::pair<const odb::Rect, const odb::dbTechLayer*>;

  void drawPathSegment(odb::dbNet* net,
                       const odb::dbObject* sink,
                       Painter& painter) const;
  void findSourcesAndSinksInGraph(odb::dbNet* net,
                                  const odb::dbObject* sink,
                                  odb::dbWireGraph* graph,
                                  NodeList& source_nodes,
                                  NodeList& sink_nodes) const;
  void findSourcesAndSinks(odb::dbNet* net,
                           const odb::dbObject* sink,
                           std::vector<GraphTarget>& sources,
                           std::vector<GraphTarget>& sinks) const;
  void findPath(NodeMap& graph,
                const Node* source,
                const Node* sink,
                std::vector<odb::Point>& path) const;

  void buildNodeMap(odb::dbWireGraph* graph, NodeMap& node_map) const;

  const std::set<odb::dbNet*>& focus_nets_;
  const std::set<odb::dbNet*>& guide_nets_;
  const std::set<odb::dbNet*>& tracks_nets_;

  odb::dbNet* getNet(std::any object) const;
  odb::dbObject* getSink(std::any object) const;

  static const int max_iterms_ = 10000;
};

class DbITermDescriptor : public Descriptor
{
 public:
  DbITermDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getShortName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Actions getActions(std::any object) const override;
  Selected makeSelected(std::any object) const override;
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

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Editors getEditors(std::any object) const override;
  Actions getActions(std::any object) const override;
  Selected makeSelected(std::any object) const override;
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

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Editors getEditors(std::any object) const override;
  Selected makeSelected(std::any object) const override;
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

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Actions getActions(std::any object) const override;
  Selected makeSelected(std::any object) const override;
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

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

// The ap doesn't know its location as it is associated the master and
// needs the iterm to get a location
struct DbItermAccessPoint
{
  odb::dbAccessPoint* ap;
  odb::dbITerm* iterm;
};

class DbItermAccessPointDescriptor : public Descriptor
{
 public:
  DbItermAccessPointDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbGroupDescriptor : public Descriptor
{
 public:
  DbGroupDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbRegionDescriptor : public Descriptor
{
 public:
  DbRegionDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbModuleDescriptor : public Descriptor
{
 public:
  DbModuleDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getShortName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;

  void getModules(odb::dbModule* module, SelectionSet& objects) const;
};

class DbTechViaDescriptor : public Descriptor
{
 public:
  DbTechViaDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbMetalWidthViaMapDescriptor : public Descriptor
{
 public:
  DbMetalWidthViaMapDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbGenerateViaDescriptor : public Descriptor
{
 public:
  DbGenerateViaDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbNonDefaultRuleDescriptor : public Descriptor
{
 public:
  DbNonDefaultRuleDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any /* object */, odb::Rect& /* bbox */) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbTechLayerRuleDescriptor : public Descriptor
{
 public:
  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any /* object */, odb::Rect& /* bbox */) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;
};

class DbTechSameNetRuleDescriptor : public Descriptor
{
 public:
  DbTechSameNetRuleDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any /* object */, odb::Rect& /* bbox */) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

class DbSiteDescriptor : public Descriptor
{
 public:
  struct SpecificSite
  {
    odb::dbSite* site;
    odb::Rect rect;
  };

  DbSiteDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;

  odb::dbSite* getSite(std::any object) const;
  odb::Rect getRect(std::any object) const;
  bool isSpecificSite(std::any object) const;
};

class DbRowDescriptor : public Descriptor
{
 public:
  DbRowDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
};

};  // namespace gui
