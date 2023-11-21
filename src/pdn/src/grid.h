///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "odb/dbTypes.h"
#include "shape.h"

namespace odb {
class dbBlock;
class dbInst;
class dbMaster;
class dbNet;
class dbRegion;
class dbRow;
class dbSWire;
class Rect;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace pdn {
class VoltageDomain;
class Rings;
class Straps;
class Connect;
class GridComponent;
class GridSwitchedPower;

class PdnGen;

class Grid
{
 public:
  enum Type
  {
    Core,
    Instance,
    Existing
  };

  Grid(VoltageDomain* domain,
       const std::string& name,
       bool starts_with_power,
       const std::vector<odb::dbTechLayer*>& generate_obstructions);
  virtual ~Grid();

  std::string getName() const { return name_; }
  // returns the long name of the grid.
  virtual std::string getLongName() const { return getName(); }

  void setDomain(VoltageDomain* domain) { domain_ = domain; }
  VoltageDomain* getDomain() const { return domain_; }

  virtual void report() const;
  virtual Type type() const = 0;
  static std::string typeToString(Type type);

  odb::dbBlock* getBlock() const;
  utl::Logger* getLogger() const;

  virtual void addRing(std::unique_ptr<Rings> ring);
  virtual void addStrap(std::unique_ptr<Straps> strap);
  virtual void addConnect(std::unique_ptr<Connect> connect);

  void removeStrap(Straps* strap);

  std::set<odb::dbTechLayer*> connectableLayers(odb::dbTechLayer* layer) const;

  // specify the layers to convert to pins
  void setPinLayers(const std::vector<odb::dbTechLayer*>& layers)
  {
    pin_layers_.clear();
    pin_layers_.insert(layers.begin(), layers.end());
  }
  const std::set<odb::dbTechLayer*>& getPinLayers() const
  {
    return pin_layers_;
  }

  // make the shapes for this grid
  void makeShapes(const ShapeTreeMap& global_shapes,
                  const ShapeTreeMap& obstructions);
  virtual ShapeTreeMap getShapes() const;

  // make the vias for the this grid
  void makeVias(const ShapeTreeMap& global_shapes,
                const ShapeTreeMap& obstructions,
                ShapeTreeMap& local_obstructions);
  void makeVias(const ShapeTreeMap& global_shapes,
                const ShapeTreeMap& obstructions);
  void getVias(std::vector<ViaPtr>& vias) const;
  void clearVias() { vias_.clear(); }
  void removeVia(const ViaPtr& via);
  // remove all vias which are invalid
  void removeInvalidVias();

  bool startsWithPower() const { return starts_with_power_; }
  bool startsWithGround() const { return !startsWithPower(); }

  void setAllowRepairChannels(bool allow) { allow_repair_channels_ = allow; }
  bool allowsRepairChannels() const { return allow_repair_channels_; }

  // returns the ordered nets for this grid.
  virtual std::vector<odb::dbNet*> getNets(bool starts_with_power) const;
  std::vector<odb::dbNet*> getNets() const
  {
    return getNets(starts_with_power_);
  };

  // returns the core area of the grid
  virtual odb::Rect getDomainArea() const;
  // returns the largest boundary for the grid
  virtual odb::Rect getGridArea() const;
  // returns the outline of the rings
  virtual odb::Rect getRingArea() const;
  // returns the core area to use for extending straps
  virtual odb::Rect getDomainBoundary() const;
  // returns the  largest boundary to use for extending straps
  virtual odb::Rect getGridBoundary() const;

  const std::vector<std::unique_ptr<Rings>>& getRings() const { return rings_; }
  const std::vector<std::unique_ptr<Straps>>& getStraps() const
  {
    return straps_;
  }
  const std::vector<std::unique_ptr<Connect>>& getConnect() const
  {
    return connect_;
  }

  // returns the obstructions the other grids should be aware of,
  // such as the outline of an instance or layers in use
  virtual void getGridLevelObstructions(ShapeTreeMap& obstructions) const;
  void getObstructions(ShapeTreeMap& obstructions) const;

  void resetShapes();

  void writeToDb(const std::map<odb::dbNet*, odb::dbSWire*>& net_map,
                 bool do_pins,
                 const ShapeTreeMap& obstructions) const;
  void makeRoutingObstructions(odb::dbBlock* block) const;

  static void makeInitialObstructions(odb::dbBlock* block,
                                      ShapeTreeMap& obs,
                                      const std::set<odb::dbInst*>& skip_insts,
                                      utl::Logger* logger);
  static void makeInitialShapes(odb::dbBlock* block,
                                ShapeTreeMap& shapes,
                                utl::Logger* logger);

  virtual bool isReplaceable() const { return false; }

  void checkSetup() const;

  void setSwitchedPower(GridSwitchedPower* cell);

  void ripup();

  virtual std::set<odb::dbInst*> getInstances() const;

  bool hasShapes() const;
  bool hasVias() const;

 protected:
  // find all intersections in the shapes which may become vias
  virtual void getIntersections(std::vector<ViaPtr>& intersections,
                                const ShapeTreeMap& shapes) const;

  virtual void cleanupShapes() {}

 private:
  VoltageDomain* domain_;
  std::string name_;
  bool starts_with_power_;

  std::unique_ptr<GridSwitchedPower> switched_power_cell_;

  bool allow_repair_channels_ = false;

  std::vector<std::unique_ptr<Rings>> rings_;
  std::vector<std::unique_ptr<Straps>> straps_;
  std::vector<std::unique_ptr<Connect>> connect_;

  std::set<odb::dbTechLayer*> pin_layers_;
  std::vector<odb::dbTechLayer*> obstruction_layers_;

  ViaTree vias_;

  std::vector<GridComponent*> getGridComponents() const;
  bool repairVias(const ShapeTreeMap& global_shapes,
                  ShapeTreeMap& obstructions);
};

class CoreGrid : public Grid
{
 public:
  CoreGrid(VoltageDomain* domain,
           const std::string& name,
           bool start_with_power,
           const std::vector<odb::dbTechLayer*>& generate_obstructions);

  Type type() const override { return Grid::Core; }

  odb::Rect getDomainBoundary() const override;

  // finds all pad instances and adds connection straps to grid
  void setupDirectConnect(
      const std::vector<odb::dbTechLayer*>& connect_pad_layers);

  void getGridLevelObstructions(ShapeTreeMap& obstructions) const override;

 protected:
  void cleanupShapes() override;
};

class InstanceGrid : public Grid
{
 public:
  InstanceGrid(VoltageDomain* domain,
               const std::string& name,
               bool start_with_power,
               odb::dbInst* inst,
               const std::vector<odb::dbTechLayer*>& generate_obstructions);

  std::string getLongName() const override;

  void report() const override;
  Type type() const override { return Grid::Instance; }

  odb::dbInst* getInstance() const { return inst_; }
  std::set<odb::dbInst*> getInstances() const override { return {inst_}; }

  std::vector<odb::dbNet*> getNets(bool starts_with_power) const override;

  using Halo = std::array<int, 4>;
  void addHalo(const Halo& halos);
  void setGridToBoundary(bool value);

  odb::Rect getDomainArea() const override;
  odb::Rect getGridArea() const override;
  odb::Rect getDomainBoundary() const override;
  odb::Rect getGridBoundary() const override;

  void getGridLevelObstructions(ShapeTreeMap& obstructions) const override;

  void setReplaceable(bool replaceable) { replaceable_ = replaceable; }
  bool isReplaceable() const override { return replaceable_; }

  virtual bool isValid() const;

  static ShapeTreeMap getInstanceObstructions(odb::dbInst* inst,
                                              const Halo& halo = {0, 0, 0, 0});
  static ShapeTreeMap getInstancePins(odb::dbInst* inst);

 protected:
  // find all intersections that also overlap with the power/ground pins based
  // on connectivity
  void getIntersections(std::vector<ViaPtr>& vias,
                        const ShapeTreeMap& shapes) const override;

 private:
  odb::dbInst* inst_;
  Halo halos_ = {0, 0, 0, 0};
  bool grid_to_boundary_ = false;

  bool replaceable_ = false;

  odb::Rect applyHalo(const odb::Rect& rect,
                      bool rect_is_min,
                      bool apply_horizontal,
                      bool apply_vertical) const;
  static odb::Rect applyHalo(const odb::Rect& rect,
                             const Halo& halo,
                             bool rect_is_min,
                             bool apply_horizontal,
                             bool apply_vertical);
};

class BumpGrid : public InstanceGrid
{
 public:
  BumpGrid(VoltageDomain* domain, const std::string& name, odb::dbInst* inst);

  bool isValid() const override;

 private:
  bool isRouted() const;
};

class ExistingGrid : public Grid
{
 public:
  ExistingGrid(PdnGen* pdngen,
               odb::dbBlock* block,
               utl::Logger* logger,
               const std::string& name,
               const std::vector<odb::dbTechLayer*>& generate_obstructions);

  Type type() const override { return Grid::Existing; }

  ShapeTreeMap getShapes() const override { return shapes_; };

  void addRing(std::unique_ptr<Rings> ring) override;
  void addStrap(std::unique_ptr<Straps> strap) override;

 private:
  ShapeTreeMap shapes_;

  std::unique_ptr<VoltageDomain> domain_;

  void populate();

  void addGridComponent(GridComponent* component) const;
};

}  // namespace pdn
