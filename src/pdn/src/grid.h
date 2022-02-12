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

#include "connect.h"
#include "odb/dbTypes.h"
#include "ring.h"
#include "strap.h"

namespace odb {
class dbBlock;
class dbInst;
class dbMaster;
class dbNet;
class dbRegion;
class dbRow;
class dbSWire;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace pdn {
class VoltageDomain;

class Grid
{
 public:
  enum Type
  {
    Core,
    Instance
  };

  Grid(VoltageDomain* domain, const std::string& name, bool start_with_power);
  virtual ~Grid() {}

  const std::string getName() const { return name_; }
  // returns the long name of the grid.
  virtual const std::string getLongName() const { return getName(); }

  VoltageDomain* getDomain() const { return domain_; }

  void report() const;
  virtual Type type() const = 0;
  static const std::string typeToString(Type type);

  odb::dbBlock* getBlock() const;
  utl::Logger* getLogger() const;

  void addRing(std::unique_ptr<Ring> ring);
  void addStrap(std::unique_ptr<Strap> strap);
  void addConnect(std::unique_ptr<Connect> connect);

  // specify the layers to convert to pins
  void setPinLayers(const std::vector<odb::dbTechLayer*>& layers)
  {
    pin_layers_ = layers;
  }

  // make the shapes for this grid
  void makeShapes(const ShapeTreeMap& global_shapes,
                  const ShapeTreeMap& obstructions);
  const ShapeTreeMap getShapes() const;

  // make the vias for the this grid
  void makeVias(const ShapeTreeMap& global_shapes,
                const ShapeTreeMap& obstructions);
  void getVias(std::vector<ViaPtr>& vias) const;
  void clearVias() { vias_.clear(); }
  void removeVia(const ViaPtr& via);
  // remove all vias which are invalid
  void removeInvalidVias();

  bool startsWithPower() const { return starts_with_power_; }
  bool startsWithGround() const { return !startsWithPower(); }

  // returns the ordered nets for this grid.
  std::vector<odb::dbNet*> getNets(bool starts_with_power) const;
  std::vector<odb::dbNet*> getNets() const
  {
    return getNets(starts_with_power_);
  };

  // returns the core area of the grid
  virtual const odb::Rect getCoreArea() const;
  // returns the largest boundary for the grid
  virtual const odb::Rect getDieArea() const;
  // returns the outline of the rings
  virtual const odb::Rect getRingArea() const;
  // returns the core area to use for extending straps
  virtual const odb::Rect getCoreBoundary() const;
  // returns the  largest boundary to use for extending straps
  virtual const odb::Rect getDieBoundary() const;
  // returns the ring area to use for extending straps
  virtual const odb::Rect getRingBoundary() const;

  const std::vector<std::unique_ptr<Ring>>& getRings() const { return rings_; }
  const std::vector<std::unique_ptr<Strap>>& getStraps() const
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
                 bool do_pins) const;

 protected:
  // find all intersections in the shapes which may become vias
  virtual void getIntersections(std::vector<ViaPtr>& intersections,
                                const ShapeTreeMap& shapes) const;

 private:
  VoltageDomain* domain_;
  std::string name_;
  bool starts_with_power_;

  std::vector<std::unique_ptr<Ring>> rings_;
  std::vector<std::unique_ptr<Strap>> straps_;
  std::vector<std::unique_ptr<Connect>> connect_;

  std::vector<odb::dbTechLayer*> pin_layers_;

  ViaTree vias_;

  // repair unconnected follow pins by adding repair channel straps
  void repairChannels(const ShapeTreeMap& global_shapes,
                      ShapeTreeMap& obstructions);
  struct RepairChannelArea
  {
    odb::Rect area;
    Strap* target;
    odb::dbTechLayer* connect_to;
    std::set<odb::dbNet*> nets;
  };
  // find all follow pins that are not connected for anything
  std::vector<RepairChannelArea> findRepairChannels() const;
  Strap* getTargetStrap(odb::dbTechLayer* layer) const;
  const std::vector<GridShape*> getGridShapes() const;
};

class CoreGrid : public Grid
{
 public:
  CoreGrid(VoltageDomain* domain,
           const std::string& name,
           bool start_with_power);

  virtual Type type() const override { return Grid::Core; }

  virtual const odb::Rect getCoreBoundary() const override;

  // finds all pad instances and adds connection straps to grid
  void setupDirectConnect(
      const std::vector<odb::dbTechLayer*>& connect_pad_layers);

  virtual void getGridLevelObstructions(
      ShapeTreeMap& obstructions) const override;
};

class InstanceGrid : public Grid
{
 public:
  InstanceGrid(VoltageDomain* domain,
               const std::string& name,
               bool start_with_power,
               odb::dbInst* inst);

  virtual const std::string getLongName() const override;

  virtual Type type() const override { return Grid::Instance; }

  void addHalo(const std::array<int, 4>& halos);
  void setGridToBoundary(bool value);

  virtual const odb::Rect getCoreArea() const override;
  virtual const odb::Rect getDieArea() const override;
  virtual const odb::Rect getCoreBoundary() const override;
  virtual const odb::Rect getDieBoundary() const override;

  virtual void getGridLevelObstructions(
      ShapeTreeMap& obstructions) const override;

 protected:
  // find all intersections that also overlap with the power/ground pins based on connectivity
  virtual void getIntersections(std::vector<ViaPtr>& intersections,
                                const ShapeTreeMap& shapes) const override;

 private:
  odb::dbInst* inst_;
  std::array<int, 4> halos_;
  bool grid_to_boundary_;
};

}  // namespace pdn
