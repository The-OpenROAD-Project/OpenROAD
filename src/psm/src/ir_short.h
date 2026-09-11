// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>

#include "odb/geom.h"

namespace odb {
class dbNet;
class dbTechLayer;
class dbMarkerCategory;
class dbMarker;
class dbBPin;
class dbITerm;
class dbInst;
class dbObstruction;
class dbFill;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace psm {

// Holds a single overlap between the net being checked and an object that
// does not belong to it. The shape is the overlapping region on layer.
class IRShort
{
 public:
  IRShort(odb::dbTechLayer* layer, const odb::Polygon& shape);
  virtual ~IRShort() = default;

  odb::dbTechLayer* getLayer() const { return layer_; }
  const odb::Polygon& getShape() const { return shape_; }

  void report(odb::dbNet* net, utl::Logger* logger, double dbu) const;
  void commit(odb::dbNet* net, odb::dbMarkerCategory* category) const;

 protected:
  // Describes the object the net is shorted to.
  virtual std::string describe() const = 0;
  // Adds the object the net is shorted to as a source of the marker, only
  // object types dbMarker is able to name can be added.
  virtual void addSources(odb::dbMarker* marker) const = 0;

 private:
  odb::dbTechLayer* layer_;
  odb::Polygon shape_;
};

class IRShortBPin : public IRShort
{
 public:
  IRShortBPin(odb::dbTechLayer* layer,
              const odb::Polygon& shape,
              odb::dbBPin* bpin);

 protected:
  std::string describe() const override;
  void addSources(odb::dbMarker* marker) const override;

 private:
  odb::dbBPin* bpin_;
};

class IRShortInst : public IRShort
{
 public:
  IRShortInst(odb::dbTechLayer* layer,
              const odb::Polygon& shape,
              odb::dbInst* inst);

 protected:
  std::string describe() const override;
  void addSources(odb::dbMarker* marker) const override;

 private:
  odb::dbInst* inst_;
};

class IRShortITerm : public IRShort
{
 public:
  IRShortITerm(odb::dbTechLayer* layer,
               const odb::Polygon& shape,
               odb::dbITerm* iterm);

 protected:
  std::string describe() const override;
  void addSources(odb::dbMarker* marker) const override;

 private:
  odb::dbITerm* iterm_;
};

class IRShortObstruction : public IRShort
{
 public:
  IRShortObstruction(odb::dbTechLayer* layer,
                     const odb::Polygon& shape,
                     odb::dbObstruction* obstruction);

 protected:
  std::string describe() const override;
  void addSources(odb::dbMarker* marker) const override;

 private:
  odb::dbObstruction* obstruction_;
};

class IRShortFill : public IRShort
{
 public:
  IRShortFill(const odb::Polygon& shape, odb::dbFill* fill);

 protected:
  std::string describe() const override;
  void addSources(odb::dbMarker* marker) const override;

 private:
  odb::dbFill* fill_;
};

class IRShortNet : public IRShort
{
 public:
  IRShortNet(odb::dbTechLayer* layer,
             const odb::Polygon& shape,
             odb::dbNet* net);

 protected:
  std::string describe() const override;
  void addSources(odb::dbMarker* marker) const override;

 private:
  odb::dbNet* net_;
};

}  // namespace psm
