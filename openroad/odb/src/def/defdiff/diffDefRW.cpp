// *****************************************************************************
// *****************************************************************************
// Copyright 2014 - 2015, Cadence Design Systems
//
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8.
//
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
//
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
//
//  $Author$
//  $Revision$
//  $Date$
//  $State:  $
// *****************************************************************************
// *****************************************************************************

// This program is the diffDef core program.  It has all the callback
// routines and write it out to a temporary file.

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifndef WIN32
#include <unistd.h>
#endif /* not WIN32 */
#include "defiComponent.hpp"
#include "defiDefs.hpp"
#include "defiNet.hpp"
#include "defiPath.hpp"
#include "defrReader.hpp"

char defaultName[64];
char defaultOut[64];
static int ignorePE = 0;
static int ignoreRN = 0;
static int ignoreVN = 0;
static int netSeCmp = 0;

// Global variables
FILE* fout;
void* userData;
int numObjs;
int isSumSet;    // to keep track if within SUM
int isProp = 0;  // for PROPDEF
int begOperand;  // to keep track for constraint, to print - as the 1st char
static double curVer = 5.7;

// TX_DIR:TRANSLATION ON

void dataError()
{
  fprintf(fout, "ERROR: returned user data is not correct!\n");
}

void checkType(defrCallbackType_e c)
{
  if (c >= 0 && c <= defrDesignEndCbkType) {
    // OK
  } else {
    fprintf(fout, "ERROR: callback type is out of bounds!\n");
  }
}

// 05/24/2001 - Wanda da Rosa.  PCR 373170
// This function is added due to the rounding between machines are
// different.  For a 5, solaries will round down while hppa will roundup.
// This function will make sure it round up for all the machine
double checkDouble(double num)
{
  int64_t tempNum;
  if ((num > 1000004) || (num < -1000004)) {
    tempNum = (int64_t) num;
    if ((tempNum % 5) == 0) {
      return num + 3;
    }
  }
  return num;
}

int compMSL(defrCallbackType_e c,
            defiComponentMaskShiftLayer* co,
            defiUserData ud)
{
  int i;

  checkType(c);
  if (ud != userData) {
    dataError();
  }

  if (co->numMaskShiftLayers()) {
    fprintf(fout, "\nCOMPONENTMASKSHIFT ");

    for (i = 0; i < co->numMaskShiftLayers(); i++) {
      fprintf(fout, "%s ", co->maskShiftLayer(i));
    }
    fprintf(fout, ";\n");
  }

  return 0;
}

// Component
int compf(defrCallbackType_e c, defiComponent* co, defiUserData ud)
{
  int i;

  checkType(c);
  if (ud != userData) {
    dataError();
  }
  //  missing GENERATE, FOREIGN
  fprintf(fout, "COMP %s %s", co->id(), co->name());
  if (co->hasNets()) {
    for (i = 0; i < co->numNets(); i++) {
      fprintf(fout, " %s", co->net(i));
    }
    fprintf(fout, "\n");
  } else {
    fprintf(fout, "\n");
  }
  if (co->isFixed()) {
    fprintf(fout,
            "COMP %s FIXED ( %d %d ) %s\n",
            co->id(),
            co->placementX(),
            co->placementY(),
            co->placementOrientStr());
  }
  if (co->isCover()) {
    fprintf(fout,
            "COMP %s COVER ( %d %d ) %s\n",
            co->id(),
            co->placementX(),
            co->placementY(),
            co->placementOrientStr());
  }
  if (co->isPlaced()) {
    fprintf(fout,
            "COMP %s PLACED ( %d %d ) %s\n",
            co->id(),
            co->placementX(),
            co->placementY(),
            co->placementOrientStr());
  }
  if (co->isUnplaced()) {
    fprintf(fout, "COMP %s UNPLACED\n", co->id());
  }
  if (co->hasSource()) {
    fprintf(fout, "COMP %s SOURCE %s\n", co->id(), co->source());
  }
  if (co->hasGenerate()) {
    fprintf(fout,
            "COMP %s GENERATE %s %s\n",
            co->id(),
            co->generateName(),
            co->macroName());
  }
  if (co->hasHalo()) {
    int left, bottom, right, top;
    co->haloEdges(&left, &bottom, &right, &top);
    fprintf(fout, "COMP %s HALO", co->id());
    if (co->hasHaloSoft()) {
      fprintf(fout, " SOFT");
    }
    fprintf(fout, " %d %d %d %d\n", left, bottom, right, top);
  }
  if (co->hasRouteHalo()) {
    fprintf(fout,
            "COMP %s ROUTEHALO %d %s %s\n",
            co->id(),
            co->haloDist(),
            co->minLayer(),
            co->maxLayer());
  }
  if (co->hasForeignName()) {
    fprintf(fout,
            "COMP %s FOREIGN %s %d %d %s\n",
            co->id(),
            co->foreignName(),
            co->foreignX(),
            co->foreignY(),
            co->foreignOri());
  }
  if (co->hasWeight()) {
    fprintf(fout, "COMP %s WEIGHT %d\n", co->id(), co->weight());
  }
  if (co->hasEEQ()) {
    fprintf(fout, "COMP %s EEQMASTER %s\n", co->id(), co->EEQ());
  }
  if (co->hasRegionName()) {
    fprintf(fout, "COMP %s REGION %s\n", co->id(), co->regionName());
  }
  if (co->hasRegionBounds()) {
    int *xl, *yl, *xh, *yh;
    int size;
    co->regionBounds(&size, &xl, &yl, &xh, &yh);
    for (i = 0; i < size; i++) {
      fprintf(fout,
              "COMP %s REGION ( %d %d ) ( %d %d )\n",
              co->id(),
              xl[i],
              yl[i],
              xh[i],
              yh[i]);
    }
  }
  if (co->maskShiftSize()) {
    fprintf(fout, "MASKSHIFT ");

    for (int ii = co->maskShiftSize() - 1; ii >= 0; ii--) {
      fprintf(fout, "%d", co->maskShift(ii));
    }
    fprintf(fout, "\n");
  }
  if (co->numProps()) {
    for (i = 0; i < co->numProps(); i++) {
      fprintf(fout,
              "COMP %s PROP %s %s ",
              co->id(),
              co->propName(i),
              co->propValue(i));
      switch (co->propType(i)) {
        case 'R':
          fprintf(fout, "REAL ");
          break;
        case 'I':
          fprintf(fout, "INT ");
          break;
        case 'S':
          fprintf(fout, "STR ");
          break;
        case 'Q':
          fprintf(fout, "QSTR ");
          break;
        case 'N':
          fprintf(fout, "NUM ");
          break;
      }
      fprintf(fout, "\n");
    }
  }

  --numObjs;
  return 0;
}

// Net
int netf(defrCallbackType_e c, defiNet* net, defiUserData ud)
{
  // For net and special net.
  int i, j, k, w, x, y, z;
  int px = 0;
  int py = 0;
  int pz = 0;
  defiPath* p;
  defiSubnet* s;
  int path;
  defiVpin* vpin;
  defiShield* noShield;
  defiWire* wire;
  int nline;
  const char* layerName = "N/A";

  checkType(c);
  if (ud != userData) {
    dataError();
  }
  if (c != defrNetCbkType) {
    fprintf(fout, "BOGUS NET TYPE  ");
  }
  if (net->pinIsMustJoin(0)) {
    fprintf(fout, "NET MUSTJOIN ");
  } else {
    fprintf(fout, "NET %s ", net->name());
  }

  // compName & pinName
  for (i = 0; i < net->numConnections(); i++) {
    fprintf(fout,
            "\nNET %s ( %s %s ) ",
            net->name(),
            net->instance(i),
            net->pin(i));
  }

  if (net->hasNonDefaultRule()) {
    fprintf(
        fout, "\nNET %s NONDEFAULTRULE %s", net->name(), net->nonDefaultRule());
  }

  for (i = 0; i < net->numVpins(); i++) {
    vpin = net->vpin(i);
    fprintf(fout, "\nNET %s %s", net->name(), vpin->name());
    if (vpin->layer()) {
      fprintf(fout, " %s", vpin->layer());
    }
    fprintf(
        fout, " %d %d %d %d", vpin->xl(), vpin->yl(), vpin->xh(), vpin->yh());
    if (vpin->status() != ' ') {
      switch (vpin->status()) {
        case 'P':
        case 'p':
          fprintf(fout, " PLACED");
          break;
        case 'F':
        case 'f':
          fprintf(fout, " FIXED");
          break;
        case 'C':
        case 'c':
          fprintf(fout, " COVER");
          break;
      }
      fprintf(fout, " %d %d", vpin->xLoc(), vpin->yLoc());
      if (vpin->orient() != -1) {
        fprintf(fout, " %s", vpin->orientStr());
      }
    }
  }

  // regularWiring
  if (net->numWires()) {
    for (i = 0; i < net->numWires(); i++) {
      wire = net->wire(i);
      for (j = 0; j < wire->numPaths(); j++) {
        p = wire->path(j);
        p->initTraverse();
        fprintf(fout, "\nNET %s %s", net->name(), wire->wireType());
        nline = 0;
        while ((path = p->next()) != DEFIPATH_DONE) {
          switch (path) {
            case DEFIPATH_LAYER:
              if (!netSeCmp) {
                fprintf(fout, " %s", p->getLayer());
              }
              layerName = p->getLayer();
              px = py = pz = -99;  // reset the 1 set of point to 0
              break;
            case DEFIPATH_MASK:
              fprintf(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
              fprintf(fout,
                      "MASK %d%d%d ",
                      p->getViaTopMask(),
                      p->getViaCutMask(),
                      p->getViaBottomMask());
              break;
            case DEFIPATH_VIA:
              if (!netSeCmp) {
                if (!ignoreVN) {
                  fprintf(fout, " %s", p->getVia());
                }
              } else {
                if (nline) {
                  if (!ignoreVN) {
                    fprintf(fout,
                            "\nNET %s %s ( %d %d ) %s",
                            net->name(),
                            wire->wireType(),
                            px,
                            py,
                            p->getVia());
                  } else {
                    fprintf(fout,
                            "\nNET %s %s ( %d %d )",
                            net->name(),
                            wire->wireType(),
                            px,
                            py);
                  }
                } else {
                  if (!ignoreVN) {
                    fprintf(fout, " ( %d %d ) %s", px, py, p->getVia());
                  } else {
                    fprintf(fout, " ( %d %d )", px, py);
                  }
                }
                px = py = pz = -99;  // reset the 1 set of point to 0
              }
              nline = 1;
              break;
            case DEFIPATH_RECT:
              p->getViaRect(&w, &x, &y, &z);
              fprintf(fout, "RECT ( %d %d %d %d ) ", w, x, y, z);
              break;
            case DEFIPATH_VIRTUALPOINT:
              p->getVirtualPoint(&x, &y);
              fprintf(fout, "VIRTUAL ( %d %d ) ", x, y);
              break;
            case DEFIPATH_VIAROTATION:
              fprintf(fout, "%d ", p->getViaRotation());
              nline = 1;
              break;
            case DEFIPATH_WIDTH:
              fprintf(fout, " %d ", p->getWidth());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              if (!netSeCmp) {
                if (!nline) {
                  fprintf(fout, " ( %d %d )", x, y);
                  nline = 1;
                } else {
                  fprintf(fout,
                          "\nNET %s %s %s ( %d %d )",
                          net->name(),
                          wire->wireType(),
                          layerName,
                          x,
                          y);
                }
              } else {
                if ((px == -99) && (py == -99)) {
                  px = x;
                  py = y;
                } else {
                  if (nline) {
                    fprintf(fout,
                            "\nNET %s %s %s",
                            net->name(),
                            wire->wireType(),
                            layerName);
                  }
                  if (px < x) {
                    fprintf(fout, " ( %d %d ) ( %d %d )", px, py, x, y);
                  } else if (px == x) {
                    if (py < y) {
                      fprintf(fout, " ( %d %d ) ( %d %d )", px, py, x, y);
                    } else {
                      fprintf(fout, " ( %d %d ) ( %d %d )", x, y, px, py);
                    }
                  } else {  // px > x
                    fprintf(fout, " ( %d %d ) ( %d %d )", x, y, px, py);
                  }
                  px = x;
                  py = y;
                  nline = 1;
                }
              }
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              if (!netSeCmp) {
                if (!nline) {
                  fprintf(fout, " ( %d %d %d )", x, y, z);
                } else {
                  fprintf(fout,
                          "\nNET %s %s %s ( %d %d %d )",
                          net->name(),
                          wire->wireType(),
                          layerName,
                          x,
                          y,
                          z);
                }
              } else {
                if ((px == -99) && (py == -99) && (pz == -99)) {
                  px = x;
                  py = y;
                  pz = z;
                } else {
                  if (nline) {
                    fprintf(fout,
                            "\nNET %s %s %s",
                            net->name(),
                            wire->wireType(),
                            layerName);
                  }
                  if (px < x) {
                    if (pz != -99) {
                      fprintf(fout,
                              " ( %d %d %d ) ( %d %d %d )",
                              px,
                              py,
                              pz,
                              x,
                              y,
                              z);
                    } else {
                      fprintf(fout, " ( %d %d ) ( %d %d %d )", px, py, x, y, z);
                    }
                  } else if (px == x) {
                    if (py < y) {
                      if (pz != -99) {
                        fprintf(fout,
                                " ( %d %d %d ) ( %d %d %d )",
                                px,
                                py,
                                pz,
                                x,
                                y,
                                z);
                      } else {
                        fprintf(
                            fout, " ( %d %d ) ( %d %d %d )", px, py, x, y, z);
                      }
                    } else {
                      if (pz != -99) {
                        fprintf(fout,
                                " ( %d %d %d ) ( %d %d %d )",
                                x,
                                y,
                                z,
                                px,
                                py,
                                pz);
                      } else {
                        fprintf(
                            fout, " ( %d %d %d ) ( %d %d )", x, y, z, px, py);
                      }
                    }
                  } else {  // px > x
                    if (pz != -99) {
                      fprintf(fout,
                              " ( %d %d %d ) ( %d %d %d )",
                              x,
                              y,
                              z,
                              px,
                              py,
                              pz);
                    } else {
                      fprintf(fout, " ( %d %d %d ) ( %d %d )", x, y, z, px, py);
                    }
                  }
                  px = x;
                  py = y;
                  pz = z;
                  nline = 1;
                }
              }
              break;
            case DEFIPATH_TAPER:
              fprintf(fout, " TAPER");
              break;
            case DEFIPATH_TAPERRULE:
              fprintf(fout, " TAPERRULE %s", p->getTaperRule());
              break;
          }
        }
      }
    }
  }

  // shieldnet
  if (net->numShieldNets()) {
    for (i = 0; i < net->numShieldNets(); i++) {
      fprintf(fout, "\nNET %s SHIELDNET %s ", net->name(), net->shieldNet(i));
    }
  }
  if (net->numNoShields()) {
    for (i = 0; i < net->numNoShields(); i++) {
      noShield = net->noShield(i);
      for (j = 0; j < noShield->numPaths(); j++) {
        p = noShield->path(j);
        p->initTraverse();
        fprintf(fout, "\nNET %s NOSHIELD", net->name());
        nline = 0;
        while ((path = (int) p->next()) != DEFIPATH_DONE) {
          switch (path) {
            case DEFIPATH_LAYER:
              fprintf(fout, " %s", p->getLayer());
              layerName = p->getLayer();
              break;
            case DEFIPATH_MASK:
              fprintf(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
              fprintf(fout,
                      "MASK %d%d%d ",
                      p->getViaTopMask(),
                      p->getViaCutMask(),
                      p->getViaBottomMask());
              break;
            case DEFIPATH_VIA:
              if (!ignoreVN) {
                fprintf(fout, " %s", p->getVia());
              }
              nline = 1;
              break;
            case DEFIPATH_VIAROTATION:
              fprintf(fout, " %d", p->getViaRotation());
              nline = 1;
              break;
            case DEFIPATH_WIDTH:
              fprintf(fout, " %d", p->getWidth());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              if (!nline) {
                fprintf(fout, " ( %d %d )", x, y);
                nline = 1;
              } else {
                fprintf(fout,
                        "\nNET %s %s ( %d %d )",
                        net->name(),
                        layerName,
                        x,
                        y);
                nline = 1;
              }
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              if (!nline) {
                fprintf(fout, " ( %d %d )", x, y);
                nline = 1;
              } else {
                fprintf(fout,
                        "\nNET %s %s ( %d %d )",
                        net->name(),
                        layerName,
                        x,
                        y);
                nline = 1;
              }
            case DEFIPATH_TAPER:
              fprintf(fout, " TAPER");
              break;
            case DEFIPATH_TAPERRULE:
              fprintf(fout, " TAPERRULE %s", p->getTaperRule());
              break;
          }
        }
      }
    }
  }

  if (net->hasSubnets()) {
    for (i = 0; i < net->numSubnets(); i++) {
      s = net->subnet(i);

      if (s->numConnections()) {
        for (j = 0; j < s->numConnections(); j++) {
          if (s->pinIsMustJoin(0)) {
            fprintf(fout, "\nNET MUSTJOIN");
          } else {
            fprintf(fout, "\nNET %s", s->name());
          }
          fprintf(fout, " ( %s %s )", s->instance(j), s->pin(j));
        }
      }

      for (j = 0; j < s->numWires(); j++) {
        wire = s->wire(j);
        if (s->numPaths()) {
          for (k = 0; k < wire->numPaths(); k++) {
            int elem;
            p = wire->path(k);
            p->initTraverse();
            fprintf(fout, "\nNET %s %s", s->name(), wire->wireType());
            nline = 0;
            elem = p->next();
            while (elem) {
              switch (elem) {
                case DEFIPATH_LAYER:
                  fprintf(fout, " LAYER %s", p->getLayer());
                  layerName = p->getLayer();
                  break;
                case DEFIPATH_MASK:
                  fprintf(fout, "MASK %d ", p->getMask());
                  break;
                case DEFIPATH_VIAMASK:
                  fprintf(fout,
                          "MASK %d%d%d ",
                          p->getViaTopMask(),
                          p->getViaCutMask(),
                          p->getViaBottomMask());
                  break;
                case DEFIPATH_VIA:
                  if (!ignoreVN) {
                    fprintf(fout, " VIA %s", p->getVia());
                  }
                  nline = 1;
                  break;
                case DEFIPATH_VIAROTATION:
                  fprintf(fout, " VIAROTATION %d", p->getViaRotation());
                  nline = 1;
                  break;
                case DEFIPATH_WIDTH:
                  fprintf(fout, " WIDTH %d", p->getWidth());
                  break;
                case DEFIPATH_POINT:
                  p->getPoint(&x, &y);
                  if (!nline) {
                    fprintf(fout, " POINT %d %d", x, y);
                    nline = 1;
                  } else {
                    fprintf(fout,
                            "\nNET %s %s %s POINT %d %d",
                            s->name(),
                            wire->wireType(),
                            layerName,
                            x,
                            y);
                    nline = 1;
                  }
                  break;
                  // case DEFIPATH_FLUSHPOINT:
                  // l = 0;
                  // p->getFlushPoint(i1, i2, ext);
                  // while (i1[l] && i2[l] && ext[l]) {
                  // fprintf(fout, "NET %s FLUSHPOINT %d %d %d\n",
                  // s->name(), i1[l], i2[l], ext[l]);
                  // l++;
                  //}
                  // break;
                case DEFIPATH_TAPERRULE:
                  fprintf(fout, " TAPERRULE %s", p->getTaperRule());
                  break;
                case DEFIPATH_SHAPE:
                  fprintf(fout, " SHAPE %s", p->getShape());
                  break;
                case DEFIPATH_STYLE:
                  fprintf(fout, " STYLE %d", p->getStyle());
                  break;
              }
              elem = p->next();
            }
          }
        }
      }
    }
  }

  /* Put the following all in one line */
  if (net->hasWeight() || net->hasCap() || net->hasSource() || net->hasPattern()
      || net->hasOriginal() || net->hasUse()) {
    fprintf(fout, "\nNET %s ", net->name());

    if (net->hasWeight()) {
      fprintf(fout, "WEIGHT %d ", net->weight());
    }
    if (net->hasCap()) {
      fprintf(fout, "ESTCAP %g ", checkDouble(net->cap()));
    }
    if (net->hasSource()) {
      fprintf(fout, "SOURCE %s ", net->source());
    }
    if (net->hasFixedbump()) {
      fprintf(fout, "FIXEDBUMP ");
    }
    if (net->hasFrequency()) {
      fprintf(fout, "FREQUENCY %g ", net->frequency());
    }
    if (net->hasPattern()) {
      fprintf(fout, "PATTERN %s ", net->pattern());
    }
    if (net->hasOriginal()) {
      fprintf(fout, "ORIGINAL %s ", net->original());
    }
    if (net->hasUse()) {
      fprintf(fout, "USE %s ", net->use());
    }
  }

  fprintf(fout, "\n");
  --numObjs;
  return 0;
}

// Special Net
int snetf(defrCallbackType_e c, const defiNet* net, defiUserData ud)
{
  // For net and special net.
  int i, j, x, y, z;
  char* layerName;
  double dist, left, right;
  defiPath* p;
  defiSubnet* s;
  int path;
  defiShield* shield;
  defiWire* wire;
  int nline = 0;
  const char* sNLayerName = "N/A";
  int numX, numY, stepX, stepY;

  checkType(c);
  if (ud != userData) {
    dataError();
  }
  if (c != defrSNetCbkType) {
    fprintf(fout, "BOGUS NET TYPE  ");
  }

  // compName & pinName
  if (net->numConnections() > 0) {
    for (i = 0; i < net->numConnections(); i++) {
      fprintf(fout,
              "SNET %s ( %s %s )\n",
              net->name(),
              net->instance(i),
              net->pin(i));
    }
  }

  if (net->numRectangles()) {  // 5.6

    for (i = 0; i < net->numRectangles(); i++) {
      fprintf(fout, "\nSNET %s ", net->name());
      if (strcmp(net->rectRouteStatus(i), "") != 0) {
        fprintf(fout, "%s ", net->rectRouteStatus(i));
        if (strcmp(net->rectRouteStatus(i), "SHIELD") == 0) {
          fprintf(fout, "%s ", net->rectRouteStatusShieldName(i));
        }
      }
      if (strcmp(net->rectShapeType(i), "") != 0) {
        fprintf(fout, "SHAPE %s ", net->rectShapeType(i));
      }

      if (net->rectMask(i)) {
        fprintf(fout,
                "MASK %d RECT %s %d %d %d %d",
                net->rectMask(i),
                net->rectName(i),
                net->xl(i),
                net->yl(i),
                net->xh(i),
                net->yh(i));
      } else {
        fprintf(fout,
                "RECT %s %d %d %d %d",
                net->name(),
                net->xl(i),
                net->yl(i),
                net->xh(i),
                net->yh(i));
      }
    }
  }

  if (net->numPolygons()) {
    for (i = 0; i < net->numPolygons(); i++) {
      fprintf(fout, "\nSNET %s ", net->name());
      if (curVer >= 5.8) {
        if (strcmp(net->polyRouteStatus(i), "") != 0) {
          fprintf(fout, "%s ", net->polyRouteStatus(i));
          if (strcmp(net->polyRouteStatus(i), "SHIELD") == 0) {
            fprintf(fout, "%s ", net->polyRouteStatusShieldName(i));
          }
        }
        if (strcmp(net->polyShapeType(i), "") != 0) {
          fprintf(fout, "SHAPE %s ", net->polyShapeType(i));
        }
      }
      if (net->polyMask(i)) {
        fprintf(
            fout, "MASK %d POLYGON %s ", net->polyMask(i), net->polygonName(i));
      } else {
        fprintf(fout, "POLYGON %s", net->polygonName(i));
      }

      defiPoints points = net->getPolygon(i);
      for (j = 0; j < points.numPoints; j++) {
        fprintf(fout, " %d %d", points.x[j], points.y[j]);
      }
    }
  }

  if (curVer >= 5.8 && net->numViaSpecs()) {
    for (i = 0; i < net->numViaSpecs(); i++) {
      fprintf(fout, "\nSNET %s ", net->name());
      if (strcmp(net->viaRouteStatus(i), "") != 0) {
        fprintf(fout, "%s ", net->viaRouteStatus(i));
        if (strcmp(net->viaRouteStatus(i), "SHIELD") == 0) {
          fprintf(fout, "%s ", net->viaRouteStatusShieldName(i));
        }
      }
      if (strcmp(net->viaShapeType(i), "") != 0) {
        fprintf(fout, "SHAPE %s ", net->viaShapeType(i));
      }
      if (net->topMaskNum(i) || net->cutMaskNum(i) || net->bottomMaskNum(i)) {
        fprintf(fout,
                "MASK %d%d%d VIA %s ",
                net->topMaskNum(i),
                net->cutMaskNum(i),
                net->bottomMaskNum(i),
                net->viaName(i));
      } else {
        fprintf(fout, "\n  VIA %s ", net->viaName(i));
      }
      fprintf(fout, " %s", net->viaOrientStr(i));

      defiPoints points = net->getViaPts(i);

      for (int jj = 0; jj < points.numPoints; jj++) {
        fprintf(fout, " %d %d", points.x[jj], points.y[jj]);
      }
      fprintf(fout, ";\n");
    }
  }

  // specialWiring
  if (net->numWires()) {
    for (i = 0; i < net->numWires(); i++) {
      wire = net->wire(i);
      for (j = 0; j < wire->numPaths(); j++) {
        p = wire->path(j);
        fprintf(fout, "\nSNET %s %s", net->name(), wire->wireType());
        nline = 0;
        p->initTraverse();
        while ((path = (int) p->next()) != DEFIPATH_DONE) {
          switch (path) {
            case DEFIPATH_LAYER:
              fprintf(fout, " %s", p->getLayer());
              sNLayerName = p->getLayer();
              break;
            case DEFIPATH_MASK:
              fprintf(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
              fprintf(fout,
                      "MASK %d%d%d ",
                      p->getViaTopMask(),
                      p->getViaCutMask(),
                      p->getViaBottomMask());
              break;
            case DEFIPATH_VIA:
              if (!ignoreVN) {
                fprintf(fout, " %s", p->getVia());
              }
              nline = 1;
              break;
            case DEFIPATH_VIAROTATION:
              fprintf(fout, " %d", p->getViaRotation());
              nline = 1;
              break;
            case DEFIPATH_VIADATA:
              p->getViaData(&numX, &numY, &stepX, &stepY);
              fprintf(
                  fout, " DO %d BY %d STEP %d %d", numX, numY, stepX, stepY);
              nline = 1;
              break;
            case DEFIPATH_WIDTH:
              fprintf(fout, " %d", p->getWidth());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              if (!nline) {
                fprintf(fout, " ( %d %d ) ", x, y);
                nline = 1;
              } else {
                fprintf(fout,
                        "\nSNET %s %s %s ( %d %d )",
                        net->name(),
                        wire->wireType(),
                        sNLayerName,
                        x,
                        y);
                nline = 1;
              }
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              fprintf(fout, "( %d %d %d ) ", x, y, z);
              nline = 1;
              break;
            case DEFIPATH_TAPER:
              fprintf(fout, " TAPER");
              break;
            case DEFIPATH_SHAPE:
              fprintf(fout, " + SHAPE %s", p->getShape());
              break;
            case DEFIPATH_STYLE:
              fprintf(fout, " + STYLE %d", p->getStyle());
              break;
          }
        }
      }
    }
  }

  if (net->hasSubnets()) {
    for (i = 0; i < net->numSubnets(); i++) {
      s = net->subnet(i);
      if (s->numConnections()) {
        if (s->pinIsMustJoin(0)) {
          fprintf(fout, "\nSNET %s MUSTJOIN", net->name());
        } else {
          fprintf(fout, "\nSNET %s", net->name());
        }
        for (j = 0; j < s->numConnections(); j++) {
          fprintf(fout, "( %s %s ) ", s->instance(j), s->pin(j));
        }
      }

      // regularWiring
      if (s->numWires()) {
        for (i = 0; i < s->numWires(); i++) {
          wire = s->wire(i);
          for (j = 0; j < wire->numPaths(); j++) {
            p = wire->path(j);
            p->initTraverse();
            fprintf(fout, "\nSNET %s %s", net->name(), wire->wireType());
            nline = 0;
            while ((path = (int) p->next()) != DEFIPATH_DONE) {
              switch (path) {
                case DEFIPATH_LAYER:
                  fprintf(fout, " %s", p->getLayer());
                  sNLayerName = p->getLayer();
                  break;
                case DEFIPATH_VIA:
                  if (!ignoreVN) {
                    fprintf(fout, " %s", p->getVia());
                  }
                  break;
                case DEFIPATH_MASK:
                  fprintf(fout, "MASK %d ", p->getMask());
                  break;
                case DEFIPATH_VIAMASK:
                  fprintf(fout,
                          "MASK %d%d%d ",
                          p->getViaTopMask(),
                          p->getViaCutMask(),
                          p->getViaBottomMask());
                  break;

                case DEFIPATH_VIAROTATION:
                  fprintf(fout, " %d", p->getViaRotation());
                  break;
                case DEFIPATH_WIDTH:
                  fprintf(fout, " %d", p->getWidth());
                  break;
                case DEFIPATH_POINT:
                  p->getPoint(&x, &y);
                  if (!nline) {
                    fprintf(fout, "( %d %d ) ", x, y);
                    nline = 1;
                  } else {
                    fprintf(fout,
                            "\nSNET %s %s %s ( %d %d ) ",
                            net->name(),
                            wire->wireType(),
                            sNLayerName,
                            x,
                            y);
                    nline = 1;
                  }
                  break;
                case DEFIPATH_TAPER:
                  fprintf(fout, " TAPER");
                  break;
              }
            }
          }
        }
      }
    }
  }

  if (net->numProps()) {
    for (i = 0; i < net->numProps(); i++) {
      fprintf(fout,
              "\nSNET %s PROP %s %s ",
              net->name(),
              net->propName(i),
              net->propValue(i));
      switch (net->propType(i)) {
        case 'R':
          fprintf(fout, "REAL ");
          break;
        case 'I':
          fprintf(fout, "INT ");
          break;
        case 'S':
          fprintf(fout, "STR ");
          break;
        case 'Q':
          fprintf(fout, "QSTR ");
          break;
        case 'N':
          fprintf(fout, "NUM ");
          break;
      }
    }
  }

  // SHIELD
  // testing the SHIELD for 5.3
  if (net->numShields()) {
    for (i = 0; i < net->numShields(); i++) {
      shield = net->shield(i);
      for (j = 0; j < shield->numPaths(); j++) {
        p = shield->path(j);
        fprintf(fout, "\nSNET %s SHIELD %s", net->name(), shield->shieldName());
        p->initTraverse();
        while ((path = (int) p->next()) != DEFIPATH_DONE) {
          switch (path) {
            case DEFIPATH_LAYER:
              fprintf(fout, " %s", p->getLayer());
              sNLayerName = p->getLayer();
              break;
            case DEFIPATH_VIA:
              if (!ignoreVN) {
                fprintf(fout, " %s", p->getVia());
              }
              break;
            case DEFIPATH_MASK:
              fprintf(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
              fprintf(fout,
                      "MASK %d%d%d ",
                      p->getViaTopMask(),
                      p->getViaCutMask(),
                      p->getViaBottomMask());
              break;
            case DEFIPATH_VIAROTATION:
              fprintf(fout, " %d", p->getViaRotation());
              break;
            case DEFIPATH_WIDTH:
              fprintf(fout, " %d", p->getWidth());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              if (!nline) {
                fprintf(fout, "( %d %d ) ", x, y);
                nline = 1;
              } else {
                fprintf(fout,
                        "\nSNET %s SHIELD %s %s ( %d %d )",
                        net->name(),
                        shield->shieldName(),
                        sNLayerName,
                        x,
                        y);
              }
              break;
            case DEFIPATH_TAPER:
              fprintf(fout, " TAPER");
              break;
          }
        }
      }
    }
  }

  // layerName width
  if (net->hasWidthRules()) {
    fprintf(fout, "\nSNET %s", net->name());
    for (i = 0; i < net->numWidthRules(); i++) {
      net->widthRule(i, &layerName, &dist);
      fprintf(fout, " WIDTH %s %g ", layerName, checkDouble(dist));
    }
  }

  // layerName spacing
  if (net->hasSpacingRules()) {
    fprintf(fout, "\nSNET %s", net->name());
    for (i = 0; i < net->numSpacingRules(); i++) {
      net->spacingRule(i, &layerName, &dist, &left, &right);
      if (left == right) {
        fprintf(fout, " SPACING %s %g ", layerName, checkDouble(dist));
      } else {
        fprintf(fout,
                " SPACING %s %g RANGE %g %g ",
                layerName,
                checkDouble(dist),
                checkDouble(left),
                checkDouble(right));
      }
    }
  }

  if (net->hasVoltage() || net->hasWeight() || net->hasCap() || net->hasSource()
      || net->hasPattern() || net->hasOriginal() || net->hasUse()) {
    fprintf(fout, "\nSNET %s", net->name());
    if (net->hasVoltage()) {
      fprintf(fout, " VOLTAGE %g", checkDouble(net->voltage()));
    }
    if (net->hasWeight()) {
      fprintf(fout, " WEIGHT %d", net->weight());
    }
    if (net->hasCap()) {
      fprintf(fout, " ESTCAP %g", checkDouble(net->cap()));
    }
    if (net->hasSource()) {
      fprintf(fout, " SOURCE %s", net->source());
    }
    if (net->hasPattern()) {
      fprintf(fout, " PATTERN %s", net->pattern());
    }
    if (net->hasOriginal()) {
      fprintf(fout, " ORIGINAL %s", net->original());
    }
    if (net->hasUse()) {
      fprintf(fout, " USE %s", net->use());
    }
  }

  fprintf(fout, "\n");
  --numObjs;
  return 0;
}

int ndr(defrCallbackType_e c, const defiNonDefault* nd, defiUserData ud)
{
  // For nondefaultrule
  int i;

  checkType(c);
  if (ud != userData) {
    dataError();
  }
  if (c != defrNonDefaultCbkType) {
    fprintf(fout, "BOGUS NONDEFAULTRULE TYPE  ");
  }
  fprintf(fout, "NDR %s", nd->name());
  if (nd->hasHardspacing()) {
    fprintf(fout, " HARDSPACING\n");
  }
  fprintf(fout, "\n");
  for (i = 0; i < nd->numLayers(); i++) {
    fprintf(fout, "NDR %s LAYER %s", nd->name(), nd->layerName(i));
    fprintf(fout, " WIDTH %d", nd->layerWidthVal(i));
    if (nd->hasLayerDiagWidth(i)) {
      fprintf(fout, " DIAGWIDTH %d", nd->layerDiagWidthVal(i));
    }
    if (nd->hasLayerSpacing(i)) {
      fprintf(fout, " SPACING %d", nd->layerSpacingVal(i));
    }
    if (nd->hasLayerWireExt(i)) {
      fprintf(fout, " WIREEXT %d", nd->layerWireExtVal(i));
    }
    fprintf(fout, "\n");
  }
  for (i = 0; i < nd->numVias(); i++) {
    fprintf(fout, "NDR %s VIA %s\n", nd->name(), nd->viaName(i));
  }
  for (i = 0; i < nd->numViaRules(); i++) {
    fprintf(fout, "NDR %s VIARULE %s\n", nd->name(), nd->viaRuleName(i));
  }
  for (i = 0; i < nd->numMinCuts(); i++) {
    fprintf(fout,
            "NDR %s MINCUTS %s %d\n",
            nd->name(),
            nd->cutLayerName(i),
            nd->numCuts(i));
  }
  for (i = 0; i < nd->numProps(); i++) {
    fprintf(fout,
            "NDR %s PROPERTY %s %s\n",
            nd->name(),
            nd->propName(i),
            nd->propValue(i));
  }
  --numObjs;
  return 0;
}

// Technology
int tname(defrCallbackType_e c, const char* string, defiUserData ud)
{
  checkType(c);
  if (ud != userData) {
    dataError();
  }
  fprintf(fout, "TECHNOLOGY %s\n", string);
  return 0;
}

// Design
int dname(defrCallbackType_e c, const char* string, defiUserData ud)
{
  checkType(c);
  if (ud != userData) {
    dataError();
  }
  fprintf(fout, "DESIGN %s\n", string);

  return 0;
}

char* address(const char* in)
{
  return ((char*) in);
}

// Assertion or Constraints
void operand(defrCallbackType_e c, const defiAssertion* a, int ind)
{
  int i, first = 1;
  char* netName;
  char *fromInst, *fromPin, *toInst, *toPin;

  if (a->isSum()) {
    // Sum in operand, recursively call operand
    fprintf(fout, "ASSERTIONS/CONSTRAINTS SUM ( ");
    a->unsetSum();
    isSumSet = 1;
    begOperand = 0;
    operand(c, a, ind);
    fprintf(fout, ") ");
  } else {
    // operand
    if (ind >= a->numItems()) {
      fprintf(fout, "ERROR: when writing out SUM in Constraints.\n");
      return;
    }
    if (begOperand) {
      fprintf(fout, "ASSRT/CONSTR ");
      begOperand = 0;
    }
    for (i = ind; i < a->numItems(); i++) {
      if (a->isNet(i)) {
        a->net(i, &netName);
        if (!first) {
          fprintf(fout, ", ");  // print , as separator
        }
        fprintf(fout, "NET %s ", netName);
      } else if (a->isPath(i)) {
        a->path(i, &fromInst, &fromPin, &toInst, &toPin);
        if (!first) {
          fprintf(fout, ", ");
        }
        fprintf(fout, "PATH %s %s %s %s ", fromInst, fromPin, toInst, toPin);
      } else if (isSumSet) {
        // SUM within SUM, reset the flag
        a->setSum();
        operand(c, a, i);
      }
      first = 0;
    }
  }
}

// Assertion or Constraints
int constraint(defrCallbackType_e c, const defiAssertion* a, defiUserData ud)
{
  // Handles both constraints and assertions

  checkType(c);
  if (ud != userData) {
    dataError();
  }
  if (a->isWiredlogic()) {
    // Wirelogic
    fprintf(fout,
            "ASSRT/CONSTR WIREDLOGIC %s + MAXDIST %g\n",
            a->netName(),
            checkDouble(a->fallMax()));
  } else {
    // Call the operand function
    isSumSet = 0;  // reset the global variable
    begOperand = 1;
    operand(c, a, 0);
    // Get the Rise and Fall
    if (a->hasRiseMax()) {
      fprintf(fout, " RISEMAX %g ", checkDouble(a->riseMax()));
    }
    if (a->hasFallMax()) {
      fprintf(fout, " FALLMAX %g ", checkDouble(a->fallMax()));
    }
    if (a->hasRiseMin()) {
      fprintf(fout, " RISEMIN %g ", checkDouble(a->riseMin()));
    }
    if (a->hasFallMin()) {
      fprintf(fout, " FALLMIN %g ", checkDouble(a->fallMin()));
    }
    fprintf(fout, "\n");
  }
  --numObjs;
  return 0;
}

// Property definitions
int prop(defrCallbackType_e c, const defiProp* p, defiUserData ud)
{
  checkType(c);
  if (ud != userData) {
    dataError();
  }
  if (strcmp(p->propType(), "design") == 0) {
    fprintf(fout, "PROPDEF DESIGN %s ", p->propName());
  } else if (strcmp(p->propType(), "net") == 0) {
    fprintf(fout, "PROPDEF NET %s ", p->propName());
  } else if (strcmp(p->propType(), "component") == 0) {
    fprintf(fout, "PROPDEF COMP %s ", p->propName());
  } else if (strcmp(p->propType(), "specialnet") == 0) {
    fprintf(fout, "PROPDEF SNET %s ", p->propName());
  } else if (strcmp(p->propType(), "group") == 0) {
    fprintf(fout, "PROPDEF GROUP %s ", p->propName());
  } else if (strcmp(p->propType(), "row") == 0) {
    fprintf(fout, "PROPDEF ROW %s ", p->propName());
  } else if (strcmp(p->propType(), "componentpin") == 0) {
    fprintf(fout, "PROPDEF COMPPIN %s ", p->propName());
  } else if (strcmp(p->propType(), "region") == 0) {
    fprintf(fout, "PROPDEF REGION %s ", p->propName());
  } else if (strcmp(p->propType(), "nondefaultrule") == 0) {
    fprintf(fout, "PROPDEF NONDEFAULTRULE %s ", p->propName());
  }
  if (p->dataType() == 'I') {
    fprintf(fout, "INT ");
  }
  if (p->dataType() == 'R') {
    fprintf(fout, "REAL ");
  }
  if (p->dataType() == 'S') {
    fprintf(fout, "STR ");
  }
  if (p->dataType() == 'Q') {
    fprintf(fout, "STR ");
  }
  if (p->hasRange()) {
    fprintf(
        fout, "RANGE %g %g ", checkDouble(p->left()), checkDouble(p->right()));
  }
  if (p->hasNumber()) {
    fprintf(fout, "%g ", checkDouble(p->number()));
  }
  if (p->hasString()) {
    fprintf(fout, "\"%s\" ", p->string());
  }
  fprintf(fout, "\n");

  return 0;
}

// History
int hist(defrCallbackType_e c, const char* h, defiUserData ud)
{
  checkType(c);
  if (ud != userData) {
    dataError();
  }
  fprintf(fout, "HIST %s\n", h);
  return 0;
}

// Busbitchars
int bbn(defrCallbackType_e c, const char* h, defiUserData ud)
{
  checkType(c);
  if (ud != userData) {
    dataError();
  }
  fprintf(fout, "BUSBITCHARS \"%s\" \n", h);
  return 0;
}

// Version
int vers(defrCallbackType_e c, double d, defiUserData ud)
{
  checkType(c);
  if (ud != userData) {
    dataError();
  }
  fprintf(fout, "VERSION %g\n", d);

  curVer = d;
  return 0;
}

// Units
int units(defrCallbackType_e c, double d, defiUserData ud)
{
  checkType(c);
  if (ud != userData) {
    dataError();
  }
  fprintf(fout, "UNITS DISTANCE MICRONS %g\n", checkDouble(d));
  return 0;
}

// Casesensitive
int casesens(defrCallbackType_e c, int d, defiUserData ud)
{
  checkType(c);
  if (ud != userData) {
    dataError();
  }
  if (d == 1) {
    fprintf(fout, "NAMESCASESENSITIVE OFF\n");
  } else {
    fprintf(fout, "NAMESCASESENSITIVE ON\n");
  }
  return 0;
}

// Site, Canplace, Cannotoccupy, Diearea, Pin, Pincap, DefaultCap,
// Row, Gcellgrid, Track, Via, Scanchain, IOtiming, Flooplan,
// Region, Group, TiminDisable, Pin property
int cls(defrCallbackType_e c, void* cl, defiUserData ud)
{
  defiSite* site;  // Site and Canplace and CannotOccupy
  defiBox* box;    // DieArea and
  defiPinCap* pc;
  defiPin* pin;
  int i, j, k;
  defiRow* row;
  defiTrack* track;
  defiGcellGrid* gcg;
  defiVia* via;
  defiRegion* re;
  defiGroup* group;
  defiComponentMaskShiftLayer* maskShiftLayer = nullptr;
  defiScanchain* sc;
  defiIOTiming* iot;
  defiFPC* fpc;
  defiTimingDisable* td;
  defiPartition* part;
  defiPinProp* pprop;
  defiBlockage* block;
  defiSlot* slot;
  defiFill* fill;
  defiStyles* styles;
  int xl, yl, xh, yh;
  char* name;
  char **inst, **inPin, **outPin;
  int* bits;
  int size;
  int corner, typ;
  const char* itemT;
  char dir;
  defiPinAntennaModel* aModel;
  char* tmpPinName = nullptr;
  char* extraPinName = nullptr;
  char* pName = nullptr;
  char* tmpName = nullptr;

  checkType(c);
  if (ud != userData) {
    dataError();
  }
  switch (c) {
    case defrSiteCbkType:
      site = (defiSite*) cl;
      fprintf(fout,
              "SITE %s %g %g %s ",
              site->name(),
              checkDouble(site->x_orig()),
              checkDouble(site->y_orig()),
              site->orientStr());
      fprintf(fout,
              "DO %g BY %g STEP %g %g\n",
              checkDouble(site->x_num()),
              checkDouble(site->y_num()),
              checkDouble(site->x_step()),
              checkDouble(site->y_step()));
      break;
    case defrCanplaceCbkType:
      site = (defiSite*) cl;
      fprintf(fout,
              "CANPLACE %s %g %g %s ",
              site->name(),
              checkDouble(site->x_orig()),
              checkDouble(site->y_orig()),
              site->orientStr());
      fprintf(fout,
              "DO %g BY %g STEP %g %g\n",
              checkDouble(site->x_num()),
              checkDouble(site->y_num()),
              checkDouble(site->x_step()),
              checkDouble(site->y_step()));
      break;
    case defrCannotOccupyCbkType:
      site = (defiSite*) cl;
      fprintf(fout,
              "CANNOTOCCUPY %s %g %g %s ",
              site->name(),
              checkDouble(site->x_orig()),
              checkDouble(site->y_orig()),
              site->orientStr());
      fprintf(fout,
              "DO %g BY %g STEP %g %g\n",
              checkDouble(site->x_num()),
              checkDouble(site->y_num()),
              checkDouble(site->x_step()),
              checkDouble(site->y_step()));
      break;
    case defrDieAreaCbkType:
      box = (defiBox*) cl;
      fprintf(fout, "DIEAREA");
      {
        defiPoints points = box->getPoint();
        for (i = 0; i < points.numPoints; i++) {
          fprintf(fout, " %d %d", points.x[i], points.y[i]);
        }
        fprintf(fout, "\n");
      }
      break;
    case defrPinCapCbkType:
      pc = (defiPinCap*) cl;
      fprintf(fout,
              "DEFCAP MINPINS %d WIRECAP %g\n",
              pc->pin(),
              checkDouble(pc->cap()));
      --numObjs;
      break;
    case defrPinCbkType:
      pin = (defiPin*) cl;
      pName = strdup((char*) pin->pinName());  // get the pinName
      // check if there has .extra<n> in the pName and ignorePE
      // is set to 1
      if (ignorePE) {
        // check if .extra is in the name, if it is, ignore it
        extraPinName = strstr(pName, ".extra");
        if (extraPinName == nullptr) {
          tmpPinName = pName;
        } else {
          // make sure name ends with .extraNNN
          tmpName = extraPinName;
          extraPinName = extraPinName + 6;
          *tmpName = '\0';
          tmpPinName = pName;
          if (extraPinName != nullptr) {
            while (*extraPinName != '\0' && *extraPinName != '\n') {
              if (isdigit(*extraPinName++)) {
                continue;
              }
              // Name does not end only .extraNNN
              tmpPinName = strdup(pin->pinName());
              break;
            }
          }
        }
      } else {
        tmpPinName = pName;
      }
      fprintf(fout, "PIN %s + NET %s ", tmpPinName, pin->netName());
      if (pin->hasDirection()) {
        fprintf(fout, "+ DIRECTION %s ", pin->direction());
      }
      if (pin->hasUse()) {
        fprintf(fout, "+ USE %s ", pin->use());
      }
      if (pin->hasNetExpr()) {
        fprintf(fout, "+ NETEXPR %s", pin->netExpr());
      }
      if (pin->hasSupplySensitivity()) {
        fprintf(fout, "+ SUPPLYSENSITIVITY %s ", pin->supplySensitivity());
      }
      if (pin->hasGroundSensitivity()) {
        fprintf(fout, "+ GROUNDSENSITIVITY %s ", pin->groundSensitivity());
      }
      if (pin->hasLayer()) {
        for (i = 0; i < pin->numLayer(); i++) {
          fprintf(fout, "+ LAYER %s ", pin->layer(i));
          if (pin->layerMask(i)) {
            fprintf(fout, "MASK %d ", pin->layerMask(i));
          }
          if (pin->hasLayerSpacing(i)) {
            fprintf(fout, "SPACING %d ", pin->layerSpacing(i));
          }
          if (pin->hasLayerDesignRuleWidth(i)) {
            fprintf(fout, "DESIGNRULEWIDTH %d ", pin->layerDesignRuleWidth(i));
          }
          pin->bounds(i, &xl, &yl, &xh, &yh);
          fprintf(fout, "( %d %d ) ( %d %d ) ", xl, yl, xh, yh);
        }
        for (i = 0; i < pin->numPolygons(); i++) {
          fprintf(fout, "+ POLYGON %s", pin->polygonName(i));
          if (pin->polygonMask(i)) {
            fprintf(fout, "MASK %d ", pin->polygonMask(i));
          }
          if (pin->hasPolygonSpacing(i)) {
            fprintf(fout, "SPACING %d ", pin->polygonSpacing(i));
          }
          if (pin->hasPolygonDesignRuleWidth(i)) {
            fprintf(
                fout, "DESIGNRULEWIDTH %d ", pin->polygonDesignRuleWidth(i));
          }
          defiPoints points = pin->getPolygon(i);
          for (k = 0; k < points.numPoints; k++) {
            fprintf(fout, " %d %d", points.x[k], points.y[k]);
          }
        }
        for (i = 0; i < pin->numVias(); i++) {
          if (pin->viaTopMask(i) || pin->viaCutMask(i)
              || pin->viaBottomMask(i)) {
            fprintf(fout,
                    "\nVIA %s MASK %d%d%d %d %d ",
                    pin->viaName(i),
                    pin->viaTopMask(i),
                    pin->viaCutMask(i),
                    pin->viaBottomMask(i),
                    pin->viaPtX(i),
                    pin->viaPtY(i));
          } else {
            fprintf(fout,
                    "\nVIA %s %d %d ",
                    pin->viaName(i),
                    pin->viaPtX(i),
                    pin->viaPtY(i));
          }
        }
      }
      if (pin->hasPlacement()) {
        if (pin->isPlaced()) {
          fprintf(fout, " PLACED ");
        }
        if (pin->isCover()) {
          fprintf(fout, " COVER ");
        }
        if (pin->isFixed()) {
          fprintf(fout, " FIXED ");
        }
        fprintf(fout,
                "( %d %d ) %s ",
                pin->placementX(),
                pin->placementY(),
                pin->orientStr());
      }
      if (pin->hasSpecial()) {
        fprintf(fout, " SPECIAL ");
      }
      fprintf(fout, "\n");

      if (pin->hasPort()) {
        defiPinPort* port;
        for (j = 0; j < pin->numPorts(); j++) {
          fprintf(fout, "PIN %s", tmpPinName);
          port = pin->pinPort(j);
          fprintf(fout, " + PORT");
          for (i = 0; i < port->numLayer(); i++) {
            fprintf(fout, "+ LAYER %s", port->layer(i));
            if (port->layerMask(i)) {
              fprintf(fout, "MASK %d ", port->layerMask(i));
            }
            if (port->hasLayerSpacing(i)) {
              fprintf(fout, " SPACING %d", port->layerSpacing(i));
            }
            if (port->hasLayerDesignRuleWidth(i)) {
              fprintf(
                  fout, " DESIGNRULEWIDTH %d", port->layerDesignRuleWidth(i));
            }
            port->bounds(i, &xl, &yl, &xh, &yh);
            fprintf(fout, " %d %d %d %d", xl, yl, xh, yh);
          }
          for (i = 0; i < port->numPolygons(); i++) {
            fprintf(fout, " + POLYGON %s", port->polygonName(i));
            if (port->polygonMask(i)) {
              fprintf(fout, "MASK %d ", port->polygonMask(i));
            }
            if (port->hasPolygonSpacing(i)) {
              fprintf(fout, " SPACING %d", port->polygonSpacing(i));
            }
            if (port->hasPolygonDesignRuleWidth(i)) {
              fprintf(
                  fout, " DESIGNRULEWIDTH %d", port->polygonDesignRuleWidth(i));
            }

            defiPoints pts = port->getPolygon(i);
            for (k = 0; k < pts.numPoints; k++) {
              fprintf(fout, " %d %d", pts.x[k], pts.y[k]);
            }
          }
          for (i = 0; i < port->numVias(); i++) {
            if (port->viaTopMask(i) || port->viaCutMask(i)
                || port->viaBottomMask(i)) {
              fprintf(fout,
                      "\n    VIA %s MASK %d%d%d ( %d %d ) ",
                      port->viaName(i),
                      port->viaTopMask(i),
                      port->viaCutMask(i),
                      port->viaBottomMask(i),
                      port->viaPtX(i),
                      port->viaPtY(i));
            } else {
              fprintf(fout,
                      " VIA %s ( %d %d ) ",
                      port->viaName(i),
                      port->viaPtX(i),
                      port->viaPtY(i));
            }
          }
          if (port->hasPlacement()) {
            if (port->isPlaced()) {
              fprintf(fout, " + PLACED");
              fprintf(fout,
                      " %d %d %d ",
                      port->placementX(),
                      port->placementY(),
                      port->orient());
            }
            if (port->isCover()) {
              fprintf(fout, " + COVER");
              fprintf(fout,
                      " %d %d %d ",
                      port->placementX(),
                      port->placementY(),
                      port->orient());
            }
            if (port->isFixed()) {
              fprintf(fout, " + FIXED");
              fprintf(fout,
                      " %d %d %d",
                      port->placementX(),
                      port->placementY(),
                      port->orient());
            }
          }
          fprintf(fout, "\n");
        }
      }
      if (pin->hasAPinPartialMetalArea()) {
        fprintf(fout, "PIN %s + NET %s ", tmpPinName, pin->netName());
        for (i = 0; i < pin->numAPinPartialMetalArea(); i++) {
          fprintf(fout,
                  " ANTPINPARTIALMETALAREA %d ",
                  pin->APinPartialMetalArea(i));
          if (*(pin->APinPartialMetalAreaLayer(i))) {
            fprintf(fout, " %s ", pin->APinPartialMetalAreaLayer(i));
          }
        }
        fprintf(fout, "\n");
      }
      if (pin->hasAPinPartialMetalSideArea()) {
        fprintf(fout, "PIN %s + NET %s ", tmpPinName, pin->netName());
        for (i = 0; i < pin->numAPinPartialMetalSideArea(); i++) {
          fprintf(fout,
                  "ANTPINPARTIALMETALSIDEAREA %d",
                  pin->APinPartialMetalSideArea(i));
          if (*(pin->APinPartialMetalSideAreaLayer(i))) {
            fprintf(fout, " %s", pin->APinPartialMetalSideAreaLayer(i));
          }
        }
        fprintf(fout, "\n");
      }
      if (pin->hasAPinPartialCutArea()) {
        fprintf(fout, "PIN %s + NET %s ", tmpPinName, pin->netName());
        for (i = 0; i < pin->numAPinPartialCutArea(); i++) {
          fprintf(fout, "ANTPINPARTIALCUTAREA %d", pin->APinPartialCutArea(i));
          if (*(pin->APinPartialCutAreaLayer(i))) {
            fprintf(fout, " %s", pin->APinPartialCutAreaLayer(i));
          }
        }
        fprintf(fout, "\n");
      }
      if (pin->hasAPinDiffArea()) {
        fprintf(fout, "PIN %s + NET %s ", tmpPinName, pin->netName());
        for (i = 0; i < pin->numAPinDiffArea(); i++) {
          fprintf(fout, "ANTPINDIFFAREA %d", pin->APinDiffArea(i));
          if (*(pin->APinDiffAreaLayer(i))) {
            fprintf(fout, " %s", pin->APinDiffAreaLayer(i));
          }
        }
        fprintf(fout, "\n");
      }

      for (j = 0; j < pin->numAntennaModel(); j++) {
        aModel = pin->antennaModel(j);

        if (aModel->hasAPinGateArea()) {
          fprintf(fout,
                  "PIN %s + NET %s %s ",
                  tmpPinName,
                  pin->netName(),
                  aModel->antennaOxide());
          for (i = 0; i < aModel->numAPinGateArea(); i++) {
            fprintf(fout, "ANTPINGATEAREA %d", aModel->APinGateArea(i));
            if (*(aModel->APinGateAreaLayer(i))) {
              fprintf(fout, " %s", aModel->APinGateAreaLayer(i));
            }
          }
          fprintf(fout, "\n");
        }
        if (aModel->hasAPinMaxAreaCar()) {
          fprintf(fout,
                  "PIN %s + NET %s %s ",
                  tmpPinName,
                  pin->netName(),
                  aModel->antennaOxide());
          for (i = 0; i < aModel->numAPinMaxAreaCar(); i++) {
            fprintf(fout, "ANTPINMAXAREACAR %d", aModel->APinMaxAreaCar(i));
            if (*(aModel->APinMaxAreaCarLayer(i))) {
              fprintf(fout, " %s", aModel->APinMaxAreaCarLayer(i));
            }
          }
          fprintf(fout, "\n");
        }
        if (aModel->hasAPinMaxSideAreaCar()) {
          fprintf(fout,
                  "PIN %s + NET %s %s ",
                  tmpPinName,
                  pin->netName(),
                  aModel->antennaOxide());
          for (i = 0; i < aModel->numAPinMaxSideAreaCar(); i++) {
            fprintf(
                fout, "ANTPINMAXSIDEAREACAR %d", aModel->APinMaxSideAreaCar(i));
            if (*(aModel->APinMaxSideAreaCarLayer(i))) {
              fprintf(fout, " %s", aModel->APinMaxSideAreaCarLayer(i));
            }
          }
          fprintf(fout, "\n");
        }
        if (aModel->hasAPinMaxCutCar()) {
          fprintf(fout,
                  "PIN %s + NET %s %s ",
                  tmpPinName,
                  pin->netName(),
                  aModel->antennaOxide());
          for (i = 0; i < aModel->numAPinMaxCutCar(); i++) {
            fprintf(fout, "ANTPINMAXCUTCAR %d", aModel->APinMaxCutCar(i));
            if (*(aModel->APinMaxCutCarLayer(i))) {
              fprintf(fout, " %s", aModel->APinMaxCutCarLayer(i));
            }
          }
          fprintf(fout, "\n");
        }
      }
      if (tmpPinName) {
        free(tmpPinName);
      }
      --numObjs;
      break;
    case defrDefaultCapCbkType:
      i = (uint64_t) cl;
      fprintf(fout, "DEFAULTCAP %d\n", i);
      numObjs = i;
      break;
    case defrRowCbkType:
      row = (defiRow*) cl;
      if (ignoreRN) {  // PCR 716759, if flag is set don't bother with name
        fprintf(fout,
                "ROW %s %g %g %d",
                row->macro(),
                checkDouble(row->x()),
                checkDouble(row->y()),
                row->orient());
      } else {
        fprintf(fout,
                "ROW %s %s %g %g %d",
                row->name(),
                row->macro(),
                checkDouble(row->x()),
                checkDouble(row->y()),
                row->orient());
      }
      if (row->hasDo()) {
        fprintf(fout,
                " DO %g BY %g",
                checkDouble(row->xNum()),
                checkDouble(row->yNum()));
        if (row->hasDoStep()) {
          fprintf(fout,
                  " STEP %g %g\n",
                  checkDouble(row->xStep()),
                  checkDouble(row->yStep()));
        }
      }
      fprintf(fout, "\n");
      if (row->numProps() > 0) {
        if (ignoreRN) {
          for (i = 0; i < row->numProps(); i++) {
            fprintf(
                fout, "ROW PROP %s %s\n", row->propName(i), row->propValue(i));
          }
        } else {
          for (i = 0; i < row->numProps(); i++) {
            fprintf(fout,
                    "ROW %s PROP %s %s\n",
                    row->name(),
                    row->propName(i),
                    row->propValue(i));
          }
        }
      }
      break;
    case defrTrackCbkType:
      track = (defiTrack*) cl;
      /*if (track->firstTrackMask()) {
         if (track->sameMask()) {
         fprintf(fout, "TRACKS %s %g DO %g STEP %g MASK %d SAMEMASK LAYER ",
                  track->macro(), track->x(),
                  track->xNum(), track->xStep(),
              track->firstTrackMask());
         } else {
         fprintf(fout, "TRACKS %s %g DO %g STEP %g MASK %d LAYER ",
                  track->macro(), track->x(),
                  track->xNum(), track->xStep(),
              track->firstTrackMask());
         }
      } else {
         fprintf(fout, "TRACKS %s %g DO %g STEP %g LAYER ",
                  track->macro(), track->x(),
                  track->xNum(), track->xStep());
      } */
      for (i = 0; i < track->numLayers(); i++) {
        if (track->firstTrackMask()) {
          if (track->sameMask()) {
            fprintf(fout,
                    "TRACKS %s %g DO %g STEP %g MASK %d SAMEMASK LAYER %s\n",
                    track->macro(),
                    track->x(),
                    track->xNum(),
                    track->xStep(),
                    track->firstTrackMask(),
                    track->layer(i));
          } else {
            fprintf(fout,
                    "TRACKS %s %g DO %g STEP %g MASK %d LAYER %s\n",
                    track->macro(),
                    track->x(),
                    track->xNum(),
                    track->xStep(),
                    track->firstTrackMask(),
                    track->layer(i));
          }
        } else {
          fprintf(fout,
                  "TRACKS %s %g DO %g STEP %g LAYER %s\n",
                  track->macro(),
                  track->x(),
                  track->xNum(),
                  track->xStep(),
                  track->layer(i));
        }
      }
      break;
    case defrGcellGridCbkType:
      gcg = (defiGcellGrid*) cl;
      fprintf(fout,
              "GCELLGRID %s %d DO %d STEP %g\n",
              gcg->macro(),
              gcg->x(),
              gcg->xNum(),
              checkDouble(gcg->xStep()));
      break;
    case defrViaCbkType:
      via = (defiVia*) cl;
      fprintf(fout, "VIA %s ", via->name());
      if (via->hasPattern()) {
        fprintf(fout, " PATTERNNAME %s\n", via->pattern());
      } else {
        fprintf(fout, "\n");
      }
      for (i = 0; i < via->numLayers(); i++) {
        via->layer(i, &name, &xl, &yl, &xh, &yh);
        int rectMask = via->rectMask(i);

        if (rectMask) {
          fprintf(fout,
                  "VIA %s RECT %s  MASK %d ( %d %d ) ( %d %d ) \n",
                  via->name(),
                  name,
                  rectMask,
                  xl,
                  yl,
                  xh,
                  yh);
        } else {
          fprintf(fout,
                  "VIA %s RECT %s ( %d %d ) ( %d %d ) \n",
                  via->name(),
                  name,
                  xl,
                  yl,
                  xh,
                  yh);
        }
      }
      // POLYGON
      if (via->numPolygons()) {
        for (i = 0; i < via->numPolygons(); i++) {
          int polyMask = via->polyMask(i);

          if (polyMask) {
            fprintf(
                fout, "\n  POLYGON %s MASK %d ", via->polygonName(i), polyMask);
          } else {
            fprintf(fout, "\n  POLYGON %s ", via->polygonName(i));
          }
          defiPoints pts = via->getPolygon(i);
          for (j = 0; j < pts.numPoints; j++) {
            fprintf(fout, "%d %d ", pts.x[j], pts.y[j]);
          }
        }
        fprintf(fout, " \n");
      }

      if (via->hasViaRule()) {
        char *vrn, *bl, *cll, *tl;
        int xs, ys, xcs, ycs, xbe, ybe, xte, yte;
        int cr, cc, xo, yo, xbo, ybo, xto, yto;
        (void) via->viaRule(
            &vrn, &xs, &ys, &bl, &cll, &tl, &xcs, &ycs, &xbe, &ybe, &xte, &yte);
        fprintf(fout,
                "VIA %s VIARULE %s CUTSIZE %d %d LAYERS %s %s %s",
                via->name(),
                vrn,
                xs,
                ys,
                bl,
                cll,
                tl);
        fprintf(fout,
                " CUTSPACING %d %d ENCLOSURE %d %d %d %d",
                xcs,
                ycs,
                xbe,
                ybe,
                xte,
                yte);
        if (via->hasRowCol()) {
          (void) via->rowCol(&cr, &cc);
          fprintf(fout, " ROWCOL %d %d", cr, cc);
        }
        if (via->hasOrigin()) {
          (void) via->origin(&xo, &yo);
          fprintf(fout, " ORIGIN %d %d", xo, yo);
        }
        if (via->hasOffset()) {
          (void) via->offset(&xbo, &ybo, &xto, &yto);
          fprintf(fout, " OFFSET %d %d %d %d", xbo, ybo, xto, yto);
        }
        if (via->hasCutPattern()) {
          fprintf(fout, " PATTERN %s", via->cutPattern());
        }
        fprintf(fout, "\n");
      }
      --numObjs;
      break;
    case defrRegionCbkType:
      re = (defiRegion*) cl;
      for (i = 0; i < re->numRectangles(); i++) {
        fprintf(fout,
                "REGION %s ( %d %d ) ( %d %d )\n",
                re->name(),
                re->xl(i),
                re->yl(i),
                re->xh(i),
                re->yh(i));
      }
      if (re->hasType()) {
        fprintf(fout, "REGION %s TYPE %s\n", re->name(), re->type());
      }
      --numObjs;
      break;
    case defrGroupCbkType:
      group = (defiGroup*) cl;
      fprintf(fout, "GROUP %s ", group->name());
      if (group->hasMaxX() | group->hasMaxY() | group->hasPerim()) {
        fprintf(fout, "SOFT ");
        if (group->hasPerim()) {
          fprintf(fout, "MAXHALFPERIMETER %d ", group->perim());
        }
        if (group->hasMaxX()) {
          fprintf(fout, "MAXX %d ", group->maxX());
        }
        if (group->hasMaxY()) {
          fprintf(fout, "MAXY %d ", group->maxY());
        }
      }
      if (group->hasRegionName()) {
        fprintf(fout, "REGION %s ", group->regionName());
      }
      if (group->hasRegionBox()) {
        int *gxl, *gyl, *gxh, *gyh;
        int sz;
        group->regionRects(&sz, &gxl, &gyl, &gxh, &gyh);
        for (i = 0; i < sz; i++) {
          fprintf(
              fout, "REGION (%d %d) (%d %d) ", gxl[i], gyl[i], gxh[i], gyh[i]);
        }
      }
      fprintf(fout, "\n");
      --numObjs;
      break;
    case defrComponentMaskShiftLayerCbkType:
      fprintf(fout, "COMPONENTMASKSHIFT ");

      for (i = 0; i < maskShiftLayer->numMaskShiftLayers(); i++) {
        fprintf(fout, "%s ", maskShiftLayer->maskShiftLayer(i));
      }
      fprintf(fout, ";\n");
      break;
    case defrScanchainCbkType:
      sc = (defiScanchain*) cl;
      fprintf(fout, "SCANCHAINS %s", sc->name());
      if (sc->hasStart()) {
        const char *a, *b;
        sc->start(&a, &b);
        fprintf(fout, " START %s %s", sc->name(), a);
      }
      if (sc->hasStop()) {
        const char *a, *b;
        sc->stop(&a, &b);
        fprintf(fout, " STOP %s %s", sc->name(), a);
      }
      if (sc->hasCommonInPin() || sc->hasCommonOutPin()) {
        fprintf(fout, " COMMONSCANPINS ");
        if (sc->hasCommonInPin()) {
          fprintf(fout, " ( IN  )");
        }
        if (sc->hasCommonOutPin()) {
          fprintf(fout, " ( OUT  )");
        }
      }
      fprintf(fout, "\n");
      if (sc->hasFloating()) {
        sc->floating(&size, &inst, &inPin, &outPin, &bits);
        for (i = 0; i < size; i++) {
          fprintf(fout, "SCANCHAINS %s FLOATING %s", sc->name(), inst[i]);
          if (inPin[i]) {
            fprintf(fout, " IN %s", inPin[i]);
          }
          if (outPin[i]) {
            fprintf(fout, " OUT %s", outPin[i]);
          }
          if (bits[i] != -1) {
            fprintf(fout, " BITS %d", bits[i]);
          }
          fprintf(fout, "\n");
        }
      }

      if (sc->hasOrdered()) {
        for (i = 0; i < sc->numOrderedLists(); i++) {
          sc->ordered(i, &size, &inst, &inPin, &outPin, &bits);
          for (j = 0; j < size; j++) {
            fprintf(fout, "SCANCHAINS %s ORDERED %s", sc->name(), inst[j]);
            if (inPin[j]) {
              fprintf(fout, " IN %s", inPin[j]);
            }
            if (outPin[j]) {
              fprintf(fout, " OUT %s", outPin[j]);
            }
            if (bits[j] != -1) {
              fprintf(fout, " BITS %d", bits[j]);
            }
            fprintf(fout, "\n");
          }
        }
      }

      if (sc->hasPartition()) {
        fprintf(fout,
                "SCANCHAINS %s PARTITION %s",
                sc->name(),
                sc->partitionName());
        if (sc->hasPartitionMaxBits()) {
          fprintf(fout, " MAXBITS %d", sc->partitionMaxBits());
        }
      }
      fprintf(fout, "\n");
      --numObjs;
      break;
    case defrIOTimingCbkType:
      iot = (defiIOTiming*) cl;
      fprintf(fout, "IOTIMING ( %s %s )\n", iot->inst(), iot->pin());
      if (iot->hasSlewRise()) {
        fprintf(fout,
                "IOTIMING %s RISE SLEWRATE %g %g\n",
                iot->inst(),
                checkDouble(iot->slewRiseMin()),
                checkDouble(iot->slewRiseMax()));
      }
      if (iot->hasSlewFall()) {
        fprintf(fout,
                "IOTIMING %s FALL SLEWRATE %g %g\n",
                iot->inst(),
                checkDouble(iot->slewFallMin()),
                checkDouble(iot->slewFallMax()));
      }
      if (iot->hasVariableRise()) {
        fprintf(fout,
                "IOTIMING %s RISE VARIABLE %g %g\n",
                iot->inst(),
                checkDouble(iot->variableRiseMin()),
                checkDouble(iot->variableRiseMax()));
      }
      if (iot->hasVariableFall()) {
        fprintf(fout,
                "IOTIMING %s FALL VARIABLE %g %g\n",
                iot->inst(),
                checkDouble(iot->variableFallMin()),
                checkDouble(iot->variableFallMax()));
      }
      if (iot->hasCapacitance()) {
        fprintf(fout,
                "IOTIMING %s CAPACITANCE %g\n",
                iot->inst(),
                checkDouble(iot->capacitance()));
      }
      if (iot->hasDriveCell()) {
        fprintf(
            fout, "IOTIMING %s DRIVECELL %s ", iot->inst(), iot->driveCell());
        if (iot->hasFrom()) {
          fprintf(fout, " FROMPIN %s ", iot->from());
        }
        if (iot->hasTo()) {
          fprintf(fout, " TOPIN %s ", iot->to());
        }
        if (iot->hasParallel()) {
          fprintf(fout, "PARALLEL %g", checkDouble(iot->parallel()));
        }
        fprintf(fout, "\n");
      }
      --numObjs;
      break;
    case defrFPCCbkType:
      fpc = (defiFPC*) cl;
      fprintf(fout, "FLOORPLAN %s ", fpc->name());
      if (fpc->isVertical()) {
        fprintf(fout, "VERTICAL ");
      }
      if (fpc->isHorizontal()) {
        fprintf(fout, "HORIZONTAL ");
      }
      if (fpc->hasAlign()) {
        fprintf(fout, "ALIGN ");
      }
      if (fpc->hasMax()) {
        fprintf(fout, "%g ", checkDouble(fpc->alignMax()));
      }
      if (fpc->hasMin()) {
        fprintf(fout, "%g ", checkDouble(fpc->alignMin()));
      }
      if (fpc->hasEqual()) {
        fprintf(fout, "%g ", checkDouble(fpc->equal()));
      }
      for (i = 0; i < fpc->numParts(); i++) {
        fpc->getPart(i, &corner, &typ, &name);
        if (corner == 'B') {
          fprintf(fout, "BOTTOMLEFT ");
        } else {
          fprintf(fout, "TOPRIGHT ");
        }
        if (typ == 'R') {
          fprintf(fout, "ROWS %s ", name);
        } else {
          fprintf(fout, "COMPS %s ", name);
        }
      }
      fprintf(fout, "\n");
      --numObjs;
      break;
    case defrTimingDisableCbkType:
      td = (defiTimingDisable*) cl;
      if (td->hasFromTo()) {
        fprintf(fout,
                "TIMINGDISABLE FROMPIN %s %s ",
                td->fromInst(),
                td->fromPin());
      }
      if (td->hasThru()) {
        fprintf(fout, " THRUPIN %s %s ", td->thruInst(), td->thruPin());
      }
      if (td->hasMacroFromTo()) {
        fprintf(fout,
                " MACRO %s FROMPIN %s %s ",
                td->macroName(),
                td->fromPin(),
                td->toPin());
      }
      if (td->hasMacroThru()) {
        fprintf(fout, " MACRO %s THRUPIN %s  ", td->macroName(), td->fromPin());
      }
      fprintf(fout, "\n");
      break;
    case defrPartitionCbkType:
      part = (defiPartition*) cl;
      fprintf(fout, "PARTITION %s ", part->name());
      if (part->isSetupRise() | part->isSetupFall() | part->isHoldRise()
          | part->isHoldFall()) {
        // has turnoff
        fprintf(fout, "TURNOFF ");
        if (part->isSetupRise()) {
          fprintf(fout, "SETUPRISE ");
        }
        if (part->isSetupFall()) {
          fprintf(fout, "SETUPFALL ");
        }
        if (part->isHoldRise()) {
          fprintf(fout, "HOLDRISE ");
        }
        if (part->isHoldFall()) {
          fprintf(fout, "HOLDFALL ");
        }
      }
      itemT = part->itemType();
      dir = part->direction();
      if (strcmp(itemT, "CLOCK") == 0) {
        if (dir == 'T') {  // toclockpin
          fprintf(
              fout, " TOCLOCKPIN %s %s ", part->instName(), part->pinName());
        }
        if (dir == 'F') {  // fromclockpin
          fprintf(
              fout, " FROMCLOCKPIN %s %s ", part->instName(), part->pinName());
        }
        if (part->hasMin()) {
          fprintf(fout,
                  "MIN %g %g ",
                  checkDouble(part->partitionMin()),
                  checkDouble(part->partitionMax()));
        }
        if (part->hasMax()) {
          fprintf(fout,
                  "MAX %g %g ",
                  checkDouble(part->partitionMin()),
                  checkDouble(part->partitionMax()));
        }
        fprintf(fout, "PINS ");
        for (i = 0; i < part->numPins(); i++) {
          fprintf(fout, "%s ", part->pin(i));
        }
      } else if (strcmp(itemT, "IO") == 0) {
        if (dir == 'T') {  // toiopin
          fprintf(fout, " TOIOPIN %s %s ", part->instName(), part->pinName());
        }
        if (dir == 'F') {  // fromiopin
          fprintf(fout, " FROMIOPIN %s %s ", part->instName(), part->pinName());
        }
      } else if (strcmp(itemT, "COMP") == 0) {
        if (dir == 'T') {  // tocomppin
          fprintf(fout, " TOCOMPPIN %s %s ", part->instName(), part->pinName());
        }
        if (dir == 'F') {  // fromcomppin
          fprintf(
              fout, " FROMCOMPPIN %s %s ", part->instName(), part->pinName());
        }
      }
      fprintf(fout, "\n");
      --numObjs;
      break;

    case defrPinPropCbkType:
      pprop = (defiPinProp*) cl;
      if (pprop->isPin()) {
        fprintf(fout, "PINPROP PIN %s ", pprop->pinName());
      } else {
        fprintf(fout, "PINPROP %s %s ", pprop->instName(), pprop->pinName());
      }
      fprintf(fout, "\n");
      if (pprop->numProps() > 0) {
        for (i = 0; i < pprop->numProps(); i++) {
          fprintf(fout,
                  "PINPROP PIN %s PROP %s %s\n",
                  pprop->pinName(),
                  pprop->propName(i),
                  pprop->propValue(i));
        }
      }
      --numObjs;
      break;

    case defrBlockageCbkType:
      block = (defiBlockage*) cl;
      if (block->hasLayer()) {
        fprintf(fout, "BLOCKAGE LAYER %s", block->layerName());
        if (block->hasComponent()) {
          fprintf(fout, " COMP %s", block->layerComponentName());
        }
        if (block->hasSlots()) {
          fprintf(fout, " SLOTS");
        }
        if (block->hasFills()) {
          fprintf(fout, " FILLS");
        }
        if (block->hasPushdown()) {
          fprintf(fout, " PUSHDOWN");
        }
        if (block->hasExceptpgnet()) {
          fprintf(fout, " EXCEPTPGNET");
        }
        if (block->hasMask()) {
          fprintf(fout, " MASK %d", block->mask());
        }
        if (block->hasSpacing()) {
          fprintf(fout, " SPACING %d", block->minSpacing());
        }
        if (block->hasDesignRuleWidth()) {
          fprintf(fout, " DESIGNRULEWIDTH %d", block->designRuleWidth());
        }
        fprintf(fout, "\n");
        for (i = 0; i < block->numRectangles(); i++) {
          fprintf(fout,
                  "BLOCKAGE LAYER %s RECT %d %d %d %d\n",
                  block->layerName(),
                  block->xl(i),
                  block->yl(i),
                  block->xh(i),
                  block->yh(i));
        }
        for (i = 0; i < block->numPolygons(); i++) {
          fprintf(fout, "BLOCKAGE LAYER %s POLYGON", block->layerName());
          defiPoints pts = block->getPolygon(i);
          for (j = 0; j < pts.numPoints; j++) {
            fprintf(fout, "%d %d ", pts.x[j], pts.y[j]);
          }
          fprintf(fout, "\n");
        }
      } else if (block->hasPlacement()) {
        fprintf(fout, "BLOCKAGE PLACEMENT");
        if (block->hasSoft()) {
          fprintf(fout, " SOFT");
        }
        if (block->hasPartial()) {
          fprintf(fout, " PARTIAL %g", block->placementMaxDensity());
        }
        if (block->hasComponent()) {
          fprintf(fout, " COMP %s", block->layerComponentName());
        }
        if (block->hasPushdown()) {
          fprintf(fout, " PUSHDOWN");
        }
        fprintf(fout, "\n");
        for (i = 0; i < block->numRectangles(); i++) {
          fprintf(fout,
                  "BLOCKAGE PLACEMENT RECT %d %d %d %d\n",
                  block->xl(i),
                  block->yl(i),
                  block->xh(i),
                  block->yh(i));
        }
      }
      --numObjs;
      break;

    case defrSlotCbkType:
      slot = (defiSlot*) cl;
      for (i = 0; i < slot->numRectangles(); i++) {
        fprintf(fout, "SLOT LAYER %s", slot->layerName());
        fprintf(fout,
                " RECT %d %d %d %d\n",
                slot->xl(i),
                slot->yl(i),
                slot->xh(i),
                slot->yh(i));
      }
      for (i = 0; i < slot->numPolygons(); i++) {
        fprintf(fout, "SLOT LAYER  POLYGON");
        defiPoints points = slot->getPolygon(i);
        for (j = 0; j < points.numPoints; j++) {
          fprintf(fout, " %d %d", points.x[j], points.y[j]);
        }
        fprintf(fout, "\n");
      }
      --numObjs;
      break;

    case defrFillCbkType:
      fill = (defiFill*) cl;
      for (i = 0; i < fill->numRectangles(); i++) {
        fprintf(fout, "FILL LAYER %s", fill->layerName());
        if (fill->layerMask()) {
          fprintf(fout, " MASK %d", fill->layerMask());
        }
        if (fill->hasLayerOpc()) {
          fprintf(fout, " OPC");
        }
        fprintf(fout,
                " RECT %d %d %d %d\n",
                fill->xl(i),
                fill->yl(i),
                fill->xh(i),
                fill->yh(i));
      }
      for (i = 0; i < fill->numPolygons(); i++) {
        fprintf(fout, "FILL LAYER %s POLYGON", fill->layerName());
        defiPoints points = fill->getPolygon(i);
        for (j = 0; j < points.numPoints; j++) {
          fprintf(fout, " %d %d", points.x[j], points.y[j]);
        }
        fprintf(fout, "\n");
      }
      if (fill->hasVia()) {
        fprintf(fout, "FILL VIA %s", fill->viaName());
        if (fill->viaTopMask() || fill->viaCutMask() || fill->viaBottomMask()) {
          fprintf(fout,
                  " MASK %d%d%d",
                  fill->viaTopMask(),
                  fill->viaCutMask(),
                  fill->viaBottomMask());
        }
        if (fill->hasViaOpc()) {
          fprintf(fout, " OPC\n");
        }
        for (i = 0; i < fill->numViaPts(); i++) {
          defiPoints points = fill->getViaPts(i);
          for (j = 0; j < points.numPoints; j++) {
            fprintf(fout, " %d %d", points.x[j], points.y[j]);
          }
        }
        fprintf(fout, "\n");
      }
      --numObjs;
      break;

    case defrStylesCbkType:
      styles = (defiStyles*) cl;
      fprintf(fout, "STYLE %d", styles->style());
      {
        defiPoints points = styles->getPolygon();
        for (j = 0; j < points.numPoints; j++) {
          fprintf(fout, " %d %d", points.x[j], points.y[j]);
        }
        fprintf(fout, "\n");
        --numObjs;
      }
      break;

    default:
      fprintf(fout, "BOGUS callback to cls.\n");
      return 1;
  }
  return 0;
}

int dn(defrCallbackType_e c, const char* h, defiUserData ud)
{
  checkType(c);
  if (ud != userData) {
    dataError();
  }
  fprintf(fout, "DIVIDERCHAR \"%s\" \n", h);
  return 0;
}

int ext(defrCallbackType_e t, const char* c, defiUserData ud)
{
  char* name;

  checkType(t);
  if (ud != userData) {
    dataError();
  }

  switch (t) {
    case defrNetExtCbkType:
      name = address("net");
      break;
    case defrComponentExtCbkType:
      name = address("component");
      break;
    case defrPinExtCbkType:
      name = address("pin");
      break;
    case defrViaExtCbkType:
      name = address("via");
      break;
    case defrNetConnectionExtCbkType:
      name = address("net connection");
      break;
    case defrGroupExtCbkType:
      name = address("group");
      break;
    case defrScanChainExtCbkType:
      name = address("scanchain");
      break;
    case defrIoTimingsExtCbkType:
      name = address("io timing");
      break;
    case defrPartitionsExtCbkType:
      name = address("partition");
      break;
    default:
      name = address("BOGUS");
      return 1;
  }
  fprintf(fout, "EXTENSION %s %s\n", name, c);
  return 0;
}

//========

int diffDefReadFile(char* inFile,
                    char* outFile,
                    char* ignorePinExtra,
                    char* ignoreRowName,
                    char* ignoreViaName,
                    char* netSegComp)
{
  userData = (void*) 0x01020304;
  defrInit();

  defrSetDesignCbk(dname);
  defrSetTechnologyCbk(tname);
  defrSetPropCbk(prop);
  defrSetNetCbk(netf);
  defrSetSNetCbk(snetf);
  defrSetComponentMaskShiftLayerCbk(compMSL);
  defrSetComponentCbk(compf);
  defrSetAddPathToNet();
  defrSetHistoryCbk(hist);
  defrSetConstraintCbk(constraint);
  defrSetAssertionCbk(constraint);
  defrSetDividerCbk(dn);
  defrSetBusBitCbk(bbn);
  defrSetNonDefaultCbk(ndr);

  // All of the extensions point to the same function.
  defrSetNetExtCbk(ext);
  defrSetComponentExtCbk(ext);
  defrSetPinExtCbk(ext);
  defrSetViaExtCbk(ext);
  defrSetNetConnectionExtCbk(ext);
  defrSetGroupExtCbk(ext);
  defrSetScanChainExtCbk(ext);
  defrSetIoTimingsExtCbk(ext);
  defrSetPartitionsExtCbk(ext);

  defrSetUnitsCbk(units);
  defrSetVersionCbk(vers);
  defrSetCaseSensitiveCbk(casesens);

  // The following calls are an example of using one function "cls"
  // to be the callback for many DIFFERENT types of constructs.
  // We have to cast the function type to meet the requirements
  // of each different set function.
  defrSetSiteCbk((defrSiteCbkFnType) cls);
  defrSetCanplaceCbk((defrSiteCbkFnType) cls);
  defrSetCannotOccupyCbk((defrSiteCbkFnType) cls);
  defrSetDieAreaCbk((defrBoxCbkFnType) cls);
  defrSetPinCapCbk((defrPinCapCbkFnType) cls);
  defrSetPinCbk((defrPinCbkFnType) cls);
  defrSetPinPropCbk((defrPinPropCbkFnType) cls);
  defrSetDefaultCapCbk((defrIntegerCbkFnType) cls);
  defrSetRowCbk((defrRowCbkFnType) cls);
  defrSetTrackCbk((defrTrackCbkFnType) cls);
  defrSetGcellGridCbk((defrGcellGridCbkFnType) cls);
  defrSetViaCbk((defrViaCbkFnType) cls);
  defrSetRegionCbk((defrRegionCbkFnType) cls);
  defrSetGroupCbk((defrGroupCbkFnType) cls);
  defrSetScanchainCbk((defrScanchainCbkFnType) cls);
  defrSetIOTimingCbk((defrIOTimingCbkFnType) cls);
  defrSetFPCCbk((defrFPCCbkFnType) cls);
  defrSetTimingDisableCbk((defrTimingDisableCbkFnType) cls);
  defrSetPartitionCbk((defrPartitionCbkFnType) cls);
  defrSetBlockageCbk((defrBlockageCbkFnType) cls);
  defrSetSlotCbk((defrSlotCbkFnType) cls);
  defrSetFillCbk((defrFillCbkFnType) cls);

  if (strcmp(ignorePinExtra, "0") != 0) {
    ignorePE = 1;
  }

  if (strcmp(ignoreRowName, "0") != 0) {
    ignoreRN = 1;
  }

  if (strcmp(ignoreViaName, "0") != 0) {
    ignoreVN = 1;
  }

  if (strcmp(netSegComp, "0") != 0) {
    netSeCmp = 1;
  }

  FILE* f = fopen(inFile, "r");
  if (f == nullptr) {
    fprintf(stderr, "Couldn't open input file '%s'\n", inFile);
    return (2);
  }

  fout = fopen(outFile, "w");
  if (fout == nullptr) {
    fprintf(stderr, "Couldn't open output file '%s'\n", outFile);
    fclose(f);
    return (2);
  }

  const int res = defrRead(f, inFile, userData, 1);

  fclose(f);
  fclose(fout);

  return 0;
}
