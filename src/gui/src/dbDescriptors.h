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

#include "db_sta/dbSta.hh"
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
template <typename T>
class BaseDbDescriptor : public Descriptor
{
 public:
  BaseDbDescriptor(odb::dbDatabase* db);

  Properties getProperties(std::any object) const override;

  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

 protected:
  odb::dbDatabase* db_;

  virtual T* getObject(const std::any& object) const;
  virtual Properties getDBProperties(T* object) const = 0;
};

class DbTechDescriptor : public BaseDbDescriptor<odb::dbTech>
{
 public:
  DbTechDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbTech* tech) const override;
};

class DbBlockDescriptor : public BaseDbDescriptor<odb::dbBlock>
{
 public:
  DbBlockDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbBlock* block) const override;
};

class DbInstDescriptor : public BaseDbDescriptor<odb::dbInst>
{
 public:
  DbInstDescriptor(odb::dbDatabase* db, sta::dbSta* sta);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool isInst(std::any object) const override;

  Actions getActions(std::any object) const override;
  Editors getEditors(std::any object) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbInst* inst) const override;

 private:
  void makeMasterOptions(odb::dbMaster* master,
                         std::vector<EditorOption>& options) const;
  void makePlacementStatusOptions(std::vector<EditorOption>& options) const;
  void makeOrientationOptions(std::vector<EditorOption>& options) const;
  bool setNewLocation(odb::dbInst* inst,
                      const std::any& value,
                      bool is_x) const;

  sta::dbSta* sta_;
};

class DbMasterDescriptor : public BaseDbDescriptor<odb::dbMaster>
{
 public:
  DbMasterDescriptor(odb::dbDatabase* db, sta::dbSta* sta);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

  static void getMasterEquivalent(sta::dbSta* sta,
                                  odb::dbMaster* master,
                                  std::set<odb::dbMaster*>& masters);

 protected:
  Properties getDBProperties(odb::dbMaster* master) const override;

 private:
  void getInstances(odb::dbMaster* master, std::set<odb::dbInst*>& insts) const;

  sta::dbSta* sta_;
};

class DbNetDescriptor : public BaseDbDescriptor<odb::dbNet>
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

  Editors getEditors(std::any object) const override;
  Actions getActions(std::any object) const override;
  Selected makeSelected(std::any obj) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  odb::dbNet* getObject(const std::any& object) const override;
  Properties getDBProperties(odb::dbNet* net) const override;

 private:
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

  odb::dbObject* getSink(const std::any& object) const;

  static const int max_iterms_ = 10000;
};

class DbITermDescriptor : public BaseDbDescriptor<odb::dbITerm>
{
 public:
  DbITermDescriptor(odb::dbDatabase* db,
                    std::function<bool(void)> usingPolyDecompView);

  std::string getName(std::any object) const override;
  std::string getShortName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Actions getActions(std::any object) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbITerm* iterm) const override;

 private:
  std::function<bool(void)> usingPolyDecompView_;
};

class DbBTermDescriptor : public BaseDbDescriptor<odb::dbBTerm>
{
 public:
  DbBTermDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Editors getEditors(std::any object) const override;
  Actions getActions(std::any object) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbBTerm* bterm) const override;
};

class DbBPinDescriptor : public BaseDbDescriptor<odb::dbBPin>
{
 public:
  DbBPinDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbBPin* bpin) const override;
};

class DbMTermDescriptor : public BaseDbDescriptor<odb::dbMTerm>
{
 public:
  DbMTermDescriptor(odb::dbDatabase* db,
                    std::function<bool(void)> usingPolyDecompView);

  std::string getName(std::any object) const override;
  std::string getShortName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbMTerm* mterm) const override;

 private:
  std::function<bool(void)> usingPolyDecompView_;
};

class DbViaDescriptor : public BaseDbDescriptor<odb::dbVia>
{
 public:
  DbViaDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbVia* via) const override;
};

class DbBlockageDescriptor : public BaseDbDescriptor<odb::dbBlockage>
{
 public:
  DbBlockageDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Editors getEditors(std::any object) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbBlockage* blockage) const override;
};

class DbObstructionDescriptor : public BaseDbDescriptor<odb::dbObstruction>
{
 public:
  DbObstructionDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Actions getActions(std::any object) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbObstruction* obs) const override;
};

class DbTechLayerDescriptor : public BaseDbDescriptor<odb::dbTechLayer>
{
 public:
  DbTechLayerDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbTechLayer* layer) const override;
};

// The ap doesn't know its location as it is associated the master and
// needs the iterm to get a location
struct DbTermAccessPoint
{
  odb::dbAccessPoint* ap{nullptr};
  odb::dbITerm* iterm{nullptr};
  odb::dbBTerm* bterm{nullptr};
  DbTermAccessPoint(odb::dbAccessPoint* apIn, odb::dbITerm* itermIn)
      : ap(apIn), iterm(itermIn)
  {
  }
  DbTermAccessPoint(odb::dbAccessPoint* apIn, odb::dbBTerm* btermIn)
      : ap(apIn), bterm(btermIn)
  {
  }
};

class DbTermAccessPointDescriptor : public Descriptor
{
 public:
  DbTermAccessPointDescriptor(odb::dbDatabase* db);

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

class DbGroupDescriptor : public BaseDbDescriptor<odb::dbGroup>
{
 public:
  DbGroupDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbGroup* group) const override;
};

class DbRegionDescriptor : public BaseDbDescriptor<odb::dbRegion>
{
 public:
  DbRegionDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbRegion* region) const override;
};

class DbModuleDescriptor : public BaseDbDescriptor<odb::dbModule>
{
 public:
  DbModuleDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getShortName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbModule* module) const override;

 private:
  void getModules(odb::dbModule* module, SelectionSet& objects) const;
};

class DbTechViaDescriptor : public BaseDbDescriptor<odb::dbTechVia>
{
 public:
  DbTechViaDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbTechVia* via) const override;
};

class DbTechViaRuleDescriptor : public BaseDbDescriptor<odb::dbTechViaRule>
{
 public:
  DbTechViaRuleDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbTechViaRule* via_rule) const override;
};

class DbTechViaLayerRuleDescriptor
    : public BaseDbDescriptor<odb::dbTechViaLayerRule>
{
 public:
  DbTechViaLayerRuleDescriptor(odb::dbDatabase*);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(
      odb::dbTechViaLayerRule* via_layer_rule) const override;
};

class DbMetalWidthViaMapDescriptor
    : public BaseDbDescriptor<odb::dbMetalWidthViaMap>
{
 public:
  DbMetalWidthViaMapDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbMetalWidthViaMap* via_map) const override;
};

class DbGenerateViaDescriptor
    : public BaseDbDescriptor<odb::dbTechViaGenerateRule>
{
 public:
  DbGenerateViaDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbTechViaGenerateRule* via) const override;
};

class DbNonDefaultRuleDescriptor
    : public BaseDbDescriptor<odb::dbTechNonDefaultRule>
{
 public:
  DbNonDefaultRuleDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any /* object */, odb::Rect& /* bbox */) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbTechNonDefaultRule* rule) const override;
};

class DbTechLayerRuleDescriptor : public BaseDbDescriptor<odb::dbTechLayerRule>
{
 public:
  DbTechLayerRuleDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any /* object */, odb::Rect& /* bbox */) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbTechLayerRule* rule) const override;
};

class DbTechSameNetRuleDescriptor
    : public BaseDbDescriptor<odb::dbTechSameNetRule>
{
 public:
  DbTechSameNetRuleDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any /* object */, odb::Rect& /* bbox */) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbTechSameNetRule* rule) const override;
};

class DbSiteDescriptor : public BaseDbDescriptor<odb::dbSite>
{
 public:
  struct SpecificSite
  {
    odb::dbSite* site;
    const odb::Rect rect;
    const int index_in_row;
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

 protected:
  odb::dbSite* getObject(const std::any& object) const override;
  Properties getDBProperties(odb::dbSite* site) const override;

 private:
  odb::dbSite* getSite(const std::any& object) const;
  odb::Rect getRect(const std::any& object) const;
  bool isSpecificSite(const std::any& object) const;
};

class DbRowDescriptor : public BaseDbDescriptor<odb::dbRow>
{
 public:
  DbRowDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbRow* row) const override;
};

class DbMarkerCategoryDescriptor
    : public BaseDbDescriptor<odb::dbMarkerCategory>
{
 public:
  DbMarkerCategoryDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 protected:
  Properties getDBProperties(odb::dbMarkerCategory* category) const override;
};

class DbMarkerDescriptor : public BaseDbDescriptor<odb::dbMarker>
{
 public:
  DbMarkerDescriptor(odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;

  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  bool getAllObjects(SelectionSet& objects) const override;

  void paintMarker(odb::dbMarker* marker, Painter& painter) const;

 protected:
  Properties getDBProperties(odb::dbMarker* marker) const override;
};

};  // namespace gui
