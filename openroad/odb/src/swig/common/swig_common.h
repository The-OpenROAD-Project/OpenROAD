// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <string>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/defout.h"
#include "odb/geom.h"

odb::dbLib* read_lef(odb::dbDatabase* db, const char* path);

int write_lef(odb::dbLib* lib, const char* path);

int write_tech_lef(odb::dbTech* tech, const char* path);

int write_macro_lef(odb::dbLib* lib, const char* path);

odb::dbChip* read_def(odb::dbTech* tech, const std::string& path);

int write_def(odb::dbBlock* block,
              const char* path,
              odb::DefOut::Version version = odb::DefOut::Version::DEF_5_8);

odb::dbDatabase* read_db(odb::dbDatabase* db, const char* db_path);

int write_db(odb::dbDatabase* db, const char* db_path);

int writeEco(odb::dbBlock* block, const char* filename);

int readEco(odb::dbBlock* block, const char* filename);
// This is a very basic polygon API for scripting.  In C++ you should
// directly use the Boost Polygon classes.

using Rectangle = boost::polygon::rectangle_data<int>;
using Polygon90 = boost::polygon::polygon_90_with_holes_data<int>;
using Polygon90Set = boost::polygon::polygon_90_set_data<int>;

Polygon90Set* newSetFromRect(int xLo, int yLo, int xHi, int yHi);

Polygon90Set* bloatSet(const Polygon90Set* set, int bloating);
Polygon90Set* bloatSet(const Polygon90Set* set, int bloatX, int bloatY);

Polygon90Set* shrinkSet(const Polygon90Set* set, int shrinking);
Polygon90Set* shrinkSet(const Polygon90Set* set, int shrinkX, int shrinkY);

Polygon90Set* andSet(const Polygon90Set* set1, const Polygon90Set* set2);

Polygon90Set* orSet(const Polygon90Set* set1, const Polygon90Set* set2);
Polygon90Set* orSets(const std::vector<Polygon90Set>& sets);

Polygon90Set* subtractSet(const Polygon90Set* set1, const Polygon90Set* set2);

void destroySet(Polygon90Set* set);

void destroyPolygon(Polygon90* polygon);

std::vector<Polygon90> getPolygons(const Polygon90Set* set);

std::vector<odb::Rect> getRectangles(const Polygon90Set* set);

std::vector<odb::Point> getPoints(const Polygon90* polygon);

void createSBoxes(odb::dbSWire* swire,
                  odb::dbTechLayer* layer,
                  const std::vector<odb::Rect>& rects,
                  odb::dbWireShapeType type);

void createSBoxes(odb::dbSWire* swire,
                  odb::dbVia* via,
                  const std::vector<odb::Point>& points,
                  odb::dbWireShapeType type);

void dumpAPs(odb::dbBlock* block, const std::string& file_name);
