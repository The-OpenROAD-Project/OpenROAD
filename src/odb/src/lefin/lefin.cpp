// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/lefin.h"

#include <strings.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "CellEdgeSpacingTableParser.h"
#include "lefLayerPropParser.h"
#include "lefMacroPropParser.h"
#include "lefiDebug.hpp"
#include "lefiUtil.hpp"
#include "lefrReader.hpp"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/poly_decomp.h"
#include "utl/Logger.h"
namespace odb {

using LefParser::lefrSetRelaxMode;

// Protects the LefParser namespace that has static variables
std::mutex lefin::lef_mutex_;

extern bool lefin_parse(lefinReader*, utl::Logger*, const char*);

lefinReader::lefinReader(dbDatabase* db,
                         utl::Logger* logger,
                         bool ignore_non_routing_layers)
    : db_(db),
      tech_(nullptr),
      lib_(nullptr),
      master_(nullptr),
      logger_(logger),
      create_tech_(false),
      create_lib_(false),
      skip_obstructions_(false),
      left_bus_delimiter_('['),
      right_bus_delimiter_(']'),
      hier_delimiter_(0),
      layer_cnt_(0),
      master_cnt_(0),
      via_cnt_(0),
      errors_(0),
      lef_units_(0),
      lib_name_(nullptr),
      dist_factor_(1000.0),
      area_factor_(1000000.0),
      dbu_per_micron_(1000),
      override_lef_dbu_(false),
      master_modified_(false),
      ignore_non_routing_layers_(ignore_non_routing_layers)
{
}

void lefinReader::init()
{
  tech_ = nullptr;
  lib_ = nullptr;
  master_ = nullptr;
  create_tech_ = false;
  create_lib_ = false;
  left_bus_delimiter_ = '[';
  right_bus_delimiter_ = ']';
  hier_delimiter_ = 0;
  layer_cnt_ = 0;
  master_cnt_ = 0;
  via_cnt_ = 0;
  errors_ = 0;

  if (!override_lef_dbu_) {
    lef_units_ = 0;
    dist_factor_ = 1000.0;
    area_factor_ = 1000000.0;
    dbu_per_micron_ = 1000;
  }
}

dbSite* lefinReader::findSite(const char* name)
{
  dbSite* site = lib_->findSite(name);

  if (site == nullptr) {
    // look in the other libs
    for (dbLib* lib : db_->getLibs()) {
      site = lib->findSite(name);
      if (site) {
        break;
      }
    }
  }

  return site;
}

void lefinReader::createLibrary()
{
  lib_ = dbLib::create(db_, lib_name_, tech_, hier_delimiter_);
  lib_->setLefUnits(lef_units_);
  if (left_bus_delimiter_) {
    lib_->setBusDelimiters(left_bus_delimiter_, right_bus_delimiter_);
  }
}

static void create_path_box(dbObject* obj,
                            bool is_pin,
                            dbTechLayer* layer,
                            int dw,
                            int designRuleWidth,
                            int prev_x,
                            int prev_y,
                            int cur_x,
                            int cur_y,
                            utl::Logger* logger)
{
  int x1, x2, y1, y2;

  if ((cur_x == prev_x) && (cur_y == prev_y)) {  // single point
    x1 = cur_x - dw;
    y1 = cur_y - dw;
    x2 = cur_x + dw;
    y2 = cur_y + dw;
    dbBox* box;
    if (is_pin) {
      box = dbBox::create((dbMPin*) obj, layer, x1, y1, x2, y2);
    } else {
      box = dbBox::create((dbMaster*) obj, layer, x1, y1, x2, y2);
    }
    box->setDesignRuleWidth(designRuleWidth);
  } else if (cur_x == prev_x) {  // vert. path
    x1 = cur_x - dw;
    x2 = cur_x + dw;

    if (cur_y > prev_y) {
      y1 = prev_y - dw;
      y2 = cur_y + dw;
    } else {
      y1 = cur_y - dw;
      y2 = prev_y + dw;
    }
    dbBox* box;
    if (is_pin) {
      box = dbBox::create((dbMPin*) obj, layer, x1, y1, x2, y2);
    } else {
      box = dbBox::create((dbMaster*) obj, layer, x1, y1, x2, y2);
    }
    box->setDesignRuleWidth(designRuleWidth);
  } else if (cur_y == prev_y) {  // horiz. path
    y1 = cur_y - dw;
    y2 = cur_y + dw;

    if (cur_x > prev_x) {
      x1 = prev_x - dw;
      x2 = cur_x + dw;
    } else {
      x1 = cur_x - dw;
      x2 = prev_x + dw;
    }
    dbBox* box;
    if (is_pin) {
      box = dbBox::create((dbMPin*) obj, layer, x1, y1, x2, y2);
    } else {
      box = dbBox::create((dbMaster*) obj, layer, x1, y1, x2, y2);
    }
    box->setDesignRuleWidth(designRuleWidth);
  } else {
    logger->warn(utl::ODB, 175, "illegal: non-orthogonal-path at Pin");
  }
}

//
// add geoms to master or terminal
//
bool lefinReader::addGeoms(dbObject* object,
                           bool is_pin,
                           LefParser::lefiGeometries* geometry)
{
  int count = geometry->numItems();
  dbTechLayer* layer = nullptr;
  int dw = 0;
  int designRuleWidth = -1;

  for (int i = 0; i < count; i++) {
    master_modified_ = true;

    switch (geometry->itemType(i)) {
      case LefParser::lefiGeomLayerE: {
        layer = tech_->findLayer(geometry->getLayer(i));

        if (layer == nullptr) {
          logger_->warn(utl::ODB,
                        176,
                        "error: undefined layer ({}) referenced",
                        geometry->getLayer(i));
          return false;
        }

        dw = dbdist(layer->getWidth()) >> 1;
        designRuleWidth = -1;
        break;
      }
      case LefParser::lefiGeomWidthE: {
        dw = dbdist(geometry->getWidth(i)) >> 1;
        break;
      }
      case LefParser::lefiGeomPathE: {
        LefParser::lefiGeomPath* path = geometry->getPath(i);

        if (path->numPoints == 1) {
          int x = dbdist(path->x[0]);
          int y = dbdist(path->y[0]);
          create_path_box(
              object, is_pin, layer, dw, designRuleWidth, x, y, x, y, logger_);
          break;
        }

        int prev_x = dbdist(path->x[0]);
        int prev_y = dbdist(path->y[0]);
        int j;

        for (j = 1; j < path->numPoints; j++) {
          int cur_x = dbdist(path->x[j]);
          int cur_y = dbdist(path->y[j]);
          create_path_box(object,
                          is_pin,
                          layer,
                          dw,
                          designRuleWidth,
                          prev_x,
                          prev_y,
                          cur_x,
                          cur_y,
                          logger_);
          prev_x = cur_x;
          prev_y = cur_y;
        }
        break;
      }
      case LefParser::lefiGeomPathIterE: {
        LefParser::lefiGeomPathIter* pathItr = geometry->getPathIter(i);
        int j;
        std::vector<Point> points;

        for (j = 0; j < pathItr->numPoints; j++) {
          int x = dbdist(pathItr->x[j]);
          int y = dbdist(pathItr->y[j]);
          points.emplace_back(x, y);
        }

        int numX = lround(pathItr->xStart);
        int numY = lround(pathItr->yStart);
        int stepX = dbdist(pathItr->xStep);
        int stepY = dbdist(pathItr->yStep);
        int dx, dy, x_idx, y_idx;

        for (dx = 0, x_idx = 0; x_idx < numX; ++x_idx, dx += stepX) {
          for (dy = 0, y_idx = 0; y_idx < numY; ++y_idx, dy += stepY) {
            if (points.size() == 1) {
              Point p = points[0];
              int x = p.getX() + dx;
              int y = p.getY() + dy;
              create_path_box(object,
                              is_pin,
                              layer,
                              dw,
                              designRuleWidth,
                              x,
                              y,
                              x,
                              y,
                              logger_);
              continue;
            }

            std::vector<Point>::iterator itr = points.begin();
            Point p = *itr;
            int prev_x = p.getX() + dx;
            int prev_y = p.getY() + dy;

            for (++itr; itr != points.end(); ++itr) {
              Point c = *itr;
              int cur_x = c.getX() + dx;
              int cur_y = c.getY() + dy;
              create_path_box(object,
                              is_pin,
                              layer,
                              dw,
                              designRuleWidth,
                              cur_x,
                              cur_y,
                              prev_x,
                              prev_y,
                              logger_);
              prev_x = cur_x;
              prev_y = cur_y;
            }
          }
        }
        break;
      }
      case LefParser::lefiGeomRectE: {
        LefParser::lefiGeomRect* rect = geometry->getRect(i);
        int x1 = dbdist(rect->xl);
        int y1 = dbdist(rect->yl);
        int x2 = dbdist(rect->xh);
        int y2 = dbdist(rect->yh);

        dbBox* box;
        if (is_pin) {
          box = dbBox::create((dbMPin*) object, layer, x1, y1, x2, y2);
        } else {
          box = dbBox::create((dbMaster*) object, layer, x1, y1, x2, y2);
        }
        box->setDesignRuleWidth(designRuleWidth);
        break;
      }
      case LefParser::lefiGeomRectIterE: {
        LefParser::lefiGeomRectIter* rectItr = geometry->getRectIter(i);
        int x1 = dbdist(rectItr->xl);
        int y1 = dbdist(rectItr->yl);
        int x2 = dbdist(rectItr->xh);
        int y2 = dbdist(rectItr->yh);
        int numX = lround(rectItr->xStart);
        int numY = lround(rectItr->yStart);
        int stepX = dbdist(rectItr->xStep);
        int stepY = dbdist(rectItr->yStep);
        int dx, dy, x_idx, y_idx;

        for (dx = 0, x_idx = 0; x_idx < numX; ++x_idx, dx += stepX) {
          for (dy = 0, y_idx = 0; y_idx < numY; ++y_idx, dy += stepY) {
            dbBox* box;
            if (is_pin) {
              box = dbBox::create(
                  (dbMPin*) object, layer, x1 + dx, y1 + dy, x2 + dx, y2 + dy);
            } else {
              box = dbBox::create((dbMaster*) object,
                                  layer,
                                  x1 + dx,
                                  y1 + dy,
                                  x2 + dx,
                                  y2 + dy);
            }
            box->setDesignRuleWidth(designRuleWidth);
          }
        }
        break;
      }
      case LefParser::lefiGeomPolygonE: {
        createPolygon(
            object, is_pin, layer, geometry->getPolygon(i), designRuleWidth);
        break;
      }
      case LefParser::lefiGeomPolygonIterE: {
        LefParser::lefiGeomPolygonIter* pItr = geometry->getPolygonIter(i);
        LefParser::lefiGeomPolygon p;

        p.numPoints = pItr->numPoints;
        p.x = pItr->x;
        p.y = pItr->y;

        // An oddity of the LEF parser is that the count is stored
        // in the start field.
        int num_x = lround(pItr->xStart);
        int num_y = lround(pItr->yStart);

        for (int y = 0; y < num_y; y++) {
          for (int x = 0; x < num_x; x++) {
            createPolygon(object,
                          is_pin,
                          layer,
                          &p,
                          designRuleWidth,
                          x * pItr->xStep,
                          y * pItr->yStep);
          }
        }
        break;
      }
      case LefParser::lefiGeomViaE: {
        LefParser::lefiGeomVia* via = geometry->getVia(i);
        dbTechVia* dbvia = tech_->findVia(via->name);

        if (dbvia == nullptr) {
          logger_->warn(
              utl::ODB, 177, "error: undefined via ({}) referenced", via->name);
          return false;
        }

        int x = dbdist(via->x);
        int y = dbdist(via->y);

        if (is_pin) {
          dbBox::create((dbMPin*) object, dbvia, x, y);
        } else {
          dbBox::create((dbMaster*) object, dbvia, x, y);
        }

        break;
      }
      case LefParser::lefiGeomViaIterE: {
        LefParser::lefiGeomViaIter* viaItr = geometry->getViaIter(i);
        dbTechVia* dbvia = tech_->findVia(viaItr->name);

        if (dbvia == nullptr) {
          logger_->warn(utl::ODB,
                        178,
                        "error: undefined via ({}) referenced",
                        viaItr->name);
          return false;
        }

        int x = dbdist(viaItr->x);
        int y = dbdist(viaItr->y);
        int numX = lround(viaItr->xStart);
        int numY = lround(viaItr->yStart);
        int stepX = dbdist(viaItr->xStep);
        int stepY = dbdist(viaItr->yStep);
        int dx, dy, x_idx, y_idx;

        for (dx = 0, x_idx = 0; x_idx < numX; ++x_idx, dx += stepX) {
          for (dy = 0, y_idx = 0; y_idx < numY; ++y_idx, dy += stepY) {
            if (is_pin) {
              dbBox::create((dbMPin*) object, dbvia, x + dx, y + dy);
            } else {
              dbBox::create((dbMaster*) object, dbvia, x + dx, y + dy);
            }
          }
        }
        break;
      }
      case LefParser::lefiGeomLayerRuleWidthE: {
        designRuleWidth = dbdist(geometry->getLayerRuleWidth(i));
        break;
      }
      // FIXME??
      case LefParser::lefiGeomUnknown:  // error
      case LefParser::lefiGeomLayerExceptPgNetE:
      case LefParser::lefiGeomLayerMinSpacingE:
      case LefParser::lefiGeomClassE:

      default:
        break;
    }
  }

  return true;
}

void lefinReader::createPolygon(dbObject* object,
                                bool is_pin,
                                dbTechLayer* layer,
                                LefParser::lefiGeomPolygon* p,
                                int design_rule_width,
                                double offset_x,
                                double offset_y)
{
  std::vector<Point> points;

  for (int j = 0; j < p->numPoints; ++j) {
    int x = dbdist(p->x[j] + offset_x);
    int y = dbdist(p->y[j] + offset_y);
    points.emplace_back(x, y);
  }

  dbPolygon* pbox = nullptr;
  if (is_pin) {
    pbox = dbPolygon::create((dbMPin*) object, layer, points);
  } else {
    pbox = dbPolygon::create((dbMaster*) object, layer, points);
  }

  if (pbox != nullptr) {
    pbox->setDesignRuleWidth(design_rule_width);
  }
}

void lefinReader::antenna(lefinReader::AntennaType /* unused: type */,
                          double /* unused: value */)
{
}

void lefinReader::arrayBegin(const char* /* unused: name */)
{
  /* Gate arrays not supported */
}

void lefinReader::array(LefParser::lefiArray* /* unused: a */)
{
  /* Gate arrays not supported */
}

void lefinReader::arrayEnd(const char* /* unused: name */)
{
  /* Gate arrays not supported */
}

int lefinReader::busBitChars(const char* busBit)
{
  if (busBit[0] == '\0' || busBit[1] == '\0') {
    logger_->error(utl::ODB, 179, "invalid BUSBITCHARS ({})\n", busBit);
  }

  left_bus_delimiter_ = busBit[0];
  right_bus_delimiter_ = busBit[1];

  if (lib_) {
    lib_->setBusDelimiters(left_bus_delimiter_, right_bus_delimiter_);
  }

  return PARSE_OK;
}

void lefinReader::caseSense(int caseSense)
{
  tech_->setNamesCaseSensitive(dbOnOffType(caseSense));
}

void lefinReader::clearance(const char* name)
{
  tech_->setClearanceMeasure(dbClMeasureType(name));
}

void lefinReader::divider(const char* div)
{
  hier_delimiter_ = div[0];
}

void lefinReader::noWireExt(const char* name)
{
  tech_->setNoWireExtAtPin(dbOnOffType(name));
}

void lefinReader::noiseMargin(LefParser::lefiNoiseMargin* /* unused: noise */)
{
}

void lefinReader::edge1(double /* unused: name */)
{
}

void lefinReader::edge2(double /* unused: name */)
{
}

void lefinReader::edgeScale(double /* unused: name */)
{
}

void lefinReader::noiseTable(LefParser::lefiNoiseTable* /* unused: noise */)
{
}

void lefinReader::correction(LefParser::lefiCorrectionTable* /* unused: corr */)
{
}

void lefinReader::dielectric(double /* unused: dielectric */)
{
}

void lefinReader::irdropBegin(void* /* unused: ptr */)
{
}

void lefinReader::irdrop(LefParser::lefiIRDrop* /* unused: irdrop */)
{
}

void lefinReader::irdropEnd(void* /* unused: ptr */)
{
}

void lefinReader::layer(LefParser::lefiLayer* layer)
{
  if (!create_tech_) {
    return;
  }

  if (tech_->findLayer(layer->name())) {
    logger_->warn(utl::ODB, 180, "duplicate LAYER ({}) ignored", layer->name());
    return;
  }

  for (int i = 0; i < layer->numProps(); i++) {
    if (!strcmp(layer->propName(i), "LEF58_REGION")) {
      logger_->warn(
          utl::ODB, 423, "LEF58_REGION layer {} ignored", layer->name());
      return;
    }
  }

  dbTechLayerType type(dbTechLayerType::ROUTING);

  if (layer->hasType()) {
    type = dbTechLayerType(layer->type());
  }

  if (ignore_non_routing_layers_
      && ((type != dbTechLayerType::ROUTING) && (type != dbTechLayerType::CUT)
          && (type != dbTechLayerType::MASTERSLICE)
          && (type != dbTechLayerType::OVERLAP))) {
    logger_->warn(utl::ODB,
                  181,
                  "Skipping LAYER ({}) ; Non Routing or Cut type",
                  layer->name());
    return;
  }

  dbTechLayer* l = dbTechLayer::create(tech_, layer->name(), type);
  if (l == nullptr) {
    logger_->warn(utl::ODB,
                  182,
                  "Skipping LAYER ({}) ; cannot understand type",
                  layer->name());
    return;
  }

  if (layer->hasPitch()) {
    l->setPitch(dbdist(layer->pitch()));
  } else if (layer->hasXYPitch()) {
    l->setPitchXY(dbdist(layer->pitchX()), dbdist(layer->pitchY()));
  }

  for (int iii = 0; iii < layer->numProps(); iii++) {
    dbStringProperty::create(l, layer->propName(iii), layer->propValue(iii));
    bool valid = true;
    bool supported = true;
    if (type.getValue() == dbTechLayerType::ROUTING) {
      if (!strcmp(layer->propName(iii), "LEF58_SPACING")) {
        if (std::string(layer->propValue(iii)).find("WRONGDIRECTION")
            != std::string::npos) {
          lefTechLayerWrongDirSpacingParser::parse(
              layer->propValue(iii), l, this);
        } else {
          lefTechLayerSpacingEolParser::parse(layer->propValue(iii), l, this);
        }
      } else if (!strcmp(layer->propName(iii), "LEF58_MINSTEP")
                 || !strcmp(layer->propName(iii), "LEF57_MINSTEP")) {
        lefTechLayerMinStepParser minStepParser;
        valid = minStepParser.parse(layer->propValue(iii), l, this);
      } else if (!strcmp(layer->propName(iii), "LEF58_CORNERSPACING")) {
        valid = lefTechLayerCornerSpacingParser::parse(
            layer->propValue(iii), l, this);
      } else if (!strcmp(layer->propName(iii), "LEF58_SPACINGTABLE")) {
        if (std::string(layer->propValue(iii)).find("PARALLELRUNLENGTH")
            == std::string::npos) {
          warning(256,
                  "unsupported {} property for layer {} :\"{}\"",
                  layer->propName(iii),
                  layer->name(),
                  layer->propValue(iii));
        } else {
          lefTechLayerSpacingTablePrlParser parser;
          valid = parser.parse(layer->propValue(iii), l, this);
        }
      } else if (!strcmp(layer->propName(iii), "LEF58_RIGHTWAYONGRIDONLY")) {
        valid = lefTechLayerRightWayOnGridOnlyParser::parse(
            layer->propValue(iii), l, this);
      } else if (!strcmp(layer->propName(iii), "LEF58_RECTONLY")) {
        valid
            = lefTechLayerRectOnlyParser::parse(layer->propValue(iii), l, this);
      } else if (!strcmp(layer->propName(iii), "LEF58_TYPE")) {
        valid = lefTechLayerTypeParser::parse(layer->propValue(iii), l, this);
      } else if (!strcmp(layer->propName(iii), "LEF58_EOLEXTENSIONSPACING")) {
        lefTechLayerEolExtensionRuleParser parser(this);
        parser.parse(layer->propValue(iii), l);
      } else if (!strcmp(layer->propName(iii), "LEF58_EOLKEEPOUT")) {
        lefTechLayerEolKeepOutRuleParser eolkoutParser(this);
        eolkoutParser.parse(layer->propValue(iii), l);
      } else if (!strcmp(layer->propName(iii), "LEF58_WIDTHTABLE")) {
        WidthTableParser parser(l, this);
        parser.parse(layer->propValue(iii));
      } else if (!strcmp(layer->propName(iii), "LEF58_MINIMUMCUT")) {
        MinCutParser parser(l, this);
        parser.parse(layer->propValue(iii));
      } else if (!strcmp(layer->propName(iii), "LEF58_PITCH")) {
        lefTechLayerPitchRuleParser parser(this);
        parser.parse(layer->propValue(iii), l);
      } else if (!strcmp(layer->propName(iii), "LEF58_AREA")) {
        lefTechLayerAreaRuleParser parser(this);
        parser.parse(layer->propValue(iii), l, incomplete_props_);
      } else if (!strcmp(layer->propName(iii), "LEF58_FORBIDDENSPACING")) {
        lefTechLayerForbiddenSpacingRuleParser parser(this);
        parser.parse(layer->propValue(iii), l);
      } else if (!strcmp(layer->propName(iii),
                         "LEF58_TWOWIRESFORBIDDENSPACING")) {
        lefTechLayerTwoWiresForbiddenSpcRuleParser parser(this);
        parser.parse(layer->propValue(iii), l);
      } else if (!strcmp(layer->propName(iii), "LEF58_MINWIDTH")) {
        MinWidthParser parser(l, this);
        parser.parse(layer->propValue(iii));
      } else if (!strcmp(layer->propName(iii), "LEF57_ANTENNAGATEPLUSDIFF")) {
        AntennaGatePlusDiffParser parser(layer, this);
        parser.parse(layer->propValue(iii));
      } else if (!strcmp(layer->propName(iii), "LEF58_VOLTAGESPACING")) {
        lefTechLayerVoltageSpacing parser(l, this);
        parser.parse(layer->propValue(iii));
      } else {
        supported = false;
      }
    } else if (type.getValue() == dbTechLayerType::CUT) {
      if (!strcmp(layer->propName(iii), "LEF58_SPACING")
          || !strcmp(layer->propName(iii), "LEF57_SPACING")) {
        lefTechLayerCutSpacingParser cutSpacingParser;
        valid = cutSpacingParser.parse(
            layer->propValue(iii), l, this, incomplete_props_);
      } else if (!strcmp(layer->propName(iii), "LEF58_CUTCLASS")) {
        valid
            = lefTechLayerCutClassParser::parse(layer->propValue(iii), l, this);
      } else if (!strcmp(layer->propName(iii), "LEF58_ENCLOSURE")) {
        lefTechLayerCutEnclosureRuleParser encParser(this);
        encParser.parse(layer->propValue(iii), l);
      } else if (!strcmp(layer->propName(iii), "LEF58_SPACINGTABLE")) {
        lefTechLayerCutSpacingTableParser cutSpacingTableParser(l);
        valid = cutSpacingTableParser.parse(
            layer->propValue(iii), this, incomplete_props_);
      } else if (!strcmp(layer->propName(iii), "LEF58_ARRAYSPACING")) {
        ArraySpacingParser parser(l, this);
        valid = parser.parse(layer->propValue(iii));
      } else if (!strcmp(layer->propName(iii), "LEF58_TYPE")) {
        valid = lefTechLayerTypeParser::parse(layer->propValue(iii), l, this);
      } else if (!strcmp(layer->propName(iii), "LEF58_KEEPOUTZONE")) {
        KeepOutZoneParser parser(l, this);
        parser.parse(layer->propValue(iii));
      } else if (!strcmp(layer->propName(iii), "LEF58_MAXSPACING")) {
        MaxSpacingParser parser(l, this);
        parser.parse(layer->propValue(iii));
      } else if (!strcmp(layer->propName(iii), "LEF57_ANTENNAGATEPLUSDIFF")) {
        AntennaGatePlusDiffParser parser(layer, this);
        parser.parse(layer->propValue(iii));
      } else {
        supported = false;
      }
    } else if (type.getValue() == dbTechLayerType::MASTERSLICE) {
      if (!strcmp(layer->propName(iii), "LEF58_TYPE")) {
        valid = lefTechLayerTypeParser::parse(layer->propValue(iii), l, this);
      } else {
        supported = false;
      }
    } else if (type.getValue() == dbTechLayerType::IMPLANT) {
      if (!strcmp(layer->propName(iii), "LEF58_AREA")) {
        lefTechLayerAreaRuleParser parser(this);
        parser.parse(layer->propValue(iii), l, incomplete_props_);
      } else {
        supported = false;
      }
    } else {
      supported = false;
    }
    if (supported && !valid) {
      logger_->warn(utl::ODB,
                    279,
                    "parse mismatch in layer property {} for layer {} : \"{}\"",
                    layer->propName(iii),
                    layer->name(),
                    layer->propValue(iii));
    }
    if (!supported) {
      logger_->info(utl::ODB,
                    388,
                    "unsupported {} property for layer {} :\"{}\"",
                    layer->propName(iii),
                    layer->name(),
                    layer->propValue(iii));
    }
  }
  // update wrong way width
  for (auto rule : l->getTechLayerWidthTableRules()) {
    if (rule->isWrongDirection()) {
      l->setWrongWayWidth(*rule->getWidthTable().begin());
      break;
    }
  }
  if (layer->hasWidth()) {
    l->setWidth(dbdist(layer->width()));
  }

  if (l->getMinWidth() == 0) {
    if (layer->hasMinwidth()) {
      l->setMinWidth(dbdist(layer->minwidth()));
    } else if (type == dbTechLayerType::ROUTING) {
      l->setMinWidth(l->getWidth());
    }
  }
  if (l->getWrongWayMinWidth() == 0) {
    l->setWrongWayMinWidth(l->getWrongWayWidth());
  }

  if (layer->hasOffset()) {
    l->setOffset(dbdist(layer->offset()));
  } else if (layer->hasXYOffset()) {
    l->setOffsetXY(dbdist(layer->offsetX()), dbdist(layer->offsetY()));
  }

  int j;
  dbTechLayerSpacingRule* cur_rule;
  if (layer->hasSpacingNumber()) {
    for (j = 0; j < layer->numSpacing(); j++) {
      cur_rule = dbTechLayerSpacingRule::create(l);
      cur_rule->setSpacing(dbdist(layer->spacing(j)));

      cur_rule->setCutStacking(layer->hasSpacingLayerStack(j));
      cur_rule->setCutCenterToCenter(layer->hasSpacingCenterToCenter(j));
      cur_rule->setCutSameNet(layer->hasSpacingSamenet(j));
      cur_rule->setSameNetPgOnly(layer->hasSpacingSamenetPGonly(j));
      cur_rule->setCutParallelOverlap(layer->hasSpacingParallelOverlap(j));
      cur_rule->setSpacingEndOfNotchWidthValid(
          layer->hasSpacingEndOfNotchWidth(j));
      cur_rule->setSpacingNotchLengthValid(layer->hasSpacingNotchLength(j));

      if (layer->hasSpacingArea(j)) {
        cur_rule->setCutArea(dbdist(layer->spacingArea(j)));
      }

      if (layer->hasSpacingRange(j)) {
        cur_rule->setRange(dbdist(layer->spacingRangeMin(j)),
                           dbdist(layer->spacingRangeMax(j)));
        if (layer->hasSpacingRangeUseLengthThreshold(j)) {
          cur_rule->setUseLengthThreshold();
        } else if (layer->hasSpacingRangeInfluence(j)) {
          cur_rule->setInfluence(dbdist(layer->spacingRangeInfluence(j)));
          if (layer->hasSpacingRangeInfluenceRange(j)) {
            cur_rule->setInfluenceRange(
                dbdist(layer->spacingRangeInfluenceMin(j)),
                dbdist(layer->spacingRangeInfluenceMax(j)));
          }
        } else if (layer->hasSpacingRangeRange(j)) {
          cur_rule->setRangeRange(dbdist(layer->spacingRangeRangeMin(j)),
                                  dbdist(layer->spacingRangeRangeMax(j)));
        }
      } else if (layer->hasSpacingLengthThreshold(j)) {
        cur_rule->setLengthThreshold(dbdist(layer->spacingLengthThreshold(j)));
        if (layer->hasSpacingLengthThresholdRange(j)) {
          cur_rule->setLengthThresholdRange(
              dbdist(layer->spacingLengthThresholdRangeMin(j)),
              dbdist(layer->spacingLengthThresholdRangeMax(j)));
        }
      } else if (layer->hasSpacingAdjacent(j)) {
        cur_rule->setAdjacentCuts(layer->spacingAdjacentCuts(j),
                                  dbdist(layer->spacingAdjacentWithin(j)),
                                  dbdist(layer->spacing(j)),
                                  layer->hasSpacingAdjacentExcept(j));
      } else if (layer->hasSpacingEndOfLine(j)) {
        double w = layer->spacingEolWidth(j);
        double wn = layer->spacingEolWithin(j);
        if (layer->hasSpacingParellelEdge(j)) {
          double ps = layer->spacingParSpace(j);
          double pw = layer->spacingParWithin(j);
          bool t = layer->hasSpacingTwoEdges(j);
          cur_rule->setEol(
              dbdist(w), dbdist(wn), true, dbdist(ps), dbdist(pw), t);
        } else {
          cur_rule->setEol(dbdist(w), dbdist(wn), false, 0, 0, false);
        }
      } else if (layer->hasSpacingName(j)) {
        dbTechLayer* tmply = tech_->findLayer(layer->spacingName(j));
        if (tmply == nullptr) {
          logger_->error(utl::ODB,
                         183,
                         "In layer {}, spacing layer {} not found",
                         layer->name(),
                         layer->spacingName(j));
        }
        cur_rule->setCutLayer4Spacing(tmply);
      } else {
        l->setSpacing(dbdist(layer->spacing(j)));
      }
    }
  }

  if (layer->hasArraySpacing()) {
    const bool is_long = layer->hasLongArray();
    const int cut_spacing = dbdist(layer->cutSpacing());
    int width = 0;
    if (layer->hasViaWidth()) {
      width = dbdist(layer->viaWidth());
    }
    for (j = 0; j < layer->numArrayCuts(); j++) {
      const int array_spacing = dbdist(layer->arraySpacing(j));
      const int cuts = layer->arrayCuts(j);

      auto* rule = odb::dbTechLayerArraySpacingRule::create(l);
      rule->setCutSpacing(cut_spacing);
      rule->setArrayWidth(width);
      rule->setLongArray(is_long);
      rule->setCutsArraySpacing(cuts, array_spacing);
    }
  }

  LefParser::lefiSpacingTable* cur_sptbl;
  for (j = 0; j < layer->numSpacingTable(); j++) {
    cur_sptbl = layer->spacingTable(j);
    if (cur_sptbl->isInfluence()) {
      LefParser::lefiInfluence* cur_ifl = cur_sptbl->influence();
      int iflidx;
      dbTechV55InfluenceEntry* iflitem;
      for (iflidx = 0; iflidx < cur_ifl->numInfluenceEntry(); iflidx++) {
        iflitem = dbTechV55InfluenceEntry::create(l);
        iflitem->setV55InfluenceEntry(dbdist(cur_ifl->width(iflidx)),
                                      dbdist(cur_ifl->distance(iflidx)),
                                      dbdist(cur_ifl->spacing(iflidx)));
      }
    } else if (cur_sptbl->isParallel()) {
      LefParser::lefiParallel* cur_ipl = cur_sptbl->parallel();
      int wddx, lndx;

      l->initV55LengthIndex(cur_ipl->numLength());
      for (lndx = 0; lndx < cur_ipl->numLength(); lndx++) {
        l->addV55LengthEntry(dbdist(cur_ipl->length(lndx)));
      }

      l->initV55WidthIndex(cur_ipl->numWidth());
      l->initV55SpacingTable(cur_ipl->numWidth(), cur_ipl->numLength());
      for (wddx = 0; wddx < cur_ipl->numWidth(); wddx++) {
        l->addV55WidthEntry(dbdist(cur_ipl->width(wddx)));
        for (lndx = 0; lndx < cur_ipl->numLength(); lndx++) {
          l->addV55SpacingTableEntry(
              wddx, lndx, dbdist(cur_ipl->widthSpacing(wddx, lndx)));
          if ((wddx == 0) && (lndx == 0)) {
            l->setSpacing(dbdist(cur_ipl->widthSpacing(wddx, lndx)));
          }
        }
      }
    } else {  // two width spacing rule
      LefParser::lefiTwoWidths* cur_two = cur_sptbl->twoWidths();

      l->initTwoWidths(cur_two->numWidth());
      int defaultPrl = -1;
      for (int i = 0; i < cur_two->numWidth(); i++) {
        int prl = cur_two->hasWidthPRL(i) ? dbdist(cur_two->widthPRL(i))
                                          : defaultPrl;
        l->addTwoWidthsIndexEntry(dbdist(cur_two->width(i)), prl);
      }

      for (int i = 0; i < cur_two->numWidth(); i++) {
        assert(cur_two->numWidth() == cur_two->numWidthSpacing(i));

        for (int j = 0; j < cur_two->numWidth(); j++) {
          l->addTwoWidthsSpacingTableEntry(
              i, j, dbdist(cur_two->widthSpacing(i, j)));
        }
      }
    }
  }
  if (layer->hasSpacingTableOrtho()) {
    auto orth = layer->orthogonal();
    for (int k = 0; k < orth->numOrthogonal(); k++) {
      const int within = dbdist(orth->cutWithin(k));
      const int spacing = dbdist(orth->orthoSpacing(k));
      l->addOrthSpacingTableEntry(within, spacing);
    }
  }
  dbTechMinCutRule* cur_cut_rule;
  bool from_above, from_below;
  for (j = 0; j < layer->numMinimumcut(); j++) {
    cur_cut_rule = dbTechMinCutRule::create(l);
    from_above = from_below = false;
    if (layer->hasMinimumcutConnection(j)) {
      from_above = strcasecmp(layer->minimumcutConnection(j), "FROMABOVE") == 0;
      from_below = strcasecmp(layer->minimumcutConnection(j), "FROMBELOW") == 0;
    }

    cur_cut_rule->setMinimumCuts(layer->minimumcut(j),
                                 dbdist(layer->minimumcutWidth(j)),
                                 from_above,
                                 from_below);

    if (layer->hasMinimumcutWithin(j)) {
      cur_cut_rule->setCutDistance(dbdist(layer->minimumcutWithin(j)));
    }

    if (layer->hasMinimumcutNumCuts(j)) {
      cur_cut_rule->setLengthForCuts(dbdist(layer->minimumcutLength(j)),
                                     dbdist(layer->minimumcutDistance(j)));
    }
  }

  dbTechMinEncRule* cur_enc_rule;
  for (j = 0; j < layer->numMinenclosedarea(); j++) {
    cur_enc_rule = dbTechMinEncRule::create(l);
    cur_enc_rule->setEnclosure(dbarea(layer->minenclosedarea(j)));
    if (layer->hasMinenclosedareaWidth(j)) {
      cur_enc_rule->setEnclosureWidth(dbdist(layer->minenclosedareaWidth(j)));
    }
  }

  dbTechLayerAntennaRule* cur_ant_rule;
  LefParser::lefiAntennaModel* cur_model;
  LefParser::lefiAntennaPWL* cur_pwl;
  std::vector<double> dffdx, dffratio;
  int k;

  if (layer->numAntennaModel() > 0) {
    for (j = 0; j < std::min(layer->numAntennaModel(), 2); j++) {
      cur_ant_rule = (j == 1) ? l->createOxide2AntennaRule()
                              : l->createDefaultAntennaRule();
      cur_model = layer->antennaModel(j);
      if (cur_model->hasAntennaAreaFactor()) {
        cur_ant_rule->setAreaFactor(cur_model->antennaAreaFactor(),
                                    cur_model->hasAntennaAreaFactorDUO());
      }

      if (cur_model->hasAntennaSideAreaFactor()) {
        cur_ant_rule->setSideAreaFactor(
            cur_model->antennaSideAreaFactor(),
            cur_model->hasAntennaSideAreaFactorDUO());
      }

      if (cur_model->hasAntennaAreaRatio()) {
        cur_ant_rule->setPAR(cur_model->antennaAreaRatio());
      }

      if (cur_model->hasAntennaDiffAreaRatio()) {
        cur_ant_rule->setDiffPAR(cur_model->antennaDiffAreaRatio());
      } else if (cur_model->hasAntennaDiffAreaRatioPWL()) {
        dffdx.clear();
        dffratio.clear();
        cur_pwl = cur_model->antennaDiffAreaRatioPWL();
        for (k = 0; k < cur_pwl->numPWL(); k++) {
          dffdx.push_back(cur_pwl->PWLdiffusion(k));
          dffratio.push_back(cur_pwl->PWLratio(k));
        }
        cur_ant_rule->setDiffPAR(dffdx, dffratio);
      }

      if (cur_model->hasAntennaCumAreaRatio()) {
        cur_ant_rule->setCAR(cur_model->antennaCumAreaRatio());
      }

      if (cur_model->hasAntennaCumDiffAreaRatio()) {
        cur_ant_rule->setDiffCAR(cur_model->antennaCumDiffAreaRatio());
      } else if (cur_model->hasAntennaCumDiffAreaRatioPWL()) {
        dffdx.clear();
        dffratio.clear();
        cur_pwl = cur_model->antennaCumDiffAreaRatioPWL();
        for (k = 0; k < cur_pwl->numPWL(); k++) {
          dffdx.push_back(cur_pwl->PWLdiffusion(k));
          dffratio.push_back(cur_pwl->PWLratio(k));
        }
        cur_ant_rule->setDiffCAR(dffdx, dffratio);
      }

      if (cur_model->hasAntennaSideAreaRatio()) {
        cur_ant_rule->setPSR(cur_model->antennaSideAreaRatio());
      }

      if (cur_model->hasAntennaDiffSideAreaRatio()) {
        cur_ant_rule->setDiffPSR(cur_model->antennaDiffSideAreaRatio());
      } else if (cur_model->hasAntennaDiffSideAreaRatioPWL()) {
        dffdx.clear();
        dffratio.clear();
        cur_pwl = cur_model->antennaDiffSideAreaRatioPWL();
        for (k = 0; k < cur_pwl->numPWL(); k++) {
          dffdx.push_back(cur_pwl->PWLdiffusion(k));
          dffratio.push_back(cur_pwl->PWLratio(k));
        }
        cur_ant_rule->setDiffPSR(dffdx, dffratio);
      }

      if (cur_model->hasAntennaCumSideAreaRatio()) {
        cur_ant_rule->setCSR(cur_model->antennaCumSideAreaRatio());
      }

      if (cur_model->hasAntennaCumDiffSideAreaRatio()) {
        cur_ant_rule->setDiffCSR(cur_model->antennaCumDiffSideAreaRatio());
      } else if (cur_model->hasAntennaCumDiffSideAreaRatioPWL()) {
        dffdx.clear();
        dffratio.clear();
        cur_pwl = cur_model->antennaCumDiffSideAreaRatioPWL();
        for (k = 0; k < cur_pwl->numPWL(); k++) {
          dffdx.push_back(cur_pwl->PWLdiffusion(k));
          dffratio.push_back(cur_pwl->PWLratio(k));
        }
        cur_ant_rule->setDiffCSR(dffdx, dffratio);
      }

      if (cur_model->hasAntennaCumRoutingPlusCut()) {
        cur_ant_rule->setAntennaCumRoutingPlusCut();
      }

      if (cur_model->hasAntennaGatePlusDiff()) {
        double factor = cur_model->antennaGatePlusDiff();
        cur_ant_rule->setGatePlusDiffFactor(factor);
      }

      if (cur_model->hasAntennaAreaMinusDiff()) {
        double factor = cur_model->antennaAreaMinusDiff();
        cur_ant_rule->setAreaMinusDiffFactor(factor);
      }

      if (cur_model->hasAntennaAreaDiffReducePWL()) {
        dffdx.clear();
        dffratio.clear();
        cur_pwl = cur_model->antennaAreaDiffReducePWL();
        for (k = 0; k < cur_pwl->numPWL(); k++) {
          dffdx.push_back(cur_pwl->PWLdiffusion(k));
          dffratio.push_back(cur_pwl->PWLratio(k));
        }
        cur_ant_rule->setAreaDiffReduce(dffdx, dffratio);
      }
    }
  }

  if (layer->hasArea()) {
    l->setArea(layer->area());
  }

  if (layer->hasThickness()) {
    l->setThickness(dbdist(layer->thickness()));
  }

  if (layer->hasMaxwidth()) {
    l->setMaxWidth(dbdist(layer->maxwidth()));
  }

  if (layer->hasMask()) {
    l->setNumMasks(layer->mask());
  }

  if (layer->hasMinstep()) {
    l->setMinStep(dbdist(layer->minstep(0)));
    if (layer->hasMinstepType(0)) {
      l->setMinStepType(layer->minstepType(0));
    }
    if (layer->hasMinstepLengthsum(0)) {
      l->setMinStepMaxLength(dbdist(layer->minstepLengthsum(0)));
    }
    if (layer->hasMinstepMaxedges(0)) {
      l->setMinStepMaxEdges(layer->minstepMaxedges(0));
    }
  }

  if (layer->hasProtrusion()) {
    l->setProtrusion(dbdist(layer->protrusionWidth1()),
                     dbdist(layer->protrusionLength()),
                     dbdist(layer->protrusionWidth2()));
  }

  if (layer->hasDirection()) {
    dbTechLayerDir direction(layer->direction());
    l->setDirection(direction);
  }

  if (layer->hasResistance()) {  // routing layers
    l->setResistance(layer->resistance());
  } else if (layer->hasResistancePerCut()) {  // via layers
    l->setResistance(layer->resistancePerCut());
  }

  if (layer->hasCapacitance()) {
    l->setCapacitance(layer->capacitance());
  }

  if (layer->hasEdgeCap()) {
    l->setEdgeCapacitance(layer->edgeCap());
  }

  if (layer->hasWireExtension()) {
    l->setWireExtension(dbdist(layer->wireExtension()));
  }

  for (int i = 0; i < layer->numEnclosure(); ++i) {
    auto* rule = odb::dbTechLayerCutEnclosureRule::create(l);
    rule->setFirstOverhang(dbdist(layer->enclosureOverhang1(i)));
    rule->setSecondOverhang(dbdist(layer->enclosureOverhang2(i)));
    rule->setType(dbTechLayerCutEnclosureRule::DEFAULT);

    if (layer->hasEnclosureRule(i)) {
      const char* rule_name = layer->enclosureRule(i);
      if (strcasecmp(rule_name, "ABOVE") == 0) {
        rule->setAbove(true);
      } else if (strcasecmp(rule_name, "BELOW") == 0) {
        rule->setBelow(true);
      }
    }

    if (layer->hasEnclosureWidth(i)) {
      rule->setMinWidth(dbdist(layer->enclosureMinWidth(i)));
    }

    if (layer->hasEnclosureExceptExtraCut(i)) {
      rule->setExceptExtraCut(true);
      rule->setCutWithin(dbdist(layer->enclosureExceptExtraCut(i)));
    }

    if (layer->hasEnclosureMinLength(i)) {
      rule->setMinLength(dbdist(layer->enclosureMinLength(i)));
    }
  }

  dbSet<dbProperty> props = dbProperty::getProperties(l);
  if (!props.empty() && props.orderReversed()) {
    props.reverse();
  }

  layer_cnt_++;
}

void lefinReader::macroBegin(const char* name)
{
  master_ = nullptr;

  if (create_lib_) {
    if (lib_ == nullptr) {
      createLibrary();
    }

    master_ = lib_->findMaster(name);

    if (master_ == nullptr) {
      master_ = dbMaster::create(lib_, name);
    }
  }

  master_modified_ = false;
}

void lefinReader::macro(LefParser::lefiMacro* macro)
{
  if (master_ == nullptr) {
    return;
  }

  if (macro->hasClass()) {
    dbMasterType type(macro->macroClass());
    master_->setType(type);
  }

  for (int i = 0; i < macro->numProperties(); i++) {
    bool valid = true;
    if (!strcmp(macro->propName(i), "LEF58_CLASS")) {
      valid = lefMacroClassTypeParser::parse(macro->propValue(i), master_);
    } else if (!strcmp(macro->propName(i), "LEF58_EDGETYPE")) {
      lefMacroEdgeTypeParser(master_, this).parse(macro->propValue(i));
    } else {
      dbStringProperty::create(
          master_, macro->propName(i), macro->propValue(i));
    }

    if (!valid) {
      logger_->warn(utl::ODB,
                    2000,
                    "Cannot parse LEF property '{}' with value '{}'",
                    macro->propName(i),
                    macro->propValue(i));
    }
  }

  if (macro->hasEEQ()) {
    dbMaster* eeq = lib_->findMaster(macro->EEQ());
    if (eeq == nullptr) {
      logger_->warn(
          utl::ODB, 184, "cannot find EEQ for macro {}", macro->name());
    } else {
      master_->setEEQ(eeq);
    }
  }

  if (macro->hasLEQ()) {
    dbMaster* leq = lib_->findMaster(macro->LEQ());
    if (leq == nullptr) {
      logger_->warn(
          utl::ODB, 185, "cannot find LEQ for macro {}", macro->name());
    } else {
      master_->setLEQ(leq);
    }
  }

  if (macro->hasSize()) {
    int w = dbdist(macro->sizeX());
    int h = dbdist(macro->sizeY());
    master_->setWidth(w);
    master_->setHeight(h);
  }

  if (macro->hasOrigin()) {
    int x = dbdist(macro->originX());
    int y = dbdist(macro->originY());
    master_->setOrigin(x, y);
  }

  if (macro->hasSiteName()) {
    dbSite* site = findSite(macro->siteName());

    if (site == nullptr) {
      logger_->warn(utl::ODB,
                    186,
                    "macro {} references unknown site {}",
                    macro->name(),
                    macro->siteName());
    } else {
      master_->setSite(site);
    }
  }

  if (macro->hasXSymmetry()) {
    master_->setSymmetryX();
  }

  if (macro->hasYSymmetry()) {
    master_->setSymmetryY();
  }

  if (macro->has90Symmetry()) {
    master_->setSymmetryR90();
  }
}

void lefinReader::macroEnd(const char* /* unused: macroName */)
{
  if (master_) {
    master_->setFrozen();
    master_ = nullptr;
    master_cnt_++;
  }
}

void lefinReader::manufacturing(double num)
{
  tech_->setManufacturingGrid(dbdist(num));
}

void lefinReader::maxStackVia(
    LefParser::lefiMaxStackVia* /* unused: maxStack */)
{
}

void lefinReader::minFeature(LefParser::lefiMinFeature* /* unused: min */)
{
}

void lefinReader::nonDefault(LefParser::lefiNonDefault* rule)
{
  if (!create_tech_) {
    return;
  }

  dbTechNonDefaultRule* dbrule
      = dbTechNonDefaultRule::create(tech_, rule->name());

  if (dbrule == nullptr) {
    logger_->warn(
        utl::ODB, 187, "duplicate NON DEFAULT RULE ({})", rule->name());
    return;
  }

  int i;

  for (i = 0; i < rule->numLayers(); ++i) {
    dbTechLayer* dblayer = tech_->findLayer(rule->layerName(i));

    if (dblayer == nullptr) {
      logger_->warn(utl::ODB,
                    188,
                    "Invalid layer name {} in NON DEFAULT RULE {}",
                    rule->layerName(i),
                    rule->name());
      continue;
    }

    dbTechLayerRule* lr = dbTechLayerRule::create(dbrule, dblayer);

    if (rule->hasLayerWidth(i)) {
      lr->setWidth(dbdist(rule->layerWidth(i)));
    }

    if (rule->hasLayerSpacing(i)) {
      lr->setSpacing(dbdist(rule->layerSpacing(i)));
    }

    if (rule->hasLayerWireExtension(i)) {
      lr->setWireExtension(dbdist(rule->layerWireExtension(i)));
    }

    if (rule->hasLayerResistance(i)) {
      lr->setResistance(rule->layerResistance(i));
    }

    if (rule->hasLayerCapacitance(i)) {
      lr->setCapacitance(rule->layerCapacitance(i));
    }

    if (rule->hasLayerEdgeCap(i)) {
      lr->setEdgeCapacitance(rule->layerEdgeCap(i));
    }
  }

  for (i = 0; i < rule->numVias(); ++i) {
    via(rule->viaRule(i), dbrule);
  }

  for (i = 0; i < rule->numSpacingRules(); ++i) {
    LefParser::lefiSpacing* spacing = rule->spacingRule(i);
    dbTechLayer* l1 = tech_->findLayer(spacing->name1());
    if (l1 == nullptr) {
      logger_->warn(utl::ODB,
                    189,
                    "Invalid layer name {} in NONDEFAULT SPACING",
                    spacing->name1());
      return;
    }
    dbTechLayer* l2 = tech_->findLayer(spacing->name2());
    if (l2 == nullptr) {
      logger_->warn(utl::ODB,
                    190,
                    "Invalid layer name {} in NONDEFAULT SPACING",
                    spacing->name2());
      return;
    }
    dbTechSameNetRule* srule = dbTechSameNetRule::create(dbrule, l1, l2);

    if (spacing->hasStack()) {
      srule->setAllowStackedVias(true);
    }

    srule->setSpacing(dbdist(spacing->distance()));
  }

  // 5.6 additions
  if (rule->hasHardspacing()) {
    dbrule->setHardSpacing(true);
  }

  for (i = 0; i < rule->numUseVia(); ++i) {
    const char* vname = rule->viaName(i);
    dbTechVia* via = tech_->findVia(vname);

    if (via == nullptr) {
      logger_->warn(utl::ODB, 191, "error: undefined VIA {}", vname);
      ++errors_;
      continue;
    }

    dbrule->addUseVia(via);
  }

  for (i = 0; i < rule->numUseViaRule(); ++i) {
    const char* rname = rule->viaRuleName(i);
    dbTechViaGenerateRule* genrule = tech_->findViaGenerateRule(rname);

    if (genrule == nullptr) {
      logger_->warn(
          utl::ODB, 192, "error: undefined VIA GENERATE RULE {}", rname);
      ++errors_;
      continue;
    }

    dbrule->addUseViaRule(genrule);
  }

  for (i = 0; i < rule->numMinCuts(); ++i) {
    const char* lname = rule->cutLayerName(i);
    dbTechLayer* layer = tech_->findLayer(lname);

    if (layer == nullptr) {
      logger_->warn(utl::ODB, 193, "error: undefined LAYER {}", lname);
      ++errors_;
      continue;
    }

    dbrule->setMinCuts(layer, rule->numCuts(i));
  }
}

void lefinReader::obstruction(LefParser::lefiObstruction* obs)
{
  if ((master_ == nullptr) || (skip_obstructions_ == true)) {
    return;
  }

  LefParser::lefiGeometries* geometries = obs->geometries();

  if (geometries->numItems()) {
    addGeoms(master_, false, geometries);
    dbSet<dbPolygon> poly_obstructions = master_->getPolygonObstructions();

    // Reverse the stored order to match the created order.
    if (poly_obstructions.reversible() && poly_obstructions.orderReversed()) {
      poly_obstructions.reverse();
    }

    dbSet<dbBox> obstructions = master_->getObstructions();

    // Reverse the stored order to match the created order.
    if (obstructions.reversible() && obstructions.orderReversed()) {
      obstructions.reverse();
    }
  }
}

void lefinReader::pin(LefParser::lefiPin* pin)
{
  if (master_ == nullptr) {
    return;
  }

  dbIoType io_type;

  if (pin->hasDirection()) {
    if (strcasecmp(pin->direction(), "OUTPUT TRISTATE") == 0) {
      io_type = dbIoType(dbIoType::OUTPUT);
    } else {
      io_type = dbIoType(pin->direction());
    }
  }

  dbSigType sig_type;
  dbMTermShapeType shape_type;

  if (pin->LefParser::lefiPin::hasUse()) {
    sig_type = dbSigType(pin->use());
  }

  if (pin->LefParser::lefiPin::hasShape()) {
    shape_type = dbMTermShapeType(pin->shape());
  }

  dbMTerm* term = master_->findMTerm(pin->name());

  if (term == nullptr) {
    if (master_->isFrozen()) {
      std::string n = master_->getName();
      logger_->warn(
          utl::ODB,
          194,
          "Cannot add a new PIN ({}) to MACRO ({}), because the pins have "
          "already been defined. \n",
          pin->name(),
          n);
      return;
    }

    term = dbMTerm::create(master_, pin->name(), io_type, sig_type, shape_type);
  }

  //
  // Parse and install antenna info
  //
  int i;
  dbTechLayer* tply;

  if (pin->LefParser::lefiPin::hasAntennaPartialMetalArea()) {
    for (i = 0; i < pin->LefParser::lefiPin::numAntennaPartialMetalArea();
         i++) {
      tply = nullptr;
      if (pin->LefParser::lefiPin::antennaPartialMetalAreaLayer(i)) {
        tply = tech_->findLayer(
            pin->LefParser::lefiPin::antennaPartialMetalAreaLayer(i));
        if (!tply) {
          logger_->warn(
              utl::ODB,
              195,
              "Invalid layer name {} in antenna info for term {}",
              pin->LefParser::lefiPin::antennaPartialMetalAreaLayer(i),
              term->getName());
        }
      }
      term->addPartialMetalAreaEntry(
          pin->LefParser::lefiPin::antennaPartialMetalArea(i), tply);
    }
  }

  if (pin->LefParser::lefiPin::hasAntennaPartialMetalSideArea()) {
    for (i = 0; i < pin->LefParser::lefiPin::numAntennaPartialMetalSideArea();
         i++) {
      tply = nullptr;
      if (pin->LefParser::lefiPin::antennaPartialMetalSideAreaLayer(i)) {
        tply = tech_->findLayer(
            pin->LefParser::lefiPin::antennaPartialMetalSideAreaLayer(i));
        if (!tply) {
          logger_->warn(
              utl::ODB,
              196,
              "Invalid layer name {} in antenna info for term {}",
              pin->LefParser::lefiPin::antennaPartialMetalSideAreaLayer(i),
              term->getName());
        }
      }

      term->addPartialMetalSideAreaEntry(
          pin->LefParser::lefiPin::antennaPartialMetalSideArea(i), tply);
    }
  }

  if (pin->LefParser::lefiPin::hasAntennaPartialCutArea()) {
    for (i = 0; i < pin->LefParser::lefiPin::numAntennaPartialCutArea(); i++) {
      tply = nullptr;
      if (pin->LefParser::lefiPin::antennaPartialCutAreaLayer(i)) {
        tply = tech_->findLayer(
            pin->LefParser::lefiPin::antennaPartialCutAreaLayer(i));
        if (!tply) {
          logger_->warn(utl::ODB,
                        197,
                        "Invalid layer name {} in antenna info for term {}",
                        pin->LefParser::lefiPin::antennaPartialCutAreaLayer(i),
                        term->getName());
        }
      }

      term->addPartialCutAreaEntry(
          pin->LefParser::lefiPin::antennaPartialCutArea(i), tply);
    }
  }

  if (pin->LefParser::lefiPin::hasAntennaDiffArea()) {
    for (i = 0; i < pin->LefParser::lefiPin::numAntennaDiffArea(); i++) {
      tply = nullptr;
      if (pin->LefParser::lefiPin::antennaDiffAreaLayer(i)) {
        tply = tech_->findLayer(
            pin->LefParser::lefiPin::antennaDiffAreaLayer(i));
        if (!tply) {
          logger_->warn(utl::ODB,
                        198,
                        "Invalid layer name {} in antenna info for term {}",
                        pin->LefParser::lefiPin::antennaDiffAreaLayer(i),
                        term->getName());
        }
      }

      term->addDiffAreaEntry(pin->LefParser::lefiPin::antennaDiffArea(i), tply);
    }
  }

  int j;
  dbTechAntennaPinModel* curmodel;
  LefParser::lefiPinAntennaModel* curlefmodel;
  if (pin->LefParser::lefiPin::numAntennaModel() > 0) {
    // NOTE: Only two different oxides supported for now!
    for (i = 0; (i < pin->LefParser::lefiPin::numAntennaModel()) && (i < 2);
         i++) {
      curmodel = (i == 1) ? term->createOxide2AntennaModel()
                          : term->createDefaultAntennaModel();
      curlefmodel = pin->LefParser::lefiPin::antennaModel(i);

      if (curlefmodel->hasAntennaGateArea()) {
        for (j = 0; j < curlefmodel->numAntennaGateArea(); j++) {
          tply = nullptr;
          if (curlefmodel->antennaGateAreaLayer(j)) {
            tply = tech_->findLayer(curlefmodel->antennaGateAreaLayer(j));
            if (!tply) {
              logger_->warn(utl::ODB,
                            199,
                            "Invalid layer name {} in antenna info for term {}",
                            curlefmodel->antennaGateAreaLayer(j),
                            term->getName());
            }
          }
          curmodel->addGateAreaEntry(curlefmodel->antennaGateArea(j), tply);
        }
      }

      if (curlefmodel->hasAntennaMaxAreaCar()) {
        for (j = 0; j < curlefmodel->numAntennaMaxAreaCar(); j++) {
          tply = nullptr;
          if (curlefmodel->antennaMaxAreaCarLayer(j)) {
            tply = tech_->findLayer(curlefmodel->antennaMaxAreaCarLayer(j));
            if (!tply) {
              logger_->warn(utl::ODB,
                            200,
                            "Invalid layer name {} in antenna info for term {}",
                            curlefmodel->antennaMaxAreaCarLayer(j),
                            term->getName().c_str());
            }
          }
          curmodel->addMaxAreaCAREntry(curlefmodel->antennaMaxAreaCar(j), tply);
        }
      }

      if (curlefmodel->hasAntennaMaxSideAreaCar()) {
        for (j = 0; j < curlefmodel->numAntennaMaxSideAreaCar(); j++) {
          tply = nullptr;
          if (curlefmodel->antennaMaxSideAreaCarLayer(j)) {
            tply = tech_->findLayer(curlefmodel->antennaMaxSideAreaCarLayer(j));
            if (!tply) {
              logger_->warn(utl::ODB,
                            201,
                            "Invalid layer name {} in antenna info for term {}",
                            curlefmodel->antennaMaxSideAreaCarLayer(j),
                            term->getName());
            }
          }
          curmodel->addMaxSideAreaCAREntry(
              curlefmodel->antennaMaxSideAreaCar(j), tply);
        }
      }

      if (curlefmodel->hasAntennaMaxCutCar()) {
        for (j = 0; j < curlefmodel->numAntennaMaxCutCar(); j++) {
          tply = nullptr;
          if (curlefmodel->antennaMaxCutCarLayer(j)) {
            tply = tech_->findLayer(curlefmodel->antennaMaxCutCarLayer(j));
            if (!tply) {
              logger_->warn(utl::ODB,
                            202,
                            "Invalid layer name {} in antenna info for term {}",
                            curlefmodel->antennaMaxCutCarLayer(j),
                            term->getName());
            }
          }
          curmodel->addMaxCutCAREntry(curlefmodel->antennaMaxCutCar(j), tply);
        }
      }
    }
  }

  bool created_mpins = false;
  int numPorts = pin->LefParser::lefiPin::numPorts();
  for (i = 0; i < numPorts; i++) {
    LefParser::lefiGeometries* geometries = pin->LefParser::lefiPin::port(i);
    if (geometries->numItems()) {
      dbMPin* dbpin = dbMPin::create(term);
      created_mpins = true;
      addGeoms(dbpin, true, geometries);

      dbSet<dbPolygon> poly_geoms = dbpin->getPolygonGeometry();
      if (poly_geoms.reversible() && poly_geoms.orderReversed()) {
        poly_geoms.reverse();
      }

      dbSet<dbBox> geoms = dbpin->getGeometry();
      if (geoms.reversible() && geoms.orderReversed()) {
        geoms.reverse();
      }
    }
  }

  if (created_mpins)  // created pins
  {
    dbSet<dbMPin> pins = term->getMPins();
    if (pins.reversible() && pins.orderReversed()) {
      pins.reverse();
    }
  }
}

void lefinReader::propDefBegin(void* /* unused: ptr */)
{
}

void lefinReader::propDef(LefParser::lefiProp* prop)
{
  if (std::string(prop->propName()) == "LEF58_METALWIDTHVIAMAP") {
    auto parser = MetalWidthViaMapParser(tech_, this, incomplete_props_);
    parser.parse(prop->string());
  } else if (std::string(prop->propName()) == "LEF58_CELLEDGESPACINGTABLE") {
    auto parser = CellEdgeSpacingTableParser(tech_, this);
    parser.parse(prop->string());
  }
}

void lefinReader::propDefEnd(void* /* unused: ptr */)
{
}

void lefinReader::site(LefParser::lefiSite* lefsite)
{
  if (!create_lib_) {
    return;
  }

  if (lib_ == nullptr) {
    createLibrary();
  }

  dbSite* site = lib_->findSite(lefsite->name());

  if (site) {
    return;
  }

  for (dbLib* lib : db_->getLibs()) {
    site = lib->findSite(lefsite->name());
    if (site) {
      logger_->info(utl::ODB,
                    394,
                    "Duplicate site {} in {} already seen in {}",
                    lefsite->name(),
                    lib_->getName(),
                    lib->getName());
      return;
    }
  }

  site = dbSite::create(lib_, lefsite->name());

  if (lefsite->hasSize()) {
    site->setWidth(dbdist(lefsite->sizeX()));
    site->setHeight(dbdist(lefsite->sizeY()));
  }

  if (lefsite->hasXSymmetry()) {
    site->setSymmetryX();
  }

  if (lefsite->hasYSymmetry()) {
    site->setSymmetryY();
  }

  if (lefsite->has90Symmetry()) {
    site->setSymmetryR90();
  }

  if (lefsite->hasClass()) {
    site->setClass(dbSiteClass(lefsite->siteClass()));
  }

  if (lefsite->hasRowPattern()) {
    auto row_pattern = lefsite->getRowPatterns();
    std::vector<dbSite::OrientedSite> converted_row_pattern;
    converted_row_pattern.reserve(row_pattern.size());
    for (auto& row : row_pattern) {
      dbOrientType orient(row.second.c_str());
      auto child_site = findSite(row.first.c_str());
      if (!child_site) {
        ++errors_;
        logger_->warn(
            utl::ODB, 208, "Row pattern site {} can't be found", row.first);
        continue;
      }
      converted_row_pattern.push_back({child_site, orient});
    }
    site->setRowPattern(converted_row_pattern);
  }
}

void lefinReader::spacingBegin(void* /* unused: ptr */)
{
}

void lefinReader::spacing(LefParser::lefiSpacing* spacing)
{
  if (create_tech_ == false) {
    return;
  }

  dbTechLayer* l1 = tech_->findLayer(spacing->name1());
  if (l1 == nullptr) {
    logger_->warn(
        utl::ODB, 203, "Invalid layer name {} in SPACING", spacing->name1());
    return;
  }
  dbTechLayer* l2 = tech_->findLayer(spacing->name2());
  if (l2 == nullptr) {
    logger_->warn(
        utl::ODB, 204, "Invalid layer name {} in SPACING", spacing->name2());
    return;
  }
  dbTechSameNetRule* rule = dbTechSameNetRule::create(l1, l2);

  if (rule == nullptr) {
    return;
  }

  if (spacing->hasStack()) {
    rule->setAllowStackedVias(true);
  }

  rule->setSpacing(dbdist(spacing->distance()));
}

void lefinReader::spacingEnd(void* /* unused: ptr */)
{
}

void lefinReader::timing(LefParser::lefiTiming* /* unused: timing */)
{
}

void lefinReader::units(LefParser::lefiUnits* unit)
{
  if (unit->hasDatabase()) {
    lef_units_ = (int) unit->databaseNumber();

    if (create_tech_ && !override_lef_dbu_) {
      // historically the database was always stored in nm
      setDBUPerMicron(std::max(lef_units_, 1000));
      if (db_->getDbuPerMicron() == 0) {
        db_->setDbuPerMicron(dbu_per_micron_);
      }
      tech_->setLefUnits(lef_units_);
    }

    if (lef_units_ > dbu_per_micron_) {
      ++errors_;
      logger_->warn(
          utl::ODB,
          205,
          "The LEF UNITS DATABASE MICRON convert factor ({}) is greater than "
          "the database units per micron ({}) of the current technology.",
          lef_units_,
          dbu_per_micron_);
    }
  }
}

namespace {
bool isValidDBUPerMicron(int dbu)
{
  switch (dbu) {
    case 1000:
    case 2000:
    case 4000:
    case 8000:
    case 10000:
    case 20000:
      return true;
    default:
      return false;
  }
}
}  // namespace

void lefinReader::setDBUPerMicron(int dbu)
{
  if (!isValidDBUPerMicron(dbu)) {
    ++errors_;
    logger_->warn(utl::ODB,
                  400,
                  "error: invalid dbu-per-micron value {}; valid units (1000, "
                  "2000, 4000, 8000, 10000, 20000)",
                  dbu);

    return;
  }
  if (db_->getDbuPerMicron() != 0) {
    if (dbu > db_->getDbuPerMicron()) {
      ++errors_;
      logger_->warn(
          utl::ODB,
          401,
          "The LEF UNITS DATABASE MICRON convert factor ({}) is greater than "
          "the database units per micron ({}) of the current database.",
          dbu,
          db_->getDbuPerMicron());
    }
    if (db_->getDbuPerMicron() % dbu != 0) {
      ++errors_;
      logger_->warn(utl::ODB,
                    402,
                    "The LEF UNITS DATABASE MICRON convert factor ({}) is "
                    "not a multiplier of the database units per micron ({}) of "
                    "the current database.",
                    dbu,
                    db_->getDbuPerMicron());
    }
    dbu = db_->getDbuPerMicron();
  }
  dist_factor_ = dbu;
  dbu_per_micron_ = dbu;
  area_factor_ = dbu_per_micron_ * dbu_per_micron_;
}

void lefinReader::useMinSpacing(LefParser::lefiUseMinSpacing* spacing)
{
  if (!strncasecmp(spacing->name(), "PIN", 3)) {
    tech_->setUseMinSpacingPin(dbOnOffType(spacing->value()));
  } else if (!strncasecmp(spacing->name(), "OBS", 3)) {
    tech_->setUseMinSpacingObs(dbOnOffType(spacing->value()));
  } else {
    logger_->warn(utl::ODB,
                  207,
                  "Unknown object type for USEMINSPACING: {}",
                  spacing->name());
  }
}

void lefinReader::version(double num)
{
  tech_->setLefVersion(num);
}

void lefinReader::via(LefParser::lefiVia* via, dbTechNonDefaultRule* rule)
{
  if (tech_->findVia(via->name())) {
    debugPrint(logger_,
               utl::ODB,
               "lefinReader",
               1,
               "VIA: duplicate VIA ({}) ignored...",
               via->name());
    return;
  }

  dbTechVia* v;

  if (rule) {
    v = dbTechVia::create(rule, via->name());
  } else {
    v = dbTechVia::create(tech_, via->name());
  }

  for (int iii = 0; iii < via->numProperties(); iii++) {
    dbStringProperty::create(v, via->propName(iii), via->propValue(iii));
  }

  if (via->hasDefault()) {
    v->setDefault();
  }

  if (via->hasTopOfStack()) {
    v->setTopOfStack();
  }

  if (via->hasResistance()) {
    v->setResistance(via->resistance());
  }

  if (via->numLayers() > 0) {
    int i;
    int j;

    for (i = 0; i < via->LefParser::lefiVia::numLayers(); i++) {
      dbTechLayer* l = tech_->findLayer(via->layerName(i));

      if (l == nullptr) {
        logger_->warn(utl::ODB,
                      209,
                      "VIA: undefined layer ({}) in VIA ({})",
                      via->layerName(i),
                      via->name());

        continue;
      }

      for (j = 0; j < via->numRects(i); j++) {
        int xlo = dbdist(via->xl(i, j));
        int xhi = dbdist(via->xh(i, j));
        int ylo = dbdist(via->yl(i, j));
        int yhi = dbdist(via->yh(i, j));
        dbBox::create(v, l, xlo, ylo, xhi, yhi);
      }
    }

    dbSet<dbBox> boxes = v->getBoxes();
    // Reverse the stored order to match the created order.
    if (boxes.reversible() && boxes.orderReversed()) {
      boxes.reverse();
    }
  }

  // 5.6 VIA RULE
  if (via->hasViaRule()) {
    dbTechViaGenerateRule* gen_rule
        = tech_->findViaGenerateRule(via->viaRuleName());

    if (gen_rule == nullptr) {
      logger_->warn(utl::ODB,
                    210,
                    "error: missing VIA GENERATE rule {}",
                    via->viaRuleName());
      ++errors_;
      return;
    }

    v->setViaGenerateRule(gen_rule);
    dbViaParams P;
    P.setXCutSize(dbdist(via->xCutSize()));
    P.setYCutSize(dbdist(via->yCutSize()));

    dbTechLayer* bot = tech_->findLayer(via->botMetalLayer());

    if (bot == nullptr) {
      logger_->warn(
          utl::ODB, 211, "error: missing LAYER {}", via->botMetalLayer());
      ++errors_;
      return;
    }

    dbTechLayer* cut = tech_->findLayer(via->cutLayer());

    if (cut == nullptr) {
      logger_->warn(utl::ODB, 212, "error: missing LAYER {}", via->cutLayer());
      ++errors_;
      return;
    }

    dbTechLayer* top = tech_->findLayer(via->topMetalLayer());

    if (top == nullptr) {
      logger_->warn(
          utl::ODB, 213, "error: missing LAYER {}", via->topMetalLayer());
      ++errors_;
      return;
    }

    P.setTopLayer(top);
    P.setCutLayer(cut);
    P.setBottomLayer(bot);
    P.setXCutSpacing(dbdist(via->xCutSpacing()));
    P.setYCutSpacing(dbdist(via->yCutSpacing()));
    P.setXBottomEnclosure(dbdist(via->xBotEnc()));
    P.setYBottomEnclosure(dbdist(via->yBotEnc()));
    P.setXTopEnclosure(dbdist(via->xTopEnc()));
    P.setYTopEnclosure(dbdist(via->yTopEnc()));

    if (via->hasRowCol()) {
      P.setNumCutRows(via->numCutRows());
      P.setNumCutCols(via->numCutCols());
    }

    if (via->hasOrigin()) {
      P.setXOrigin(dbdist(via->xOffset()));
      P.setYOrigin(dbdist(via->yOffset()));
    }

    if (via->hasOffset()) {
      P.setXBottomEnclosure(dbdist(via->xBotOffset()));
      P.setYBottomEnclosure(dbdist(via->yBotOffset()));
      P.setXTopEnclosure(dbdist(via->xTopOffset()));
      P.setYTopEnclosure(dbdist(via->yTopOffset()));
    }

    v->setViaParams(P);

    if (via->hasCutPattern()) {
      v->setPattern(via->cutPattern());
    }
  }

  via_cnt_++;
}

void lefinReader::viaRule(LefParser::lefiViaRule* viaRule)
{
  if (viaRule->hasGenerate()) {
    viaGenerateRule(viaRule);
    return;
  }

  const char* name = viaRule->name();

  dbTechViaRule* rule = dbTechViaRule::create(tech_, name);

  if (rule == nullptr) {
    logger_->warn(utl::ODB, 214, "duplicate VIARULE ({}) ignoring...", name);
    return;
  }

  int idx;
  for (idx = 0; idx < viaRule->numLayers(); ++idx) {
    LefParser::lefiViaRuleLayer* leflay = viaRule->layer(idx);
    dbTechLayer* layer = tech_->findLayer(leflay->name());

    if (layer == nullptr) {
      logger_->warn(utl::ODB,
                    215,
                    "error: VIARULE ({}) undefined layer {}",
                    name,
                    leflay->name());
      ++errors_;
      return;
    }

    dbTechViaLayerRule* layrule
        = dbTechViaLayerRule::create(tech_, rule, layer);

    if (viaRule->layer(idx)->hasDirection()) {
      if (viaRule->layer(idx)->isVertical()) {
        layrule->setDirection(dbTechLayerDir::VERTICAL);
      } else if (viaRule->layer(idx)->isHorizontal()) {
        layrule->setDirection(dbTechLayerDir::HORIZONTAL);
      }
    }

    if (viaRule->layer(idx)->hasWidth()) {
      int minW = dbdist(viaRule->layer(idx)->widthMin());
      int maxW = dbdist(viaRule->layer(idx)->widthMax());
      layrule->setWidth(minW, maxW);
    }
  }

  for (idx = 0; idx < viaRule->numVias(); ++idx) {
    dbTechVia* via = tech_->findVia(viaRule->viaName(idx));

    if (via == nullptr) {
      logger_->warn(utl::ODB,
                    216,
                    "error: undefined VIA {} in VIARULE {}",
                    viaRule->viaName(idx),
                    name);
      ++errors_;
    } else {
      rule->addVia(via);
    }
  }
}

void lefinReader::viaGenerateRule(LefParser::lefiViaRule* viaRule)
{
  const char* name = viaRule->name();
  dbTechViaGenerateRule* rule
      = dbTechViaGenerateRule::create(tech_, name, viaRule->hasDefault());

  if (rule == nullptr) {
    logger_->warn(utl::ODB, 217, "duplicate VIARULE ({}) ignoring...", name);
    return;
  }

  int idx;
  for (idx = 0; idx < viaRule->numLayers(); ++idx) {
    LefParser::lefiViaRuleLayer* leflay = viaRule->layer(idx);
    dbTechLayer* layer = tech_->findLayer(leflay->name());

    if (layer == nullptr) {
      logger_->warn(utl::ODB,
                    218,
                    "error: VIARULE ({}) undefined layer {}",
                    name,
                    leflay->name());
      ++errors_;
      return;
    }

    dbTechViaLayerRule* layrule
        = dbTechViaLayerRule::create(tech_, rule, layer);

    if (viaRule->layer(idx)->hasDirection()) {
      if (viaRule->layer(idx)->isVertical()) {
        layrule->setDirection(dbTechLayerDir::VERTICAL);
      } else if (viaRule->layer(idx)->isHorizontal()) {
        layrule->setDirection(dbTechLayerDir::HORIZONTAL);
      }
    }

    if (viaRule->layer(idx)->hasEnclosure()) {
      int overhang1 = dbdist(viaRule->layer(idx)->enclosureOverhang1());
      int overhang2 = dbdist(viaRule->layer(idx)->enclosureOverhang2());
      layrule->setEnclosure(overhang1, overhang2);
    }

    if (viaRule->layer(idx)->hasWidth()) {
      int minW = dbdist(viaRule->layer(idx)->widthMin());
      int maxW = dbdist(viaRule->layer(idx)->widthMax());
      layrule->setWidth(minW, maxW);
    }

    if (viaRule->layer(idx)->hasOverhang()) {
      int overhang = dbdist(viaRule->layer(idx)->overhang());
      layrule->setOverhang(overhang);
    }

    if (viaRule->layer(idx)->hasMetalOverhang()) {
      int overhang = dbdist(viaRule->layer(idx)->metalOverhang());
      layrule->setMetalOverhang(overhang);
    }

    if (viaRule->layer(idx)->hasRect()) {
      int xMin = dbdist(viaRule->layer(idx)->xl());
      int yMin = dbdist(viaRule->layer(idx)->yl());
      int xMax = dbdist(viaRule->layer(idx)->xh());
      int yMax = dbdist(viaRule->layer(idx)->yh());
      Rect r(xMin, yMin, xMax, yMax);
      layrule->setRect(r);
    }

    if (viaRule->layer(idx)->hasSpacing()) {
      int x_spacing = dbdist(viaRule->layer(idx)->spacingStepX());
      int y_spacing = dbdist(viaRule->layer(idx)->spacingStepY());
      layrule->setSpacing(x_spacing, y_spacing);
    }

    if (viaRule->layer(idx)->hasResistance()) {
      layrule->setResistance(viaRule->layer(idx)->resistance());
    }
  }
}

void lefinReader::done(void* /* unused: ptr */)
{
}

void lefinReader::lineNumber(int lineNo)
{
  logger_->info(utl::ODB, 221, "{} lines parsed!", lineNo);
}

bool lefinReader::readLef(const char* lef_file)
{
  try {
    return readLefInner(lef_file);
  } catch (...) {
    logger_->info(utl::ODB, 222, "While reading LEF file: {}", lef_file);
    throw;
  }
}

bool lefinReader::readLefInner(const char* lef_file)
{
  bool r = lefin_parse(this, logger_, lef_file);
  for (auto& [obj, name] : incomplete_props_) {
    auto layer = tech_->findLayer(name.c_str());
    switch (obj->getObjectType()) {
      case odb::dbTechLayerCutSpacingRuleObj: {
        odb::dbTechLayerCutSpacingRule* cutSpacingRule
            = (odb::dbTechLayerCutSpacingRule*) obj;
        if (layer != nullptr) {
          cutSpacingRule->setSecondLayer(layer);
        } else {
          logger_->warn(utl::ODB,
                        277,
                        "dropping LEF58_SPACING rule for cut layer {} for "
                        "referencing undefined layer {}",
                        cutSpacingRule->getTechLayer()->getName(),
                        name);
          odb::dbTechLayerCutSpacingRule::destroy(cutSpacingRule);
        }
        break;
      }
      case odb::dbTechLayerCutSpacingTableDefRuleObj: {
        odb::dbTechLayerCutSpacingTableDefRule* cutSpacingTableRule
            = (odb::dbTechLayerCutSpacingTableDefRule*) obj;
        if (layer != nullptr) {
          cutSpacingTableRule->setSecondLayer(layer);
        } else {
          logger_->warn(utl::ODB,
                        280,
                        "dropping LEF58_SPACINGTABLE rule for cut layer {} for "
                        "referencing undefined layer {}",
                        cutSpacingTableRule->getTechLayer()->getName(),
                        name);
          odb::dbTechLayerCutSpacingTableDefRule::destroy(cutSpacingTableRule);
        }
        break;
      }
      case odb::dbMetalWidthViaMapObj: {
        odb::dbMetalWidthViaMap* metalWidthViaMap
            = (odb::dbMetalWidthViaMap*) obj;
        if (layer != nullptr) {
          metalWidthViaMap->setCutLayer(layer);
        } else {
          logger_->warn(utl::ODB,
                        356,
                        "dropping LEF58_METALWIDTHVIAMAP for "
                        "referencing undefined layer {}",
                        name);
          odb::dbMetalWidthViaMap::destroy(metalWidthViaMap);
        }
        break;
      }
      case odb::dbTechLayerAreaRuleObj: {
        odb::dbTechLayerAreaRule* areaRule = (odb::dbTechLayerAreaRule*) obj;
        if (layer != nullptr) {
          areaRule->setTrimLayer(layer);
        } else {
          logger_->warn(
              utl::ODB,
              361,
              "dropping LEF58_AREA for referencing undefined layer {}",
              name);
          odb::dbTechLayerAreaRule::destroy(areaRule);
        }
        break;
      }
      default:
        logger_->error(utl::ODB,
                       246,
                       "unknown incomplete layer prop of type {}",
                       obj->getTypeName());
        break;
    }
  }
  incomplete_props_.clear();

  std::string p = lef_file;
  std::vector<std::string> parts;

  if (layer_cnt_ > 0) {
    std::ostringstream ss;
    ss << layer_cnt_ << " layers";
    parts.push_back(ss.str());
  }

  if (via_cnt_ > 0) {
    std::ostringstream ss;
    ss << via_cnt_ << " vias";
    parts.push_back(ss.str());
  }

  if (master_cnt_ > 0) {
    std::ostringstream ss;
    ss << master_cnt_ << " library cells";
    parts.push_back(ss.str());
  }

  std::string message = "LEF file: " + p;
  if (!parts.empty()) {
    message += ", created ";
    for (size_t i = 0; i < parts.size(); ++i) {
      message += parts[i];
      if (i != parts.size() - 1) {
        message += ", ";
      }
    }
  }

  logger_->info(utl::ODB, 227, message);
  return r;
}

dbTech* lefinReader::createTech(const char* name, const char* lef_file)
{
  lefrSetRelaxMode();
  init();

  tech_ = dbTech::create(db_, name);
  create_tech_ = true;

  if (!readLef(lef_file) || errors_ != 0) {
    dbTech::destroy(tech_);
    logger_->error(
        utl::ODB, 288, "LEF data from {} is discarded due to errors", lef_file);
  }
  if (db_->getDbuPerMicron() == 0) {
    db_->setDbuPerMicron(dbu_per_micron_);
  }
  db_->triggerPostReadLef(tech_, nullptr);

  return tech_;
}

dbLib* lefinReader::createLib(dbTech* tech,
                              const char* name,
                              const char* lef_file)
{
  lefrSetRelaxMode();
  init();

  tech_ = tech;

  if (tech_ == nullptr) {
    logger_->warn(utl::ODB, 228, "Error: technology does not exists");
    return nullptr;
  }

  if (db_->findLib(name)) {
    logger_->warn(utl::ODB, 229, "Error: library ({}) already exists", name);
    return nullptr;
  };

  setDBUPerMicron(tech_->getDbUnitsPerMicron());
  lib_name_ = name;
  create_lib_ = true;

  if (!readLef(lef_file) || errors_ != 0) {
    if (lib_) {
      dbLib::destroy(lib_);
    }
    logger_->error(
        utl::ODB, 292, "LEF data from {} is discarded due to errors", lef_file);
  }
  if (db_->getDbuPerMicron() == 0) {
    db_->setDbuPerMicron(dbu_per_micron_);
  }
  db_->triggerPostReadLef(tech_, lib_);
  return lib_;
}

dbLib* lefinReader::createTechAndLib(const char* tech_name,
                                     const char* lib_name,
                                     const char* lef_file)
{
  lefrSetRelaxMode();
  init();

  if (db_->findLib(lib_name)) {
    logger_->warn(
        utl::ODB, 230, "Error: library ({}) already exists", lib_name);
    return nullptr;
  };

  if (db_->findTech(tech_name)) {
    logger_->warn(
        utl::ODB, 231, "Error: technology {} already exists", tech_name);
    ++errors_;
    return nullptr;
  };

  tech_ = dbTech::create(db_, tech_name);
  lib_name_ = lib_name;
  create_lib_ = true;
  create_tech_ = true;

  if (!readLef(lef_file) || errors_ != 0) {
    if (lib_) {
      dbLib::destroy(lib_);
    }
    dbTech::destroy(tech_);
    logger_->error(
        utl::ODB, 289, "LEF data from {} is discarded due to errors", lef_file);
  }

  dbSet<dbTechNonDefaultRule> rules = tech_->getNonDefaultRules();

  if (rules.orderReversed()) {
    rules.reverse();
  }
  if (db_->getDbuPerMicron() == 0) {
    db_->setDbuPerMicron(dbu_per_micron_);
  }
  db_->triggerPostReadLef(tech_, lib_);

  return lib_;
}

bool lefinReader::updateLib(dbLib* lib, const char* lef_file)
{
  lefrSetRelaxMode();
  init();
  tech_ = lib->getTech();
  lib_ = lib;
  create_lib_ = true;
  setDBUPerMicron(tech_->getDbUnitsPerMicron());

  if (!readLef(lef_file)) {
    return false;
  }

  return errors_ == 0;
}

//
// TODO: Recover gracefully from any update errors
//
bool lefinReader::updateTechAndLib(dbLib* lib, const char* lef_file)
{
  lefrSetRelaxMode();

  init();
  lib_ = lib;
  tech_ = lib->getTech();
  create_lib_ = true;
  create_tech_ = true;
  dbu_per_micron(tech_->getDbUnitsPerMicron());  // set override-flag, because
                                                 // the tech is being updated.

  if (!readLef(lef_file)) {
    return false;
  }

  return errors_ == 0;
}

bool lefinReader::updateTech(dbTech* tech, const char* lef_file)
{
  lefrSetRelaxMode();
  init();
  tech_ = tech;
  create_tech_ = true;
  dbu_per_micron(tech_->getDbUnitsPerMicron());  // set override-flag, because
                                                 // the tech is being updated.

  if (!readLef(lef_file)) {
    return false;
  }

  return errors_ == 0;
}

lefin::lefin(dbDatabase* db,
             utl::Logger* logger,
             bool ignore_non_routing_layers)
{
  reader_ = new lefinReader(db, logger, ignore_non_routing_layers);
}

lefin::~lefin()
{
  delete reader_;
}

int lefin::dbdist(double value)
{
  return reader_->dbdist(value);
}

dbTech* lefin::createTech(const char* name, const char* lef_file)
{
  std::lock_guard<std::mutex> lock(lef_mutex_);
  return reader_->createTech(name, lef_file);
}

dbLib* lefin::createLib(dbTech* tech, const char* name, const char* lef_file)
{
  std::lock_guard<std::mutex> lock(lef_mutex_);
  return reader_->createLib(tech, name, lef_file);
}

dbLib* lefin::createTechAndLib(const char* tech_name,
                               const char* lib_name,
                               const char* lef_file)
{
  std::lock_guard<std::mutex> lock(lef_mutex_);
  return reader_->createTechAndLib(tech_name, lib_name, lef_file);
}

bool lefin::updateLib(dbLib* lib, const char* lef_file)
{
  std::lock_guard<std::mutex> lock(lef_mutex_);
  return reader_->updateLib(lib, lef_file);
}

bool lefin::updateTech(dbTech* tech, const char* lef_file)
{
  std::lock_guard<std::mutex> lock(lef_mutex_);
  return reader_->updateTech(tech, lef_file);
}

bool lefin::updateTechAndLib(dbLib* lib, const char* lef_file)
{
  std::lock_guard<std::mutex> lock(lef_mutex_);
  return reader_->updateTechAndLib(lib, lef_file);
}

}  // namespace odb
