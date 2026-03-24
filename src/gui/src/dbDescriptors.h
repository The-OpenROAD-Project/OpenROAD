// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <any>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "db_sta/dbSta.hh"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbWireGraph.h"
#include "odb/geom.h"

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

  Properties getProperties(const std::any& object) const override;

  Selected makeSelected(const std::any& object) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

 protected:
  odb::dbDatabase* db_;

  virtual T* getObject(const std::any& object) const;
  virtual Properties getDBProperties(T* object) const = 0;
};

class DbTechDescriptor : public BaseDbDescriptor<odb::dbTech>
{
 public:
  DbTechDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbTech* tech) const override;
};

class DbBlockDescriptor : public BaseDbDescriptor<odb::dbBlock>
{
 public:
  DbBlockDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbBlock* block) const override;
};

class DbInstDescriptor : public BaseDbDescriptor<odb::dbInst>
{
 public:
  DbInstDescriptor(odb::dbDatabase* db, sta::dbSta* sta);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  bool isInst(const std::any& object) const override;

  Actions getActions(const std::any& object) const override;
  Editors getEditors(const std::any& object) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

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

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

  static void getMasterEquivalent(sta::dbSta* sta,
                                  odb::dbMaster* master,
                                  std::set<odb::dbMaster*>& masters);

  static void getInstances(odb::dbMaster* master,
                           std::set<odb::dbInst*>& insts);

 protected:
  Properties getDBProperties(odb::dbMaster* master) const override;

 private:
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

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;
  bool isSlowHighlight(const std::any& object) const override;

  bool isNet(const std::any& object) const override;

  Editors getEditors(const std::any& object) const override;
  Actions getActions(const std::any& object) const override;
  Selected makeSelected(const std::any& obj) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  odb::dbNet* getObject(const std::any& object) const override;
  Properties getDBProperties(odb::dbNet* net) const override;

 private:
  sta::dbSta* sta_;

  using Node = odb::dbWireGraph::Node;
  using NodeList = std::set<const Node*>;
  using NodeMap = std::map<const Node*, NodeList>;
  using PointList = std::set<odb::Point>;
  using PointMap = std::map<odb::Point, PointList>;
  using GraphTarget = std::pair<const odb::Rect, const odb::dbTechLayer*>;
  using DbTargets = std::map<const odb::dbObject*, std::set<odb::Point>>;

  std::set<odb::Line> convertGuidesToLines(odb::dbNet* net,
                                           DbTargets& sources,
                                           DbTargets& sinks) const;

  void drawPathSegmentWithGraph(odb::dbNet* net,
                                const odb::dbObject* sink,
                                Painter& painter) const;
  void drawPathSegmentWithGuides(const std::set<odb::Line>& lines,
                                 DbTargets& sources,
                                 DbTargets& sinks,
                                 const odb::dbObject* sink,
                                 Painter& painter) const;
  void findSourcesAndSinksInGraph(odb::dbNet* net,
                                  const odb::dbObject* sink,
                                  odb::dbWireGraph* graph,
                                  std::set<NodeList>& source_nodes,
                                  std::set<NodeList>& sink_nodes) const;
  void findSourcesAndSinks(odb::dbNet* net,
                           const odb::dbObject* sink,
                           std::vector<std::set<GraphTarget>>& sources,
                           std::vector<std::set<GraphTarget>>& sinks) const;
  void findPath(NodeMap& graph,
                const Node* source,
                const Node* sink,
                std::vector<odb::Point>& path) const;
  void findPath(PointMap& graph,
                const odb::Point& source,
                const odb::Point& sink,
                std::vector<odb::Point>& path) const;

  void buildNodeMap(odb::dbWireGraph* graph, NodeMap& node_map) const;

  const std::set<odb::dbNet*>& focus_nets_;
  const std::set<odb::dbNet*>& guide_nets_;
  const std::set<odb::dbNet*>& tracks_nets_;

  odb::dbObject* getSink(const std::any& object) const;

  static const int kMaxIterms = 10000;
  static constexpr double kMinGuidePixelWidth = 10.0;
};

class DbWireDescriptor : public BaseDbDescriptor<odb::dbWire>
{
 public:
  DbWireDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbWire* wire) const override;
};

class DbSWireDescriptor : public BaseDbDescriptor<odb::dbSWire>
{
 public:
  DbSWireDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbSWire* wire) const override;

 private:
  static constexpr int kMaxBoxes = 10000;
};

class DbITermDescriptor : public BaseDbDescriptor<odb::dbITerm>
{
 public:
  DbITermDescriptor(odb::dbDatabase* db,
                    std::function<bool()> using_poly_decomp_view);

  std::string getName(const std::any& object) const override;
  std::string getShortName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  Actions getActions(const std::any& object) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbITerm* iterm) const override;

 private:
  std::function<bool()> using_poly_decomp_view_;
};

class DbBTermDescriptor : public BaseDbDescriptor<odb::dbBTerm>
{
 public:
  DbBTermDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  Editors getEditors(const std::any& object) const override;
  Actions getActions(const std::any& object) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbBTerm* bterm) const override;
};

class DbBPinDescriptor : public BaseDbDescriptor<odb::dbBPin>
{
 public:
  DbBPinDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbBPin* bpin) const override;
};

class DbMTermDescriptor : public BaseDbDescriptor<odb::dbMTerm>
{
 public:
  DbMTermDescriptor(odb::dbDatabase* db,
                    std::function<bool()> using_poly_decomp_view);

  std::string getName(const std::any& object) const override;
  std::string getShortName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbMTerm* mterm) const override;

 private:
  std::function<bool()> using_poly_decomp_view_;
};

class DbViaDescriptor : public BaseDbDescriptor<odb::dbVia>
{
 public:
  DbViaDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbVia* via) const override;
};

class DbBlockageDescriptor : public BaseDbDescriptor<odb::dbBlockage>
{
 public:
  DbBlockageDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  Actions getActions(const std::any& object) const override;
  Editors getEditors(const std::any& object) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbBlockage* blockage) const override;
};

class DbObstructionDescriptor : public BaseDbDescriptor<odb::dbObstruction>
{
 public:
  DbObstructionDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  Actions getActions(const std::any& object) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbObstruction* obs) const override;
};

class DbTechLayerDescriptor : public BaseDbDescriptor<odb::dbTechLayer>
{
 public:
  DbTechLayerDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

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
  DbTermAccessPoint(odb::dbAccessPoint* ap, odb::dbITerm* iterm)
      : ap(ap), iterm(iterm)
  {
  }
  DbTermAccessPoint(odb::dbAccessPoint* ap, odb::dbBTerm* bterm)
      : ap(ap), bterm(bterm)
  {
  }
};

class DbTermAccessPointDescriptor : public Descriptor
{
 public:
  DbTermAccessPointDescriptor(odb::dbDatabase* db);

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
  odb::dbDatabase* db_;
};

class DbGroupDescriptor : public BaseDbDescriptor<odb::dbGroup>
{
 public:
  DbGroupDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbGroup* group) const override;
};

class DbRegionDescriptor : public BaseDbDescriptor<odb::dbRegion>
{
 public:
  DbRegionDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbRegion* region) const override;
};

class DbModuleDescriptor : public BaseDbDescriptor<odb::dbModule>
{
 public:
  DbModuleDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getShortName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbModule* module) const override;

 private:
  void getModules(odb::dbModule* module,
                  const std::function<void(const Selected&)>& func) const;
};

class DbModBTermDescriptor : public BaseDbDescriptor<odb::dbModBTerm>
{
 public:
  DbModBTermDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getShortName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbModBTerm* modbterm) const override;

 private:
  void getModBTerms(odb::dbModule* module,
                    const std::function<void(const Selected&)>& func) const;
};

class DbModITermDescriptor : public BaseDbDescriptor<odb::dbModITerm>
{
 public:
  DbModITermDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getShortName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbModITerm* moditerm) const override;

 private:
  void getModITerms(odb::dbModule* module,
                    const std::function<void(const Selected&)>& func) const;
};

class DbModInstDescriptor : public BaseDbDescriptor<odb::dbModInst>
{
 public:
  DbModInstDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getShortName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbModInst* modinst) const override;

 private:
  void getModInsts(odb::dbModule* module,
                   const std::function<void(const Selected&)>& func) const;
};

class DbModNetDescriptor : public BaseDbDescriptor<odb::dbModNet>
{
 public:
  DbModNetDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getShortName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbModNet* modnet) const override;

 private:
  void getModNets(odb::dbModule* module,
                  const std::function<void(const Selected&)>& func) const;
};

class DbTechViaDescriptor : public BaseDbDescriptor<odb::dbTechVia>
{
 public:
  DbTechViaDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbTechVia* via) const override;
};

class DbTechViaRuleDescriptor : public BaseDbDescriptor<odb::dbTechViaRule>
{
 public:
  DbTechViaRuleDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbTechViaRule* via_rule) const override;
};

class DbTechViaLayerRuleDescriptor
    : public BaseDbDescriptor<odb::dbTechViaLayerRule>
{
 public:
  DbTechViaLayerRuleDescriptor(odb::dbDatabase*);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(
      odb::dbTechViaLayerRule* via_layer_rule) const override;
};

class DbMetalWidthViaMapDescriptor
    : public BaseDbDescriptor<odb::dbMetalWidthViaMap>
{
 public:
  DbMetalWidthViaMapDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbMetalWidthViaMap* via_map) const override;
};

class DbGenerateViaDescriptor
    : public BaseDbDescriptor<odb::dbTechViaGenerateRule>
{
 public:
  DbGenerateViaDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbTechViaGenerateRule* via) const override;
};

class DbNonDefaultRuleDescriptor
    : public BaseDbDescriptor<odb::dbTechNonDefaultRule>
{
 public:
  DbNonDefaultRuleDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& /* object */,
               odb::Rect& /* bbox */) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbTechNonDefaultRule* rule) const override;
};

class DbTechLayerRuleDescriptor : public BaseDbDescriptor<odb::dbTechLayerRule>
{
 public:
  DbTechLayerRuleDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& /* object */,
               odb::Rect& /* bbox */) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbTechLayerRule* rule) const override;
};

class DbTechSameNetRuleDescriptor
    : public BaseDbDescriptor<odb::dbTechSameNetRule>
{
 public:
  DbTechSameNetRuleDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& /* object */,
               odb::Rect& /* bbox */) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

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

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  Properties getProperties(const std::any& object) const override;
  Selected makeSelected(const std::any& object) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

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

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbRow* row) const override;
};

class DbMarkerCategoryDescriptor
    : public BaseDbDescriptor<odb::dbMarkerCategory>
{
 public:
  DbMarkerCategoryDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbMarkerCategory* category) const override;
};

class DbMarkerDescriptor : public BaseDbDescriptor<odb::dbMarker>
{
 public:
  DbMarkerDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

  void paintMarker(odb::dbMarker* marker, Painter& painter) const;

 protected:
  Properties getDBProperties(odb::dbMarker* marker) const override;
};

class DbScanInstDescriptor : public BaseDbDescriptor<odb::dbScanInst>
{
 public:
  DbScanInstDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

  static Descriptor::Property getScanPinProperty(
      const std::string& name,
      const std::variant<odb::dbBTerm*, odb::dbITerm*>& pin);

 protected:
  Properties getDBProperties(odb::dbScanInst* scan_inst) const override;
};

class DbScanListDescriptor : public BaseDbDescriptor<odb::dbScanList>
{
 public:
  DbScanListDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbScanList* scan_list) const override;
};

class DbScanPartitionDescriptor : public BaseDbDescriptor<odb::dbScanPartition>
{
 public:
  DbScanPartitionDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(
      odb::dbScanPartition* scan_partition) const override;
};

class DbScanChainDescriptor : public BaseDbDescriptor<odb::dbScanChain>
{
 public:
  DbScanChainDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbScanChain* scan_chain) const override;
};

class DbBoxDescriptor : public BaseDbDescriptor<odb::dbBox>
{
 public:
  struct BoxWithTransform
  {
    odb::dbBox* box;
    odb::dbTransform xform;
  };

  DbBoxDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  Selected makeSelected(const std::any& obj) const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

  bool lessThan(const std::any& l, const std::any& r) const override;

  static void populateProperties(odb::dbBox* box, Properties& props);

 protected:
  Properties getDBProperties(odb::dbBox* box) const override;

 private:
  odb::dbBox* getObject(const std::any& object) const override;
  odb::dbTransform getTransform(const std::any& object) const;
};

class DbSBoxDescriptor : public BaseDbDescriptor<odb::dbSBox>
{
 public:
  DbSBoxDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbSBox* box) const override;
};

class DbMasterEdgeTypeDescriptor
    : public BaseDbDescriptor<odb::dbMasterEdgeType>
{
 public:
  DbMasterEdgeTypeDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;
  static void highlightEdge(odb::dbMaster* master,
                            odb::dbMasterEdgeType* edge,
                            Painter& painter,
                            const std::optional<int>& pen_width = {});

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbMasterEdgeType* edge) const override;
};

class DbCellEdgeSpacingDescriptor
    : public BaseDbDescriptor<odb::dbCellEdgeSpacing>
{
 public:
  DbCellEdgeSpacingDescriptor(odb::dbDatabase* db);

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;

  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, Painter& painter) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

 protected:
  Properties getDBProperties(odb::dbCellEdgeSpacing* rule) const override;
};

};  // namespace gui
