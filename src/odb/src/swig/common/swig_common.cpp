// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "swig_common.h"

#include <libgen.h>

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "odb/defout.h"
#include "odb/geom.h"
#include "odb/lefin.h"
#include "odb/lefout.h"
#include "utl/Logger.h"

using boost::polygon::operators::operator+;
using boost::polygon::operators::operator-;
using boost::polygon::operators::operator&;
using boost::polygon::operators::operator|;
using boost::polygon::operators::operator|=;

odb::dbLib* read_lef(odb::dbDatabase* db, const char* path)
{
  utl::Logger logger(nullptr);
  odb::lefin lefParser(db, &logger, false);
  const char* libname = basename(const_cast<char*>(path));
  if (!db->getTech()) {
    return lefParser.createTechAndLib(libname, libname, path);
  }
  return lefParser.createLib(db->getTech(), libname, path);
}

odb::dbChip* read_def(odb::dbTech* tech, const std::string& path)
{
  utl::Logger logger(nullptr);
  std::vector<odb::dbLib*> libs;
  for (auto* lib : tech->getDb()->getLibs()) {
    if (lib->getTech() == tech) {
      libs.push_back(lib);
    }
  }
  odb::defin defParser(tech->getDb(), &logger);
  auto db = tech->getDb();
  odb::dbChip* chip = nullptr;
  if (db->getChip() == nullptr) {
    chip = odb::dbChip::create(db, tech);
  } else {
    chip = db->getChip();
  }
  defParser.readChip(libs, path.c_str(), chip);
  return chip;
}

int write_def(odb::dbBlock* block,
              const char* path,
              odb::DefOut::Version version)
{
  utl::Logger logger(nullptr);
  odb::DefOut writer(&logger);
  writer.setVersion(version);
  return writer.writeBlock(block, path);
}

int write_lef(odb::dbLib* lib, const char* path)
{
  utl::Logger logger(nullptr);
  std::ofstream os;
  os.exceptions(std::ofstream::badbit | std::ofstream::failbit);
  os.open(path);
  odb::lefout writer(&logger, os);
  writer.writeTechAndLib(lib);
  return true;
}

int write_tech_lef(odb::dbTech* tech, const char* path)
{
  utl::Logger logger(nullptr);
  std::ofstream os;
  os.exceptions(std::ofstream::badbit | std::ofstream::failbit);
  os.open(path);
  odb::lefout writer(&logger, os);
  writer.writeTech(tech);
  return true;
}
int write_macro_lef(odb::dbLib* lib, const char* path)
{
  utl::Logger logger(nullptr);
  std::ofstream os;
  os.exceptions(std::ofstream::badbit | std::ofstream::failbit);
  os.open(path);
  odb::lefout writer(&logger, os);
  writer.writeLib(lib);
  return true;
}

odb::dbDatabase* read_db(odb::dbDatabase* db, const char* db_path)
{
  if (db == nullptr) {
    db = odb::dbDatabase::create();
  }

  std::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit
                  | std::ios::eofbit);
  file.open(db_path, std::ios::binary);

  try {
    db->read(file);
  } catch (const std::ios_base::failure& f) {
    auto msg = fmt::format("odb file {} is invalid: {}", db_path, f.what());
    throw std::ios_base::failure(msg);
  }

  return db;
}

int write_db(odb::dbDatabase* db, const char* db_path)
{
  std::ofstream fp(db_path, std::ios::binary);
  if (!fp) {
    int errnum = errno;
    fprintf(stderr, "Error opening file: %s\n", strerror(errnum));
    fprintf(stderr, "Errno: %d\n", errno);
    return errno;
  }
  db->write(fp);
  return 1;
}

int writeEco(odb::dbBlock* block, const char* filename)
{
  odb::dbDatabase::writeEco(block, filename);
  return 1;
}

int readEco(odb::dbBlock* block, const char* filename)
{
  odb::dbDatabase::readEco(block, filename);
  return 1;
}

Polygon90Set* newSetFromRect(int xLo, int yLo, int xHi, int yHi)
{
  using Pt = Polygon90::point_type;
  std::array<Pt, 4> pts
      = {Pt(xLo, yLo), Pt(xHi, yLo), Pt(xHi, yHi), Pt(xLo, yHi)};

  Polygon90 poly;
  poly.set(pts.begin(), pts.end());

  std::array<Polygon90, 1> arr{poly};
  return new Polygon90Set(boost::polygon::HORIZONTAL, arr.begin(), arr.end());
}

Polygon90Set* bloatSet(const Polygon90Set* set, int bloating)
{
  return new Polygon90Set(*set + bloating);
}

Polygon90Set* bloatSet(const Polygon90Set* set, int bloatX, int bloatY)
{
  Polygon90Set* result = new Polygon90Set(*set);
  bloat(*result, bloatX, bloatX, bloatY, bloatY);
  return result;
}

Polygon90Set* shrinkSet(const Polygon90Set* set, int shrinking)
{
  return new Polygon90Set(*set - shrinking);
}

Polygon90Set* shrinkSet(const Polygon90Set* set, int shrinkX, int shrinkY)
{
  Polygon90Set* result = new Polygon90Set(*set);
  shrink(*result, shrinkX, shrinkX, shrinkY, shrinkY);
  return result;
}

Polygon90Set* andSet(const Polygon90Set* set1, const Polygon90Set* set2)
{
  return new Polygon90Set(*set1 & *set2);
}

Polygon90Set* orSet(const Polygon90Set* set1, const Polygon90Set* set2)
{
  return new Polygon90Set(*set1 | *set2);
}

Polygon90Set* orSets(const std::vector<Polygon90Set>& sets)
{
  Polygon90Set* result = new Polygon90Set;
  for (const Polygon90Set& poly_set : sets) {
    *result |= poly_set;
  }
  return result;
}

Polygon90Set* subtractSet(const Polygon90Set* set1, const Polygon90Set* set2)
{
  return new Polygon90Set(*set1 - *set2);
}

void destroySet(Polygon90Set* set)
{
  delete set;
}

void destroyPolygon(Polygon90* polygon)
{
  delete polygon;
}

std::vector<Polygon90> getPolygons(const Polygon90Set* set)
{
  std::vector<Polygon90> s;
  set->get(s);
  return s;
}

std::vector<odb::Rect> getRectangles(const Polygon90Set* set)
{
  std::vector<Rectangle> rects;
  set->get_rectangles(rects);

  // Convert from Boost rect to OpenDB rect
  std::vector<odb::Rect> result;
  result.reserve(rects.size());
  for (auto& r : rects) {
    result.emplace_back(xl(r), yl(r), xh(r), yh(r));
  }
  return result;
}

std::vector<odb::Point> getPoints(const Polygon90* polygon)
{
  std::vector<odb::Point> pts;
  for (auto& pt : *polygon) {
    pts.emplace_back(pt.x(), pt.y());
  }
  return pts;
}

void createSBoxes(odb::dbSWire* swire,
                  odb::dbTechLayer* layer,
                  const std::vector<odb::Rect>& rects,
                  odb::dbWireShapeType type)
{
  for (odb::Rect rect : rects) {
    odb::dbSBox::create(
        swire, layer, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax(), type);
  }
}

void createSBoxes(odb::dbSWire* swire,
                  odb::dbVia* via,
                  const std::vector<odb::Point>& points,
                  odb::dbWireShapeType type)
{
  for (odb::Point point : points) {
    odb::dbSBox::create(swire, via, point.getX(), point.getY(), type);
  }
}

void dumpAPs(odb::dbBlock* block, const std::string& file_name)
{
  std::ofstream os(file_name);
  for (auto inst : block->getInsts()) {
    os << "Inst: " << inst->getName() << "\n";
    for (auto iterm : inst->getITerms()) {
      if (iterm->getSigType().isSupply()) {
        continue;
      }

      auto mterm = iterm->getMTerm();
      auto aps = iterm->getAccessPoints();
      os << "  iterm: " << mterm->getName() << "\n";

      for (auto mpin : mterm->getMPins()) {
        auto bbox = mpin->getBBox();
        os << "    pin (" << bbox.xMin() << ", " << bbox.yMin() << "):\n";

        auto pin_aps_it = aps.find(mpin);
        if (pin_aps_it == aps.end()) {
          continue;
        }
        for (auto ap : pin_aps_it->second) {
          std::vector<odb::dbDirection> dirs;
          ap->getAccesses(dirs);
          auto pt = ap->getPoint();
          os << "      ap ";
          os << "(" << pt.x() << ", " << pt.y() << ") ";
          os << "layer=" << ap->getLayer()->getName() << " ";
          os << "type=" << ap->getLowType().getString() << "/"
             << ap->getHighType().getString() << " ";
          for (const auto& dir : dirs) {
            os << dir.getString() << " ";
          }
          os << "\n";
        }
      }
    }
  }
}
