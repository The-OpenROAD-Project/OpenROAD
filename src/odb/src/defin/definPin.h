// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "definBase.h"
#include "definPolygon.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/odb.h"

namespace odb {

class dbBTerm;
class dbTechLayer;
class dbBPin;

class definPin : public definBase
{
  struct PinRect
  {
    dbTechLayer* _layer;
    Rect _rect;
    uint _mask;

    PinRect(dbTechLayer* layer, const Rect& rect, uint mask)
        : _layer(layer), _rect(rect), _mask(mask)
    {
    }
  };

  struct Polygon : public definPolygon
  {
    dbTechLayer* _layer;
    uint _mask;

    Polygon(dbTechLayer* layer, const std::vector<Point>& points, uint mask)
        : definPolygon(points), _layer(layer), _mask(mask)
    {
    }
  };

  struct Pin
  {
    dbBTerm* _bterm;
    std::string _pin;

    Pin(dbBTerm* bterm, const std::string& pin) : _bterm(bterm), _pin(pin) {}
  };

 public:
  int _bterm_cnt;
  int _update_cnt;

 private:
  dbBTerm* _cur_bterm;
  dbPlacementStatus::Value _status;
  dbOrientType::Value _orient;
  int _orig_x;
  int _orig_y;
  int _min_spacing;
  int _effective_width;
  char _left_bus;
  char _right_bus;
  Rect _rect;
  dbTechLayer* _layer;
  bool _has_min_spacing;
  bool _has_effective_width;
  bool _has_placement;
  std::vector<PinRect> _rects;
  std::vector<Polygon> _polygons;
  std::vector<Pin> _ground_pins;
  std::vector<Pin> _supply_pins;

  void addRect(PinRect& r, dbBPin* pin);
  void addPolygon(Polygon& p, dbBPin* pin);

 public:
  // Pin interface methods
  virtual void pinsBegin(int n);
  virtual void pinBegin(const char* name, const char* net);
  virtual void pinSpecial();
  virtual void pinUse(dbSigType type);
  virtual void pinDirection(dbIoType type);
  // Does the pin currently have this direction
  virtual bool checkPinDirection(dbIoType type);
  virtual void pinPlacement(defPlacement status,
                            int x,
                            int y,
                            dbOrientType orient);
  virtual void pinMinSpacing(int spacing);
  virtual void pinEffectiveWidth(int width);
  virtual void pinRect(const char* layer,
                       int x1,
                       int y1,
                       int x2,
                       int y2,
                       uint mask);
  virtual void pinPolygon(std::vector<defPoint>& points, uint mask);
  virtual void pinGroundPin(const char* groundPin);
  virtual void pinSupplyPin(const char* supplyPin);
  virtual void pinEnd();
  virtual void pinsEnd();
  virtual void portBegin();
  virtual void portEnd();

  definPin();
  ~definPin() override;
  void init() override;
};

}  // namespace odb
