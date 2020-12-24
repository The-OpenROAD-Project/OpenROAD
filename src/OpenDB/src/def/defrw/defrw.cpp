// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2017, Cadence Design Systems
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifndef WIN32
#   include <unistd.h>
#endif /* not WIN32 */
#include "defrReader.hpp"
#include "defiAlias.hpp"

char defaultName[64];
char defaultOut[64];

// Global variables
FILE* fout;
void* userData;
int numObjs;
int isSumSet;      // to keep track if within SUM
int isProp = 0;    // for PROPERTYDEFINITIONS
int begOperand;    // to keep track for constraint, to print - as the 1st char
static double curVer = 0;
static int setSNetWireCbk = 0;
static int isSessionless = 0;
static int ignoreRowNames = 0;
static int ignoreViaNames = 0;
static int testDebugPrint = 0;  // test for ccr1488696

// TX_DIR:TRANSLATION ON

void myLogFunction(const char* errMsg){
   fprintf(fout, "ERROR: found error: %s\n", errMsg);
}

void myWarningLogFunction(const char* errMsg){
   fprintf(fout, "WARNING: found error: %s\n", errMsg);
}

void dataError() {
  fprintf(fout, "ERROR: returned user data is not correct!\n");
}

void checkType(defrCallbackType_e c) {
  if (c >= 0 && c <= defrDesignEndCbkType) {
    // OK
  } else {
    fprintf(fout, "ERROR: callback type is out of bounds!\n");
  }
}


int done(defrCallbackType_e c, void*, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
  fprintf(fout, "END DESIGN\n");
  return 0;
}

int endfunc(defrCallbackType_e c, void*, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
  return 0;
}


char* orientStr(int orient) {
  switch (orient) {
      case 0: return ((char*)"N");
      case 1: return ((char*)"W");
      case 2: return ((char*)"S");
      case 3: return ((char*)"E");
      case 4: return ((char*)"FN");
      case 5: return ((char*)"FW");
      case 6: return ((char*)"FS");
      case 7: return ((char*)"FE");
  };
  return ((char*)"BOGUS");
}

int compMSL(defrCallbackType_e c, defiComponentMaskShiftLayer* co, defiUserData ud) {
  int i;

  checkType(c);
  if (ud != userData) dataError();

    if (co->numMaskShiftLayers()) {
	fprintf(fout, "\nCOMPONENTMASKSHIFT ");
	
	for (i = 0; i < co->numMaskShiftLayers(); i++) {
           fprintf(fout, "%s ", co->maskShiftLayer(i));
	}
	fprintf(fout, ";\n");
    }

  return 0;
}

int compf(defrCallbackType_e c, defiComponent* co, defiUserData ud) {
  if (testDebugPrint) {
      co->print(fout);
  } else {
      int i;

      checkType(c);
      if (ud != userData) dataError();
    //  missing GENERATE, FOREIGN
        fprintf(fout, "- %s %s ", co->id(),
                co->name());
    //    co->changeIdAndName("idName", "modelName");
    //    fprintf(fout, "%s %s ", co->id(),
    //            co->name());
        if (co->hasNets()) {
            for (i = 0; i < co->numNets(); i++)
                 fprintf(fout, "%s ", co->net(i));
        }
        if (co->isFixed()) 
            fprintf(fout, "+ FIXED %d %d %s ",
                    co->placementX(),
                    co->placementY(),
                    //orientStr(co->placementOrient()));
                    co->placementOrientStr());
        if (co->isCover()) 
            fprintf(fout, "+ COVER %d %d %s ",
                    co->placementX(),
                    co->placementY(),
                    orientStr(co->placementOrient()));
        if (co->isPlaced()) 
            fprintf(fout,"+ PLACED %d %d %s ",
                    co->placementX(),
                    co->placementY(),
                    orientStr(co->placementOrient()));
        if (co->isUnplaced()) {
            fprintf(fout,"+ UNPLACED ");
            if ((co->placementX() != -1) ||
                (co->placementY() != -1))
               fprintf(fout,"%d %d %s ",
                       co->placementX(),
                       co->placementY(),
                       orientStr(co->placementOrient()));
        }
        if (co->hasSource())
            fprintf(fout, "+ SOURCE %s ", co->source());
        if (co->hasGenerate()) {
            fprintf(fout, "+ GENERATE %s ", co->generateName());
            if (co->macroName() &&
                *(co->macroName()))
               fprintf(fout, "%s ", co->macroName());
        }
        if (co->hasWeight())
            fprintf(fout, "+ WEIGHT %d ", co->weight());
        if (co->hasEEQ())
            fprintf(fout, "+ EEQMASTER %s ", co->EEQ());
        if (co->hasRegionName())
            fprintf(fout, "+ REGION %s ", co->regionName());
        if (co->hasRegionBounds()) {
            int *xl, *yl, *xh, *yh;
            int size;
            co->regionBounds(&size, &xl, &yl, &xh, &yh);
            for (i = 0; i < size; i++) { 
                fprintf(fout, "+ REGION %d %d %d %d \n",
                        xl[i], yl[i], xh[i], yh[i]);
            }
        }
        if (co->maskShiftSize()) {
            fprintf(fout, "+ MASKSHIFT ");

            for (int i = co->maskShiftSize()-1; i >= 0; i--) {
                fprintf(fout, "%d", co->maskShift(i));
            }
            fprintf(fout, "\n");
        }
        if (co->hasHalo()) {
            int left, bottom, right, top;
            (void) co->haloEdges(&left, &bottom, &right, &top);
            fprintf(fout, "+ HALO ");
            if (co->hasHaloSoft())
               fprintf(fout, "SOFT ");
            fprintf(fout, "%d %d %d %d\n", left, bottom, right, top);
        }
        if (co->hasRouteHalo()) {
            fprintf(fout, "+ ROUTEHALO %d %s %s\n", co->haloDist(),
                    co->minLayer(), co->maxLayer());
        }
        if (co->hasForeignName()) {
            fprintf(fout, "+ FOREIGN %s %d %d %s %d ",
                    co->foreignName(), co->foreignX(),
                    co->foreignY(), co->foreignOri(),
                    co->foreignOrient());
        }
        if (co->numProps()) {
            for (i = 0; i < co->numProps(); i++) {
                fprintf(fout, "+ PROPERTY %s %s ", co->propName(i),
                        co->propValue(i));
                switch (co->propType(i)) {
                   case 'R': fprintf(fout, "REAL ");
                             break;
                   case 'I': fprintf(fout, "INTEGER ");
                             break;
                   case 'S': fprintf(fout, "STRING ");
                             break;
                   case 'Q': fprintf(fout, "QUOTESTRING ");
                             break;
                   case 'N': fprintf(fout, "NUMBER ");
                             break;
                }
            }
        }
        fprintf(fout, ";\n");
        --numObjs;
        if (numObjs <= 0)
            fprintf(fout, "END COMPONENTS\n");
    }

    return 0;
}


int netpath(defrCallbackType_e, defiNet*, defiUserData) {
  fprintf(fout, "\n");

  fprintf (fout, "Callback of partial path for net\n");

  return 0;
}


int netNamef(defrCallbackType_e c, const char* netName, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
    fprintf(fout, "- %s ", netName);
  return 0;
}

int subnetNamef(defrCallbackType_e c, const char* subnetName, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
    if (curVer >= 5.6)
      fprintf(fout, "   + SUBNET CBK %s ", subnetName);
  return 0;
}

int nondefRulef(defrCallbackType_e c, const char* ruleName, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
    if (curVer >= 5.6)
      fprintf(fout, "   + NONDEFAULTRULE CBK %s ", ruleName);
  return 0;
}

int netf(defrCallbackType_e c, defiNet* net, defiUserData ud) {
  // For net and special net.
  int        i, j, k, w, x, y, z, count, newLayer;
  defiPath*  p;
  defiSubnet *s;
  int        path;
  defiVpin   *vpin;
  // defiShield *noShield;
  defiWire   *wire;

  checkType(c);
  if (ud != userData) dataError();
  if (c != defrNetCbkType)
      fprintf(fout, "BOGUS NET TYPE  ");
  if (net->pinIsMustJoin(0))
      fprintf(fout, "- MUSTJOIN ");
// 5/6/2004 - don't need since I have a callback for the name
//  else
//      fprintf(fout, "- %s ", net->name());
 
//  net->changeNetName("newNetName");
//  fprintf(fout, "%s ", net->name());
  count = 0;
  // compName & pinName
  for (i = 0; i < net->numConnections(); i++) {
      // set the limit of only 5 items per line
      count++;
      if (count >= 5) {
          fprintf(fout, "\n");
          count = 0;
      }
      fprintf(fout, "( %s %s ) ", net->instance(i),
              net->pin(i));
//      net->changeInstance("newInstance", i);
//      net->changePin("newPin", i);
//      fprintf(fout, "( %s %s ) ", net->instance(i),
//              net->pin(i));
      if (net->pinIsSynthesized(i))
          fprintf(fout, "+ SYNTHESIZED ");
  }

  if (net->hasNonDefaultRule())
      fprintf(fout, "+ NONDEFAULTRULE %s\n", net->nonDefaultRule());

  for (i = 0; i < net->numVpins(); i++) {
      vpin = net->vpin(i);
      fprintf(fout, "  + %s", vpin->name());
      if (vpin->layer()) 
          fprintf(fout, " %s", vpin->layer());
      fprintf(fout, " %d %d %d %d", vpin->xl(), vpin->yl(), vpin->xh(),
              vpin->yh());
      if (vpin->status() != ' ') {
          fprintf(fout, " %c", vpin->status());
          fprintf(fout, " %d %d", vpin->xLoc(), vpin->yLoc());
          if (vpin->orient() != -1)
              fprintf(fout, " %s", orientStr(vpin->orient()));
      }
      fprintf(fout, "\n");
  }

  // regularWiring
  if (net->numWires()) {
     for (i = 0; i < net->numWires(); i++) {
        newLayer = 0;
        wire = net->wire(i);
        fprintf(fout, "\n  + %s ", wire->wireType());
        count = 0;
        for (j = 0; j < wire->numPaths(); j++) {
           p = wire->path(j);
           p->initTraverse();
           while ((path = (int)p->next()) != DEFIPATH_DONE) {
              count++;
              // Don't want the line to be too long
              if (count >= 5) {
                  fprintf(fout, "\n");
                  count = 0;
              } 
              switch (path) {
                case DEFIPATH_LAYER:
                     if (newLayer == 0) {
                         fprintf(fout, "%s ", p->getLayer());
                         newLayer = 1;
                     } else
                         fprintf(fout, "NEW %s ", p->getLayer());
                     break;
		case DEFIPATH_MASK:
		     fprintf(fout, "MASK %d ", p->getMask());
                     break;
                case DEFIPATH_VIAMASK:
                     fprintf(fout, "MASK %d%d%d ", 
                             p->getViaTopMask(), 
                             p->getViaCutMask(),
                             p->getViaBottomMask());
                     break;
                case DEFIPATH_VIA:
                     fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                     break;
                case DEFIPATH_VIAROTATION:
                     fprintf(fout, "%s ", 
                             orientStr(p->getViaRotation()));
                     break;
		case DEFIPATH_RECT:
		     p->getViaRect(&w, &x, &y, &z);
                     fprintf(fout, "RECT ( %d %d %d %d ) ", w, x, y, z);
                     break;
		case DEFIPATH_VIRTUALPOINT:
		     p->getVirtualPoint(&x, &y);
		     fprintf(fout, "VIRTUAL ( %d %d ) ", x, y);
                     break;
                case DEFIPATH_WIDTH:
                     fprintf(fout, "%d ", p->getWidth());
                     break;
                case DEFIPATH_POINT:
                     p->getPoint(&x, &y);
                     fprintf(fout, "( %d %d ) ", x, y);
                     break;
                case DEFIPATH_FLUSHPOINT:
                     p->getFlushPoint(&x, &y, &z);
                     fprintf(fout, "( %d %d %d ) ", x, y, z);
                     break;
                case DEFIPATH_TAPER:
                     fprintf(fout, "TAPER ");
                     break;
                case DEFIPATH_TAPERRULE:
                     fprintf(fout, "TAPERRULE %s ",p->getTaperRule());
                     break;
                case DEFIPATH_STYLE:
                     fprintf(fout, "STYLE %d ",p->getStyle());
                     break;
              }
           }
        }
        fprintf(fout, "\n");
        count = 0;
     }
  }

  // SHIELDNET
  if (net->numShieldNets()) {
     for (i = 0; i < net->numShieldNets(); i++) 
         fprintf(fout, "\n  + SHIELDNET %s", net->shieldNet(i));
  }
/* obsolete in 5.4
  if (net->numNoShields()) {
     for (i = 0; i < net->numNoShields(); i++) { 
         noShield = net->noShield(i); 
         fprintf(fout, "\n  + NOSHIELD ");
         newLayer = 0;
         for (j = 0; j < noShield->numPaths(); j++) {
            p = noShield->path(j);
            p->initTraverse();
            while ((path = (int)p->next()) != DEFIPATH_DONE) {
               count++;
               // Don't want the line to be too long
               if (count >= 5) {
                   fprintf(fout, "\n");
                   count = 0;
               }
               switch (path) {
                 case DEFIPATH_LAYER:
                      if (newLayer == 0) {
                          fprintf(fout, "%s ", p->getLayer());
                          newLayer = 1;
                      } else
                          fprintf(fout, "NEW %s ", p->getLayer());
                      break;
                 case DEFIPATH_VIA:
                      fprintf(fout, "%s ", p->getVia());
                      break;
                 case DEFIPATH_VIAROTATION:
                      fprintf(fout, "%s ", 
                             orientStr(p->getViaRotation()));
                      break;
                 case DEFIPATH_WIDTH:
                      fprintf(fout, "%d ", p->getWidth());
                      break;
                 case DEFIPATH_POINT:
                      p->getPoint(&x, &y);
                      fprintf(fout, "( %d %d ) ", x, y);
                      break;
                 case DEFIPATH_FLUSHPOINT:
                      p->getFlushPoint(&x, &y, &z);
                      fprintf(fout, "( %d %d %d ) ", x, y, z);
                      break;
                 case DEFIPATH_TAPER:
                      fprintf(fout, "TAPER ");
                      break;
                 case DEFIPATH_TAPERRULE:
                      fprintf(fout, "TAPERRULE %s ",
                              p->getTaperRule());
                      break;
               }
            }
         }
     }
  }
*/

  if (net->hasSubnets()) {
     for (i = 0; i < net->numSubnets(); i++) {
        s = net->subnet(i);
        fprintf(fout, "\n");
 
        if (s->numConnections()) {
           if (s->pinIsMustJoin(0))
              fprintf(fout, "- MUSTJOIN ");
           else
              fprintf(fout, "  + SUBNET %s ", s->name());
           for (j = 0; j < s->numConnections(); j++)
              fprintf(fout, " ( %s %s )\n", s->instance(j),
                      s->pin(j));

           // regularWiring
           if (s->numWires()) {
              for (k = 0; k < s->numWires(); k++) {
                 newLayer = 0;
                 wire = s->wire(k);
                 fprintf(fout, "  %s ", wire->wireType());
                 count = 0;
                 for (j = 0; j < wire->numPaths(); j++) {
                    p = wire->path(j);
                    p->initTraverse();
                    while ((path = (int)p->next()) != DEFIPATH_DONE) {
                       count++;
                       // Don't want the line to be too long
                       if (count >= 5) {
                           fprintf(fout, "\n");
                           count = 0;
                       } 
                       switch (path) {
                         case DEFIPATH_LAYER:
                              if (newLayer == 0) {
                                  fprintf(fout, "%s ", p->getLayer());
                                  newLayer = 1;
                              } else
                                  fprintf(fout, "NEW %s ",
                                          p->getLayer());
                              break;
                         case DEFIPATH_VIA:
                              fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                              break;
                         case DEFIPATH_VIAROTATION:
                              fprintf(fout, "%s ",
                                      p->getViaRotationStr());
                              break;
                         case DEFIPATH_WIDTH:
                              fprintf(fout, "%d ", p->getWidth());
                              break;
                         case DEFIPATH_POINT:
                              p->getPoint(&x, &y);
                              fprintf(fout, "( %d %d ) ", x, y);
                              break;
                         case DEFIPATH_FLUSHPOINT:
                              p->getFlushPoint(&x, &y, &z);
                              fprintf(fout, "( %d %d %d ) ", x, y, z);
                              break;
                         case DEFIPATH_TAPER:
                              fprintf(fout, "TAPER ");
                              break;
                         case DEFIPATH_TAPERRULE:
                              fprintf(fout, "TAPERRULE  %s ",
                                      p->getTaperRule());
                              break;
                         case DEFIPATH_STYLE:
                              fprintf(fout, "STYLE  %d ",
                                      p->getStyle());
                              break;
                       }
                    }
                 }
              }
           }
         }
      }
   }

  if (net->numProps()) {
    for (i = 0; i < net->numProps(); i++) {
        fprintf(fout, "  + PROPERTY %s ", net->propName(i));
        switch (net->propType(i)) {
           case 'R': fprintf(fout, "%g REAL ", net->propNumber(i));
                     break;
           case 'I': fprintf(fout, "%g INTEGER ", net->propNumber(i));
                     break;
           case 'S': fprintf(fout, "%s STRING ", net->propValue(i));
                     break;
           case 'Q': fprintf(fout, "%s QUOTESTRING ", net->propValue(i));
                     break;
           case 'N': fprintf(fout, "%g NUMBER ", net->propNumber(i));
                     break;
        }
        fprintf(fout, "\n");
    }
  }

  if (net->hasWeight())
    fprintf(fout, "+ WEIGHT %d ", net->weight());
  if (net->hasCap())
    fprintf(fout, "+ ESTCAP %g ", net->cap());
  if (net->hasSource())
    fprintf(fout, "+ SOURCE %s ", net->source());
  if (net->hasFixedbump())
    fprintf(fout, "+ FIXEDBUMP ");
  if (net->hasFrequency())
    fprintf(fout, "+ FREQUENCY %g ", net->frequency());
  if (net->hasPattern())
    fprintf(fout, "+ PATTERN %s ", net->pattern());
  if (net->hasOriginal())
    fprintf(fout, "+ ORIGINAL %s ", net->original());
  if (net->hasUse())
    fprintf(fout, "+ USE %s ", net->use());

  fprintf (fout, ";\n");
  --numObjs;
  if (numObjs <= 0)
      fprintf(fout, "END NETS\n");
  return 0;
}


int snetpath(defrCallbackType_e c, defiNet* ppath, defiUserData ud) {
  int         i, j, x, y, z, count, newLayer;
  char*       layerName;
  double      dist, left, right;
  defiPath*   p;
  defiSubnet  *s;
  int         path;
  defiShield* shield;
  defiWire*   wire;
  int         numX, numY, stepX, stepY;

  if (c != defrSNetPartialPathCbkType)
      return 1;
  if (ud != userData) dataError();

  fprintf (fout, "SPECIALNET partial data\n");

  fprintf(fout, "- %s ", ppath->name());

  count = 0;
  // compName & pinName
  for (i = 0; i < ppath->numConnections(); i++) {
      // set the limit of only 5 items print out in one line
      count++;
      if (count >= 5) {
          fprintf(fout, "\n");
          count = 0;
      }
      fprintf (fout, "( %s %s ) ", ppath->instance(i),
               ppath->pin(i));
      if (ppath->pinIsSynthesized(i))
          fprintf(fout, "+ SYNTHESIZED ");
  }

  // specialWiring
  // POLYGON
  if (ppath->numPolygons()) {
     defiPoints points;
    for (i = 0; i < ppath->numPolygons(); i++) {
      fprintf(fout, "\n  + POLYGON %s ", ppath->polygonName(i));
      points = ppath->getPolygon(i);
      for (j = 0; j < points.numPoints; j++)
        fprintf(fout, "%d %d ", points.x[j], points.y[j]);
    }
  }
  // RECT
  if (ppath->numRectangles()) {
     for (i = 0; i < ppath->numRectangles(); i++) {
       fprintf(fout, "\n  + RECT %s %d %d %d %d", ppath->rectName(i),
               ppath->xl(i), ppath->yl(i),
               ppath->xh(i), ppath->yh(i));
     }
  }

  // COVER, FIXED, ROUTED or SHIELD
  if (ppath->numWires()) {
     newLayer = 0;
     for (i = 0; i < ppath->numWires(); i++) {
        newLayer = 0;
        wire = ppath->wire(i);
        fprintf(fout, "\n  + %s ", wire->wireType());
        if (strcmp (wire->wireType(), "SHIELD") == 0)
           fprintf(fout, "%s ", wire->wireShieldNetName());
        for (j = 0; j < wire->numPaths(); j++) {
           p = wire->path(j);
           p->initTraverse();
           while ((path = (int)p->next()) != DEFIPATH_DONE) {
              count++;
              // Don't want the line to be too long
              if (count >= 5) {
                  fprintf(fout, "\n");
                  count = 0;
              }
              switch (path) {
                case DEFIPATH_LAYER:
                     if (newLayer == 0) {
                         fprintf(fout, "%s ", p->getLayer());
                         newLayer = 1;
                     } else
                         fprintf(fout, "NEW %s ", p->getLayer());
                     break;
                case DEFIPATH_VIA:
                     fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                     break;
                case DEFIPATH_VIAROTATION:
                     fprintf(fout, "%s ",
                             orientStr(p->getViaRotation()));
                     break;
                case DEFIPATH_VIADATA:
                     p->getViaData(&numX, &numY, &stepX, &stepY);
                     fprintf(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                             stepX, stepY);
                     break;
                case DEFIPATH_WIDTH:
                     fprintf(fout, "%d ", p->getWidth());
                     break;
		case DEFIPATH_MASK:
		     fprintf(fout, "MASK %d ", p->getMask());
		     break;
                case DEFIPATH_VIAMASK:
                    fprintf(fout, "MASK %d%d%d ", 
                            p->getViaTopMask(), 
                            p->getViaCutMask(),
                            p->getViaBottomMask());
                    break;
                case DEFIPATH_POINT:
                     p->getPoint(&x, &y);
                     fprintf(fout, "( %d %d ) ", x, y);
                     break;
                case DEFIPATH_FLUSHPOINT:
                     p->getFlushPoint(&x, &y, &z);
                     fprintf(fout, "( %d %d %d ) ", x, y, z);
                     break;
                case DEFIPATH_TAPER:
                     fprintf(fout, "TAPER ");
                     break;
                case DEFIPATH_SHAPE:
                     fprintf(fout, "+ SHAPE %s ", p->getShape());
                     break;
                case DEFIPATH_STYLE:
                     fprintf(fout, "+ STYLE %d ", p->getStyle());
                     break;
              }
           }
        }
        fprintf(fout, "\n");
        count = 0;
     }
  }

  if (ppath->hasSubnets()) {
    for (i = 0; i < ppath->numSubnets(); i++) {
      s = ppath->subnet(i);
      if (s->numConnections()) {
          if (s->pinIsMustJoin(0))
              fprintf(fout, "- MUSTJOIN ");
          else
              fprintf(fout, "- %s ", s->name());
          for (j = 0; j < s->numConnections(); j++) {
              fprintf(fout, " ( %s %s )\n", s->instance(j),
                      s->pin(j));
        }
      }

      // regularWiring
      if (s->numWires()) {
         for (i = 0; i < s->numWires(); i++) {
            wire = s->wire(i);
            fprintf(fout, "  + %s ", wire->wireType());
            for (j = 0; j < wire->numPaths(); j++) {
              p = wire->path(j);
              p->print(fout);
            }
         }
      }
    }
  }

  if (ppath->numProps()) {
    for (i = 0; i < ppath->numProps(); i++) {
        if (ppath->propIsString(i))
           fprintf(fout, "  + PROPERTY %s %s ", ppath->propName(i),
                   ppath->propValue(i));
        if (ppath->propIsNumber(i))
           fprintf(fout, "  + PROPERTY %s %g ", ppath->propName(i),
                   ppath->propNumber(i));
        switch (ppath->propType(i)) {
           case 'R': fprintf(fout, "REAL ");
                     break;
           case 'I': fprintf(fout, "INTEGER ");
                     break;
           case 'S': fprintf(fout, "STRING ");
                     break;
           case 'Q': fprintf(fout, "QUOTESTRING ");
                     break;
           case 'N': fprintf(fout, "NUMBER ");
                     break;
        }
        fprintf(fout, "\n");
    }
  }

  // SHIELD
  count = 0;
  // testing the SHIELD for 5.3, obsolete in 5.4
  if (ppath->numShields()) {
    for (i = 0; i < ppath->numShields(); i++) {
       shield = ppath->shield(i);
       fprintf(fout, "\n  + SHIELD %s ", shield->shieldName());
       newLayer = 0;
       for (j = 0; j < shield->numPaths(); j++) {
          p = shield->path(j);
          p->initTraverse();
          while ((path = (int)p->next()) != DEFIPATH_DONE) {
             count++;
             // Don't want the line to be too long
             if (count >= 5) {
                 fprintf(fout, "\n");
                 count = 0;
             }
             switch (path) {
               case DEFIPATH_LAYER:
                    if (newLayer == 0) {
                        fprintf(fout, "%s ", p->getLayer());
                        newLayer = 1;
                    } else
                        fprintf(fout, "NEW %s ", p->getLayer());
                    break;
               case DEFIPATH_VIA:
                    fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                    break;
               case DEFIPATH_VIAROTATION:
                    if (newLayer)
                       fprintf(fout, "%s ",
                               orientStr(p->getViaRotation()));
                    else
                       fprintf(fout, "Str %s ",
                               p->getViaRotationStr());
                    break;
               case DEFIPATH_WIDTH:
                    fprintf(fout, "%d ", p->getWidth());
                    break;
	       case DEFIPATH_MASK:
		    fprintf(fout, "MASK %d ", p->getMask());
		    break;
               case DEFIPATH_VIAMASK:
                    fprintf(fout, "MASK %d%d%d ", 
                            p->getViaTopMask(), 
                            p->getViaCutMask(),
                            p->getViaBottomMask());
                    break;
               case DEFIPATH_POINT:
                    p->getPoint(&x, &y);
                    fprintf(fout, "( %d %d ) ", x, y);
                    break;
               case DEFIPATH_FLUSHPOINT:
                    p->getFlushPoint(&x, &y, &z);
                    fprintf(fout, "( %d %d %d ) ", x, y, z);
                    break;
               case DEFIPATH_TAPER:
                    fprintf(fout, "TAPER ");
                    break;
               case DEFIPATH_SHAPE:
                    fprintf(fout, "+ SHAPE %s ", p->getShape());
                    break;
               case DEFIPATH_STYLE:
                    fprintf(fout, "+ STYLE %d ", p->getStyle());
             }
          }
       }
    }
  }

  // layerName width
  if (ppath->hasWidthRules()) {
    for (i = 0; i < ppath->numWidthRules(); i++) {
        ppath->widthRule(i, &layerName, &dist);
        fprintf (fout, "\n  + WIDTH %s %g ", layerName, dist);
    }
  }

  // layerName spacing
  if (ppath->hasSpacingRules()) {
    for (i = 0; i < ppath->numSpacingRules(); i++) {
        ppath->spacingRule(i, &layerName, &dist, &left, &right);
        if (left == right)
            fprintf (fout, "\n  + SPACING %s %g ", layerName, dist);
        else
            fprintf (fout, "\n  + SPACING %s %g RANGE %g %g ",
                     layerName, dist, left, right);
    }
  }

  if (ppath->hasFixedbump())
    fprintf(fout, "\n  + FIXEDBUMP ");
  if (ppath->hasFrequency())
    fprintf(fout, "\n  + FREQUENCY %g ", ppath->frequency());
  if (ppath->hasVoltage())
    fprintf(fout, "\n  + VOLTAGE %g ", ppath->voltage());
  if (ppath->hasWeight())
    fprintf(fout, "\n  + WEIGHT %d ", ppath->weight());
  if (ppath->hasCap())
    fprintf(fout, "\n  + ESTCAP %g ", ppath->cap());
  if (ppath->hasSource())
    fprintf(fout, "\n  + SOURCE %s ", ppath->source());
  if (ppath->hasPattern())
    fprintf(fout, "\n  + PATTERN %s ", ppath->pattern());
  if (ppath->hasOriginal())
    fprintf(fout, "\n  + ORIGINAL %s ", ppath->original());
  if (ppath->hasUse())
    fprintf(fout, "\n  + USE %s ", ppath->use());

  fprintf(fout, "\n");

  return 0;
}


int snetwire(defrCallbackType_e c, defiNet* ppath, defiUserData ud) {
  int         i, j, x, y, z, count = 0, newLayer;
  defiPath*   p;
  int         path;
  defiWire*   wire;
  defiShield* shield;
  int         numX, numY, stepX, stepY;

  if (c != defrSNetWireCbkType)
      return 1;
  if (ud != userData) dataError();

  fprintf (fout, "SPECIALNET wire data\n");

  fprintf(fout, "- %s ", ppath->name());

  // POLYGON
  if (ppath->numPolygons()) {
	defiPoints points;
	for (i = 0; i < ppath->numPolygons(); i++) {
	    fprintf(fout, "\n  + POLYGON %s ", ppath->polygonName(i));

	    points = ppath->getPolygon(i);

	    for (j = 0; j < points.numPoints; j++) {
		fprintf(fout, "%d %d ", points.x[j], points.y[j]);
	    }
	}
  // RECT
  } 
  if (ppath->numRectangles()) {
	for (i = 0; i < ppath->numRectangles(); i++) {
	    fprintf(fout, "\n  + RECT %s %d %d %d %d", ppath->rectName(i),
            ppath->xl(i), ppath->yl(i),
            ppath->xh(i), ppath->yh(i));
	}
  }
  // VIA
  if (ppath->numViaSpecs()) {
	for (i = 0; i < ppath->numViaSpecs(); i++) {
	    fprintf(fout, "\n  + VIA %s ", ppath->viaName(i)),
	    fprintf(fout, " %s", ppath->viaOrientStr(i));

	    defiPoints points = ppath->getViaPts(i);

	    for (int j = 0; j < points.numPoints; j++) {
		fprintf(fout, " %d %d", points.x[j], points.y[j]);
	    }
	}
  }

  // specialWiring
  if (ppath->numWires()) {
     newLayer = 0;
     for (i = 0; i < ppath->numWires(); i++) {
        newLayer = 0;
        wire = ppath->wire(i);
        fprintf(fout, "\n  + %s ", wire->wireType());
        if (strcmp (wire->wireType(), "SHIELD") == 0)
           fprintf(fout, "%s ", wire->wireShieldNetName());
        for (j = 0; j < wire->numPaths(); j++) {
           p = wire->path(j);
           p->initTraverse();
           while ((path = (int)p->next()) != DEFIPATH_DONE) {
              count++;
              // Don't want the line to be too long
              if (count >= 5) {
                  fprintf(fout, "\n");
                  count = 0;
              }
              switch (path) {
                case DEFIPATH_LAYER:
                     if (newLayer == 0) {
                         fprintf(fout, "%s ", p->getLayer());
                         newLayer = 1;
                     } else
                         fprintf(fout, "NEW %s ", p->getLayer());
                     break;
                case DEFIPATH_VIA:
                     fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                     break;
                case DEFIPATH_VIAROTATION:
                     fprintf(fout, "%s ",
                             orientStr(p->getViaRotation()));
                     break;
                case DEFIPATH_VIADATA:
                     p->getViaData(&numX, &numY, &stepX, &stepY);
                     fprintf(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                             stepX, stepY);
                     break;
                case DEFIPATH_WIDTH:
                     fprintf(fout, "%d ", p->getWidth());
                     break;
		case DEFIPATH_MASK:
		     fprintf(fout, "MASK %d ", p->getMask());
		     break;
                case DEFIPATH_VIAMASK:
                     fprintf(fout, "MASK %d%d%d ", 
                             p->getViaTopMask(), 
                             p->getViaCutMask(),
                             p->getViaBottomMask());
                    break;
                case DEFIPATH_POINT:
                     p->getPoint(&x, &y);
                     fprintf(fout, "( %d %d ) ", x, y);
                     break;
                case DEFIPATH_FLUSHPOINT:
                     p->getFlushPoint(&x, &y, &z);
                     fprintf(fout, "( %d %d %d ) ", x, y, z);
                     break;
                case DEFIPATH_TAPER:
                     fprintf(fout, "TAPER ");
                     break;
                case DEFIPATH_SHAPE:
                     fprintf(fout, "+ SHAPE %s ", p->getShape());
                     break;
                case DEFIPATH_STYLE:
                     fprintf(fout, "+ STYLE %d ", p->getStyle());
                     break;
              }
           }
        }
        fprintf(fout, "\n");
        count = 0;
     }
  } else if (ppath->numShields()) {
    for (i = 0; i < ppath->numShields(); i++) {
       shield = ppath->shield(i);
       fprintf(fout, "\n  + SHIELD %s ", shield->shieldName());
       newLayer = 0;
       for (j = 0; j < shield->numPaths(); j++) {
          p = shield->path(j);
          p->initTraverse();
          while ((path = (int)p->next()) != DEFIPATH_DONE) {
             count++;
             // Don't want the line to be too long
             if (count >= 5) {
                 fprintf(fout, "\n");
                 count = 0;
             } 
             switch (path) {
               case DEFIPATH_LAYER:
                    if (newLayer == 0) {
                        fprintf(fout, "%s ", p->getLayer());
                        newLayer = 1;
                    } else
                        fprintf(fout, "NEW %s ", p->getLayer());
                    break;
               case DEFIPATH_VIA:
                    fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                    break;
               case DEFIPATH_VIAROTATION:
                    fprintf(fout, "%s ", 
                            orientStr(p->getViaRotation()));
                    break;
               case DEFIPATH_WIDTH:
                    fprintf(fout, "%d ", p->getWidth());
                    break;
	       case DEFIPATH_MASK:
		    fprintf(fout, "MASK %d ", p->getMask());
		    break;
               case DEFIPATH_VIAMASK:
                   fprintf(fout, "MASK %d%d%d ", 
                           p->getViaTopMask(), 
                           p->getViaCutMask(),
                           p->getViaBottomMask());
                   break;
               case DEFIPATH_POINT:
                    p->getPoint(&x, &y);
                    fprintf(fout, "( %d %d ) ", x, y);
                    break;
               case DEFIPATH_FLUSHPOINT:
                    p->getFlushPoint(&x, &y, &z);
                    fprintf(fout, "( %d %d %d ) ", x, y, z);
                    break;
               case DEFIPATH_TAPER:
                    fprintf(fout, "TAPER ");
                    break;
               case DEFIPATH_SHAPE:
                    fprintf(fout, "+ SHAPE %s ", p->getShape());
                    break;
               case DEFIPATH_STYLE:
                    fprintf(fout, "+ STYLE %d ", p->getStyle());
                    break;
             }
          }
       }
    } 
  }

  fprintf(fout, "\n");

  return 0;
}

int snetf(defrCallbackType_e c, defiNet* net, defiUserData ud) {
  // For net and special net.
  int         i, j, x, y, z, count, newLayer;
  char*       layerName;
  double      dist, left, right;
  defiPath*   p;
  defiSubnet  *s;
  int         path;
  defiShield* shield;
  defiWire*   wire;
  int         numX, numY, stepX, stepY;

  checkType(c);
  if (ud != userData) dataError();
  if (c != defrSNetCbkType)
      fprintf(fout, "BOGUS NET TYPE  ");

// 5/6/2004 - don't need since I have a callback for the name
//  fprintf(fout, "- %s ", net->name());

  count = 0;
  // compName & pinName
  for (i = 0; i < net->numConnections(); i++) {
      // set the limit of only 5 items print out in one line
      count++;
      if (count >= 5) {
          fprintf(fout, "\n");
          count = 0;
      }
      fprintf (fout, "( %s %s ) ", net->instance(i),
               net->pin(i));
      if (net->pinIsSynthesized(i))
          fprintf(fout, "+ SYNTHESIZED ");
  }

  // specialWiring
  if (net->numWires()) {
     newLayer = 0;
     for (i = 0; i < net->numWires(); i++) {
        newLayer = 0;
        wire = net->wire(i);
        fprintf(fout, "\n  + %s ", wire->wireType());
        if (strcmp (wire->wireType(), "SHIELD") == 0)
           fprintf(fout, "%s ", wire->wireShieldNetName());
        for (j = 0; j < wire->numPaths(); j++) {
            p = wire->path(j);
            p->initTraverse();
            if (testDebugPrint) {
                p->print(fout);
            } else {
                while ((path = (int)p->next()) != DEFIPATH_DONE) {
                  count++;
                  // Don't want the line to be too long
                  if (count >= 5) {
                      fprintf(fout, "\n");
                      count = 0;
                  }
                  switch (path) {
                    case DEFIPATH_LAYER:
                         if (newLayer == 0) {
                             fprintf(fout, "%s ", p->getLayer());
                             newLayer = 1;
                         } else
                             fprintf(fout, "NEW %s ", p->getLayer());
                         break;
                    case DEFIPATH_VIA:
                         fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                         break;
                    case DEFIPATH_VIAROTATION:
                         fprintf(fout, "%s ", 
                                 orientStr(p->getViaRotation()));
                         break;
                    case DEFIPATH_VIADATA:
                         p->getViaData(&numX, &numY, &stepX, &stepY);
                         fprintf(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                                 stepX, stepY);
                         break;
                    case DEFIPATH_WIDTH:
                         fprintf(fout, "%d ", p->getWidth());
                         break;
                    case DEFIPATH_MASK:
                         fprintf(fout, "MASK %d ", p->getMask());
                         break;
                    case DEFIPATH_VIAMASK:
                         fprintf(fout, "MASK %d%d%d ", 
                                 p->getViaTopMask(), 
                                 p->getViaCutMask(),
                                 p->getViaBottomMask());
                        break;
                    case DEFIPATH_POINT:
                         p->getPoint(&x, &y);
                         fprintf(fout, "( %d %d ) ", x, y);
                         break;
                    case DEFIPATH_FLUSHPOINT:
                         p->getFlushPoint(&x, &y, &z);
                         fprintf(fout, "( %d %d %d ) ", x, y, z);
                         break;
                    case DEFIPATH_TAPER:
                         fprintf(fout, "TAPER ");
                         break;
                    case DEFIPATH_SHAPE:
                         fprintf(fout, "+ SHAPE %s ", p->getShape());
                         break;
                    case DEFIPATH_STYLE:
                         fprintf(fout, "+ STYLE %d ", p->getStyle());
                         break;
                    }
                }
            }
        }
        fprintf(fout, "\n");
        count = 0;
     }
  }

  // POLYGON
  if (net->numPolygons()) {
    defiPoints points;

    for (i = 0; i < net->numPolygons(); i++) {
      if (curVer >= 5.8 ) {
	 if (strcmp(net->polyRouteStatus(i), "") != 0) {
	   fprintf(fout, "\n  + %s ", net->polyRouteStatus(i));
	   if (strcmp(net->polyRouteStatus(i), "SHIELD") == 0) {
	      fprintf(fout, "\n  + %s ", net->polyRouteStatusShieldName(i));
	   }
         }
         if (strcmp(net->polyShapeType(i), "") != 0) {
	   fprintf(fout, "\n  + SHAPE %s ", net->polyShapeType(i));
         }
      }
      if (net->polyMask(i)) {
	  fprintf(fout, "\n  + MASK %d + POLYGON % s ", net->polyMask(i),
		  net->polygonName(i));
      } else {
          fprintf(fout, "\n  + POLYGON %s ", net->polygonName(i));
      }
      points = net->getPolygon(i);
      for (j = 0; j < points.numPoints; j++)
        fprintf(fout, "%d %d ", points.x[j], points.y[j]);
    }
  }
  // RECT
  if (net->numRectangles()) {

     for (i = 0; i < net->numRectangles(); i++) {
       if (curVer >= 5.8 ) {
	 if (strcmp(net->rectRouteStatus(i), "") != 0) {
	   fprintf(fout, "\n  + %s ", net->rectRouteStatus(i));
	   if (strcmp(net->rectRouteStatus(i), "SHIELD") == 0) {
	      fprintf(fout, "\n  + %s ", net->rectRouteStatusShieldName(i));
	   }
         }
         if (strcmp(net->rectShapeType(i), "") != 0) {
	   fprintf(fout, "\n  + SHAPE %s ", net->rectShapeType(i));
         }
       }
       if (net->rectMask(i)) {
	  fprintf(fout, "\n  + MASK %d + RECT %s %d %d %d %d", 
		  net->rectMask(i), net->rectName(i),
		  net->xl(i), net->yl(i), net->xh(i),
                  net->yh(i));
       } else {
	    fprintf(fout, "\n  + RECT %s %d %d %d %d", 
		    net->rectName(i),
                    net->xl(i), 
		    net->yl(i), 
		    net->xh(i),
                    net->yh(i));
      }
     }
  }
  // VIA
  if (curVer >= 5.8 && net->numViaSpecs()) {
     for (i = 0; i < net->numViaSpecs(); i++) {
       if (strcmp(net->viaRouteStatus(i), "") != 0) {
	fprintf(fout, "\n  + %s ", net->viaRouteStatus(i));
	if (strcmp(net->viaRouteStatus(i), "SHIELD") == 0) {
	      fprintf(fout, "\n  + %s ", net->viaRouteStatusShieldName(i));
	   }
       }
       if (strcmp(net->viaShapeType(i), "") != 0) {
	fprintf(fout, "\n  + SHAPE %s ", net->viaShapeType(i));
       }
       if (net->topMaskNum(i) || net->cutMaskNum(i) || net->bottomMaskNum(i)) {
	fprintf(fout, "\n  + MASK %d%d%d + VIA %s ", net->topMaskNum(i), 
                net->cutMaskNum(i),
                net->bottomMaskNum(i),
		net->viaName(i));
       } else {
	fprintf(fout, "\n  + VIA %s ", net->viaName(i));
       }
       fprintf(fout, " %s", net->viaOrientStr(i));
      
       defiPoints points = net->getViaPts(i);

       for (int j = 0; j < points.numPoints; j++) {
          fprintf(fout, " %d %d", points.x[j], points.y[j]);
       }
       fprintf(fout, ";\n"); 

     }
  }

  if (net->hasSubnets()) {
    for (i = 0; i < net->numSubnets(); i++) {
      s = net->subnet(i);
      if (s->numConnections()) {
          if (s->pinIsMustJoin(0))
              fprintf(fout, "- MUSTJOIN ");
          else
              fprintf(fout, "- %s ", s->name());
          for (j = 0; j < s->numConnections(); j++) {
              fprintf(fout, " ( %s %s )\n", s->instance(j),
                      s->pin(j));
        }
      }
 
      // regularWiring
      if (s->numWires()) {
         for (i = 0; i < s->numWires(); i++) {
            wire = s->wire(i);
            fprintf(fout, "  + %s ", wire->wireType());
            for (j = 0; j < wire->numPaths(); j++) {
              p = wire->path(j);
              p->print(fout);
            }
         }
      }
    }
  }

  if (net->numProps()) {
    for (i = 0; i < net->numProps(); i++) {
        if (net->propIsString(i))
           fprintf(fout, "  + PROPERTY %s %s ", net->propName(i),
                   net->propValue(i));
        if (net->propIsNumber(i))
           fprintf(fout, "  + PROPERTY %s %g ", net->propName(i),
                   net->propNumber(i));
        switch (net->propType(i)) {
           case 'R': fprintf(fout, "REAL ");
                     break;
           case 'I': fprintf(fout, "INTEGER ");
                     break;
           case 'S': fprintf(fout, "STRING ");
                     break;
           case 'Q': fprintf(fout, "QUOTESTRING ");
                     break;
           case 'N': fprintf(fout, "NUMBER ");
                     break;
        }
        fprintf(fout, "\n");
    }
  }

  // SHIELD
  count = 0;
  // testing the SHIELD for 5.3, obsolete in 5.4
  if (net->numShields()) {
    for (i = 0; i < net->numShields(); i++) {
       shield = net->shield(i);
       fprintf(fout, "\n  + SHIELD %s ", shield->shieldName());
       newLayer = 0;
       for (j = 0; j < shield->numPaths(); j++) {
          p = shield->path(j);
          p->initTraverse();
          while ((path = (int)p->next()) != DEFIPATH_DONE) {
             count++;
             // Don't want the line to be too long
             if (count >= 5) {
                 fprintf(fout, "\n");
                 count = 0;
             } 
             switch (path) {
               case DEFIPATH_LAYER:
                    if (newLayer == 0) {
                        fprintf(fout, "%s ", p->getLayer());
                        newLayer = 1;
                    } else
                        fprintf(fout, "NEW %s ", p->getLayer());
                    break;
               case DEFIPATH_VIA:
                    fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                    break;
               case DEFIPATH_VIAROTATION:
                    fprintf(fout, "%s ", 
                            orientStr(p->getViaRotation()));
                    break;
               case DEFIPATH_WIDTH:
                    fprintf(fout, "%d ", p->getWidth());
                    break;
	       case DEFIPATH_MASK:
		    fprintf(fout, "MASK %d ", p->getMask());
		    break;
               case DEFIPATH_VIAMASK:
                    fprintf(fout, "MASK %d%d%d ", 
                            p->getViaTopMask(), 
                            p->getViaCutMask(),
                            p->getViaBottomMask());
                   break;
               case DEFIPATH_POINT:
                    p->getPoint(&x, &y);
                    fprintf(fout, "( %d %d ) ", x, y);
                    break;
               case DEFIPATH_FLUSHPOINT:
                    p->getFlushPoint(&x, &y, &z);
                    fprintf(fout, "( %d %d %d ) ", x, y, z);
                    break;
               case DEFIPATH_TAPER:
                    fprintf(fout, "TAPER ");
                    break;
               case DEFIPATH_SHAPE:
                    fprintf(fout, "+ SHAPE %s ", p->getShape());
                    break;
               case DEFIPATH_STYLE:
                    fprintf(fout, "+ STYLE %d ", p->getStyle());
                    break;
             }
          }
       }
    }
  }

  // layerName width
  if (net->hasWidthRules()) {
    for (i = 0; i < net->numWidthRules(); i++) {
        net->widthRule(i, &layerName, &dist);
        fprintf (fout, "\n  + WIDTH %s %g ", layerName, dist);
    }
  }

  // layerName spacing
  if (net->hasSpacingRules()) {
    for (i = 0; i < net->numSpacingRules(); i++) {
        net->spacingRule(i, &layerName, &dist, &left, &right);
        if (left == right)
            fprintf (fout, "\n  + SPACING %s %g ", layerName, dist);
        else
            fprintf (fout, "\n  + SPACING %s %g RANGE %g %g ",
                     layerName, dist, left, right);
    }
  }

  if (net->hasFixedbump())
    fprintf(fout, "\n  + FIXEDBUMP ");
  if (net->hasFrequency())
    fprintf(fout, "\n  + FREQUENCY %g ", net->frequency());
  if (net->hasVoltage())
    fprintf(fout, "\n  + VOLTAGE %g ", net->voltage());
  if (net->hasWeight())
    fprintf(fout, "\n  + WEIGHT %d ", net->weight());
  if (net->hasCap())
    fprintf(fout, "\n  + ESTCAP %g ", net->cap());
  if (net->hasSource())
    fprintf(fout, "\n  + SOURCE %s ", net->source());
  if (net->hasPattern())
    fprintf(fout, "\n  + PATTERN %s ", net->pattern());
  if (net->hasOriginal())
    fprintf(fout, "\n  + ORIGINAL %s ", net->original());
  if (net->hasUse())
    fprintf(fout, "\n  + USE %s ", net->use());

  fprintf (fout, ";\n");
  --numObjs;
  if (numObjs <= 0)
      fprintf(fout, "END SPECIALNETS\n");
  return 0;
}


int ndr(defrCallbackType_e c, defiNonDefault* nd, defiUserData ud) {
  // For nondefaultrule
  int i;

  checkType(c);
  if (ud != userData) dataError();
  if (c != defrNonDefaultCbkType)
      fprintf(fout, "BOGUS NONDEFAULTRULE TYPE  ");
  fprintf(fout, "- %s\n", nd->name());
  if (nd->hasHardspacing())
      fprintf(fout, "   + HARDSPACING\n");
  for (i = 0; i < nd->numLayers(); i++) {
    fprintf(fout, "   + LAYER %s", nd->layerName(i));
    fprintf(fout, " WIDTH %d", nd->layerWidthVal(i));
    if (nd->hasLayerDiagWidth(i)) 
      fprintf(fout, " DIAGWIDTH %d",
              nd->layerDiagWidthVal(i));
    if (nd->hasLayerSpacing(i)) 
      fprintf(fout, " SPACING %d", nd->layerSpacingVal(i));
    if (nd->hasLayerWireExt(i)) 
      fprintf(fout, " WIREEXT %d", nd->layerWireExtVal(i));
    fprintf(fout, "\n");
  }
  for (i = 0; i < nd->numVias(); i++)
    fprintf(fout, "   + VIA %s\n", nd->viaName(i));
  for (i = 0; i < nd->numViaRules(); i++)
    fprintf(fout, "   + VIARULE %s\n", ignoreViaNames ? "XXX" : nd->viaRuleName(i));
  for (i = 0; i < nd->numMinCuts(); i++)
    fprintf(fout, "   + MINCUTS %s %d\n", nd->cutLayerName(i),
            nd->numCuts(i));
  for (i = 0; i < nd->numProps(); i++) {
    fprintf(fout, "   + PROPERTY %s %s ", nd->propName(i),
            nd->propValue(i));
    switch (nd->propType(i)) {
      case 'R': fprintf(fout, "REAL\n");
                break;
      case 'I': fprintf(fout, "INTEGER\n");
                break;
      case 'S': fprintf(fout, "STRING\n");
                break;
      case 'Q': fprintf(fout, "QUOTESTRING\n");
                break;
      case 'N': fprintf(fout, "NUMBER\n");
                break;
    }
  }
  --numObjs;
  if (numObjs <= 0)
    fprintf(fout, "END NONDEFAULTRULES\n");
  return 0;
}

int tname(defrCallbackType_e c, const char* string, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
  fprintf(fout, "TECHNOLOGY %s ;\n", string);
  return 0;
}

int dname(defrCallbackType_e c, const char* string, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
  fprintf(fout, "DESIGN %s ;\n", string);

  return 0;
}


char* address(const char* in) {
  return ((char*)in);
}

int cs(defrCallbackType_e c, int num, defiUserData ud) {
  char* name;

  checkType(c);

  if (ud != userData) dataError();

  switch (c) {
  case defrComponentStartCbkType : name = address("COMPONENTS"); break;
  case defrNetStartCbkType : name = address("NETS"); break;
  case defrStartPinsCbkType : name = address("PINS"); break;
  case defrViaStartCbkType : name = address("VIAS"); break;
  case defrRegionStartCbkType : name = address("REGIONS"); break;
  case defrSNetStartCbkType : name = address("SPECIALNETS"); break;
  case defrGroupsStartCbkType : name = address("GROUPS"); break;
  case defrScanchainsStartCbkType : name = address("SCANCHAINS"); break;
  case defrIOTimingsStartCbkType : name = address("IOTIMINGS"); break;
  case defrFPCStartCbkType : name = address("FLOORPLANCONSTRAINTS"); break;
  case defrTimingDisablesStartCbkType : name = address("TIMING DISABLES"); break;
  case defrPartitionsStartCbkType : name = address("PARTITIONS"); break;
  case defrPinPropStartCbkType : name = address("PINPROPERTIES"); break;
  case defrBlockageStartCbkType : name = address("BLOCKAGES"); break;
  case defrSlotStartCbkType : name = address("SLOTS"); break;
  case defrFillStartCbkType : name = address("FILLS"); break;
  case defrNonDefaultStartCbkType : name = address("NONDEFAULTRULES"); break;
  case defrStylesStartCbkType : name = address("STYLES"); break;
  default : name = address("BOGUS"); return 1;
  }
  fprintf(fout, "\n%s %d ;\n", name, num);
  numObjs = num;
  return 0;
}

int constraintst(defrCallbackType_e c, int num, defiUserData ud) {
  // Handles both constraints and assertions
  checkType(c);
  if (ud != userData) dataError();
  if (c == defrConstraintsStartCbkType)
      fprintf(fout, "\nCONSTRAINTS %d ;\n\n", num);
  else
      fprintf(fout, "\nASSERTIONS %d ;\n\n", num);
  numObjs = num;
  return 0;
}

void operand(defrCallbackType_e c, defiAssertion* a, int ind) {
  int i, first = 1;
  char* netName;
  char* fromInst, * fromPin, * toInst, * toPin;

  if (a->isSum()) {
      // Sum in operand, recursively call operand
      fprintf(fout, "- SUM ( ");
      a->unsetSum();
      isSumSet = 1;
      begOperand = 0;
      operand (c, a, ind);
      fprintf(fout, ") ");
  } else {
      // operand
      if (ind >= a->numItems()) {
          fprintf(fout, "ERROR: when writing out SUM in Constraints.\n");
          return;
       }
      if (begOperand) {
         fprintf(fout, "- ");
         begOperand = 0;
      }
      for (i = ind; i < a->numItems(); i++) {
          if (a->isNet(i)) {
              a->net(i, &netName);
              if (!first)
                  fprintf(fout, ", "); // print , as separator
              fprintf(fout, "NET %s ", netName); 
          } else if (a->isPath(i)) {
              a->path(i, &fromInst, &fromPin, &toInst,
                                     &toPin);
              if (!first)
                  fprintf(fout, ", ");
              fprintf(fout, "PATH %s %s %s %s ", fromInst, fromPin, toInst,
                      toPin);
          } else if (isSumSet) {
              // SUM within SUM, reset the flag
              a->setSum();
              operand(c, a, i);
          }
          first = 0;
      } 
      
  }
}

int constraint(defrCallbackType_e c, defiAssertion* a, defiUserData ud) {
  // Handles both constraints and assertions

  checkType(c);
  if (ud != userData) dataError();
  if (a->isWiredlogic())
      // Wirelogic
      fprintf(fout, "- WIREDLOGIC %s + MAXDIST %g ;\n",
// Wiredlogic dist is also store in fallMax
//              a->netName(), a->distance());
              a->netName(), a->fallMax());
  else {
      // Call the operand function
      isSumSet = 0;    // reset the global variable
      begOperand = 1;
      operand (c, a, 0);
      // Get the Rise and Fall
      if (a->hasRiseMax())
          fprintf(fout, "+ RISEMAX %g ", a->riseMax());
      if (a->hasFallMax())
          fprintf(fout, "+ FALLMAX %g ", a->fallMax());
      if (a->hasRiseMin())
          fprintf(fout, "+ RISEMIN %g ", a->riseMin());
      if (a->hasFallMin())
          fprintf(fout, "+ FALLMIN %g ", a->fallMin());
      fprintf(fout, ";\n");
  }
  --numObjs;
  if (numObjs <= 0) {
      if (c == defrConstraintCbkType)
          fprintf(fout, "END CONSTRAINTS\n");
      else 
          fprintf(fout, "END ASSERTIONS\n");
  }
  return 0;
}


int propstart(defrCallbackType_e c, void*, defiUserData) {
  checkType(c);
  fprintf(fout, "\nPROPERTYDEFINITIONS\n");
  isProp = 1;

  return 0;
}


int prop(defrCallbackType_e c, defiProp* p, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
  if (strcmp(p->propType(), "design") == 0)
      fprintf(fout, "DESIGN %s ", p->propName());
  else if (strcmp(p->propType(), "net") == 0)
      fprintf(fout, "NET %s ", p->propName());
  else if (strcmp(p->propType(), "component") == 0)
      fprintf(fout, "COMPONENT %s ", p->propName());
  else if (strcmp(p->propType(), "specialnet") == 0)
      fprintf(fout, "SPECIALNET %s ", p->propName());
  else if (strcmp(p->propType(), "group") == 0)
      fprintf(fout, "GROUP %s ", p->propName());
  else if (strcmp(p->propType(), "row") == 0)
      fprintf(fout, "ROW %s ", p->propName());
  else if (strcmp(p->propType(), "componentpin") == 0)
      fprintf(fout, "COMPONENTPIN %s ", p->propName());
  else if (strcmp(p->propType(), "region") == 0)
      fprintf(fout, "REGION %s ", p->propName());
  else if (strcmp(p->propType(), "nondefaultrule") == 0)
      fprintf(fout, "NONDEFAULTRULE %s ", p->propName());
  if (p->dataType() == 'I')
      fprintf(fout, "INTEGER ");
  if (p->dataType() == 'R')
      fprintf(fout, "REAL ");
  if (p->dataType() == 'S')
      fprintf(fout, "STRING ");
  if (p->dataType() == 'Q')
      fprintf(fout, "STRING ");
  if (p->hasRange()) {
      fprintf(fout, "RANGE %g %g ", p->left(),
              p->right());
  }
  if (p->hasNumber())
      fprintf(fout, "%g ", p->number());
  if (p->hasString())
      fprintf(fout, "\"%s\" ", p->string());
  fprintf(fout, ";\n");

  return 0;
}


int propend(defrCallbackType_e c, void*, defiUserData) {
  checkType(c);
  if (isProp) {
      fprintf(fout, "END PROPERTYDEFINITIONS\n\n");
      isProp = 0;
  }

  return 0;
}


int hist(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  defrSetCaseSensitivity(0);
  if (ud != userData) dataError();
  fprintf(fout, "HISTORY %s ;\n", h);
  defrSetCaseSensitivity(1);
  return 0;
}


int an(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
  fprintf(fout, "ARRAY %s ;\n", h);
  return 0;
}


int fn(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
  fprintf(fout, "FLOORPLAN %s ;\n", h);
  return 0;
}


int bbn(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
  fprintf(fout, "BUSBITCHARS \"%s\" ;\n", h);
  return 0;
}


int vers(defrCallbackType_e c, double d, defiUserData ud) {
  checkType(c);
  if (ud != userData) 
      dataError();
  fprintf(fout, "VERSION %g ;\n", d);  
  curVer = d;

  fprintf(fout, "ALIAS alias1 aliasValue1 1 ;\n");
  fprintf(fout, "ALIAS alias2 aliasValue2 0 ;\n");

  return 0;
}


int versStr(defrCallbackType_e c, const char* versionName, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
  fprintf(fout, "VERSION %s ;\n", versionName);
  return 0;
}


int units(defrCallbackType_e c, double d, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
  fprintf(fout, "UNITS DISTANCE MICRONS %g ;\n", d);
  return 0;
}


int casesens(defrCallbackType_e c, int d, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
  if (d == 1)
     fprintf(fout, "NAMESCASESENSITIVE ON ;\n", d);
  else
     fprintf(fout, "NAMESCASESENSITIVE OFF ;\n", d);
  return 0;
}


int cls(defrCallbackType_e c, void* cl, defiUserData ud) {
  defiSite* site;  // Site and Canplace and CannotOccupy
  defiBox* box;  // DieArea and 
  defiPinCap* pc;
  defiPin* pin;
  int i, j, k;
  defiRow* row;
  defiTrack* track;
  defiGcellGrid* gcg;
  defiVia* via;
  defiRegion* re;
  defiGroup* group;
  defiComponentMaskShiftLayer* maskShiftLayer = NULL;
  defiScanchain* sc;
  defiIOTiming* iot;
  defiFPC* fpc;
  defiTimingDisable* td;
  defiPartition* part;
  defiPinProp* pprop;
  defiBlockage* block;
  defiSlot* slots;
  defiFill* fills;
  defiStyles* styles;
  int xl, yl, xh, yh;
  char *name, *a1, *b1;
  char **inst, **inPin, **outPin;
  int  *bits;
  int  size;
  int corner, typ;
  const char *itemT;
  char dir;
  defiPinAntennaModel* aModel;
  defiPoints points;

  checkType(c);
  if (ud != userData) dataError();
  switch (c) {

  case defrSiteCbkType :
         site = (defiSite*)cl;
         fprintf(fout, "SITE %s %g %g %s ", site->name(),
                 site->x_orig(), site->y_orig(),
                 orientStr(site->orient()));
         fprintf(fout, "DO %g BY %g STEP %g %g ;\n",
                 site->x_num(), site->y_num(),
                 site->x_step(), site->y_step());
         break;
  case defrCanplaceCbkType :
         site = (defiSite*)cl;
         fprintf(fout, "CANPLACE %s %g %g %s ", site->name(),
                 site->x_orig(), site->y_orig(),
                 orientStr(site->orient()));
         fprintf(fout, "DO %g BY %g STEP %g %g ;\n",
                 site->x_num(), site->y_num(),
                 site->x_step(), site->y_step());
         break;
  case defrCannotOccupyCbkType : 
         site = (defiSite*)cl;
         fprintf(fout, "CANNOTOCCUPY %s %g %g %s ",
                 site->name(), site->x_orig(),
                 site->y_orig(), orientStr(site->orient()));
         fprintf(fout, "DO %g BY %g STEP %g %g ;\n",
                 site->x_num(), site->y_num(),
                 site->x_step(), site->y_step());
         break;
  case defrDieAreaCbkType :
         box = (defiBox*)cl;
         fprintf(fout, "DIEAREA %d %d %d %d ;\n",
                 box->xl(), box->yl(), box->xh(),
                 box->yh());
         fprintf(fout, "DIEAREA ");
         points = box->getPoint();
         for (i = 0; i < points.numPoints; i++)
           fprintf(fout, "%d %d ", points.x[i], points.y[i]);
         fprintf(fout, ";\n");
         break;
  case defrPinCapCbkType :
         pc = (defiPinCap*)cl;
         if (testDebugPrint) {
             pc->print(fout);
         } else {
             fprintf(fout, "MINPINS %d WIRECAP %g ;\n",
                     pc->pin(), pc->cap());
             --numObjs;
             if (numObjs <= 0)
                 fprintf(fout, "END DEFAULTCAP\n");
         }
         break;
  case defrPinCbkType :
         pin = (defiPin*)cl;
         if (testDebugPrint) {
             pin->print(fout);
         } else {
             fprintf(fout, "- %s + NET %s ", pin->pinName(),
                     pin->netName());
    //         pin->changePinName("pinName");
    //         fprintf(fout, "%s ", pin->pinName());
             if (pin->hasDirection())
                 fprintf(fout, "+ DIRECTION %s ", pin->direction());
             if (pin->hasUse())
                 fprintf(fout, "+ USE %s ", pin->use());
             if (pin->hasNetExpr())
                 fprintf(fout, "+ NETEXPR \"%s\" ", pin->netExpr());
             if (pin->hasSupplySensitivity())
                 fprintf(fout, "+ SUPPLYSENSITIVITY %s ",
                         pin->supplySensitivity());
             if (pin->hasGroundSensitivity())
                 fprintf(fout, "+ GROUNDSENSITIVITY %s ",
                         pin->groundSensitivity());
             if (pin->hasLayer()) {
                 defiPoints points;
                 for (i = 0; i < pin->numLayer(); i++) {
                    fprintf(fout, "\n  + LAYER %s ", pin->layer(i));
                    if (pin->layerMask(i)) 
                        fprintf(fout, "MASK %d ",
                        pin->layerMask(i));
                    if (pin->hasLayerSpacing(i))
                      fprintf(fout, "SPACING %d ",
                             pin->layerSpacing(i));
                    if (pin->hasLayerDesignRuleWidth(i))
                      fprintf(fout, "DESIGNRULEWIDTH %d ",
                             pin->layerDesignRuleWidth(i));
                    pin->bounds(i, &xl, &yl, &xh, &yh);
                    fprintf(fout, "%d %d %d %d ", xl, yl, xh, yh);
                 }
                 for (i = 0; i < pin->numPolygons(); i++) {
                    fprintf(fout, "\n  + POLYGON %s ",
                            pin->polygonName(i));
                    if (pin->polygonMask(i))
                      fprintf(fout, "MASK %d ",
                              pin->polygonMask(i));
                    if (pin->hasPolygonSpacing(i))
                      fprintf(fout, "SPACING %d ",
                             pin->polygonSpacing(i));
                    if (pin->hasPolygonDesignRuleWidth(i))
                      fprintf(fout, "DESIGNRULEWIDTH %d ",
                             pin->polygonDesignRuleWidth(i));
                    points = pin->getPolygon(i);
                    for (j = 0; j < points.numPoints; j++)
                      fprintf(fout, "%d %d ", points.x[j], points.y[j]);
                 }
                 for (i = 0; i < pin->numVias(); i++) {
                     if (pin->viaTopMask(i) || pin->viaCutMask(i) || pin->viaBottomMask(i)) {
                         fprintf(fout, "\n  + VIA %s MASK %d%d%d %d %d ", 
                             pin->viaName(i),
                             pin->viaTopMask(i),
                             pin->viaCutMask(i),
                             pin->viaBottomMask(i),
                             pin->viaPtX(i), 
                             pin->viaPtY(i));
                     } else {
                         fprintf(fout, "\n  + VIA %s %d %d ", pin->viaName(i),
                                 pin->viaPtX(i), pin->viaPtY(i));
                     }
                 }
             }
             if (pin->hasPort()) {
                 defiPoints points;
                 defiPinPort* port;
                 for (j = 0; j < pin->numPorts(); j++) {
                    port = pin->pinPort(j);
                    fprintf(fout, "\n  + PORT");
                    for (i = 0; i < port->numLayer(); i++) {
                       fprintf(fout, "\n     + LAYER %s ",
                               port->layer(i));
                       if (port->layerMask(i))
                           fprintf(fout, "MASK %d ",
                                   port->layerMask(i));
                       if (port->hasLayerSpacing(i))
                         fprintf(fout, "SPACING %d ",
                                 port->layerSpacing(i));
                       if (port->hasLayerDesignRuleWidth(i))
                         fprintf(fout, "DESIGNRULEWIDTH %d ",
                                 port->layerDesignRuleWidth(i));
                       port->bounds(i, &xl, &yl, &xh, &yh);
                       fprintf(fout, "%d %d %d %d ", xl, yl, xh, yh);
                    }
                    for (i = 0; i < port->numPolygons(); i++) {
                       fprintf(fout, "\n     + POLYGON %s ",
                               port->polygonName(i));
                       if (port->polygonMask(i))
                         fprintf(fout, "MASK %d ",
                                 port->polygonMask(i));
                       if (port->hasPolygonSpacing(i))
                         fprintf(fout, "SPACING %d ",
                                port->polygonSpacing(i));
                       if (port->hasPolygonDesignRuleWidth(i))
                         fprintf(fout, "DESIGNRULEWIDTH %d ",
                                port->polygonDesignRuleWidth(i));
                       points = port->getPolygon(i);
                       for (k = 0; k < points.numPoints; k++)
                         fprintf(fout, "( %d %d ) ", points.x[k], points.y[k]);
                    }
                    for (i = 0; i < port->numVias(); i++) {
                        if (port->viaTopMask(i) || port->viaCutMask(i) 
                            || port->viaBottomMask(i)) {
                            fprintf(fout, "\n     + VIA %s MASK %d%d%d ( %d %d ) ",
                                port->viaName(i),
                                port->viaTopMask(i),
                                port->viaCutMask(i),
                                port->viaBottomMask(i),
                                port->viaPtX(i),
                                port->viaPtY(i));
                        } else {
                            fprintf(fout, "\n     + VIA %s ( %d %d ) ",
                               port->viaName(i),
                               port->viaPtX(i),
                               port->viaPtY(i));
                        }
                    }
                    if (port->hasPlacement()) {
                       if (port->isPlaced()) {
                          fprintf(fout, "\n     + PLACED ");
                          fprintf(fout, "( %d %d ) %s ",
                             port->placementX(),
                             port->placementY(),
                             orientStr(port->orient()));
                       }
                       if (port->isCover()) {
                          fprintf(fout, "\n     + COVER ");
                          fprintf(fout, "( %d %d ) %s ",
                             port->placementX(),
                             port->placementY(),
                             orientStr(port->orient()));
                       }
                       if (port->isFixed()) {
                          fprintf(fout, "\n     + FIXED ");
                          fprintf(fout, "( %d %d ) %s ",
                             port->placementX(),
                             port->placementY(),
                             orientStr(port->orient()));
                       }
                    }
                }
             }
             if (pin->hasPlacement()) {
                 if (pin->isPlaced()) {
                     fprintf(fout, "+ PLACED ");
                     fprintf(fout, "( %d %d ) %s ", pin->placementX(),
                         pin->placementY(), 
                         orientStr(pin->orient()));
                }
                 if (pin->isCover()) {
                     fprintf(fout, "+ COVER ");
                     fprintf(fout, "( %d %d ) %s ", pin->placementX(),
                         pin->placementY(), 
                         orientStr(pin->orient()));
                 }
                 if (pin->isFixed()) {
                     fprintf(fout, "+ FIXED ");
                     fprintf(fout, "( %d %d ) %s ", pin->placementX(),
                         pin->placementY(), 
                         orientStr(pin->orient()));
                 }
                 if (pin->isUnplaced())
                     fprintf(fout, "+ UNPLACED ");
             }
             if (pin->hasSpecial()) {
                 fprintf(fout, "+ SPECIAL ");
             }
             if (pin->hasAPinPartialMetalArea()) {
                 for (i = 0; i < pin->numAPinPartialMetalArea(); i++) {
                    fprintf(fout, "ANTENNAPINPARTIALMETALAREA %d",
                            pin->APinPartialMetalArea(i));
                    if (*(pin->APinPartialMetalAreaLayer(i)))
                        fprintf(fout, " LAYER %s",
                                pin->APinPartialMetalAreaLayer(i));
                    fprintf(fout, "\n");
                 }
             }
             if (pin->hasAPinPartialMetalSideArea()) {
                 for (i = 0; i < pin->numAPinPartialMetalSideArea(); i++) {
                    fprintf(fout, "ANTENNAPINPARTIALMETALSIDEAREA %d",
                            pin->APinPartialMetalSideArea(i));
                    if (*(pin->APinPartialMetalSideAreaLayer(i)))
                        fprintf(fout, " LAYER %s",
                            pin->APinPartialMetalSideAreaLayer(i));
                    fprintf(fout, "\n");
                 }
             }
             if (pin->hasAPinDiffArea()) {
                 for (i = 0; i < pin->numAPinDiffArea(); i++) {
                    fprintf(fout, "ANTENNAPINDIFFAREA %d", pin->APinDiffArea(i));
                    if (*(pin->APinDiffAreaLayer(i)))
                        fprintf(fout, " LAYER %s", pin->APinDiffAreaLayer(i));
                    fprintf(fout, "\n");
                 }
             }
             if (pin->hasAPinPartialCutArea()) {
                 for (i = 0; i < pin->numAPinPartialCutArea(); i++) {
                    fprintf(fout, "ANTENNAPINPARTIALCUTAREA %d",
                            pin->APinPartialCutArea(i));
                    if (*(pin->APinPartialCutAreaLayer(i)))
                        fprintf(fout, " LAYER %s", pin->APinPartialCutAreaLayer(i));
                    fprintf(fout, "\n");
                 }
             }

             for (j = 0; j < pin->numAntennaModel(); j++) {
                aModel = pin->antennaModel(j);
 
                fprintf(fout, "ANTENNAMODEL %s\n",
                        aModel->antennaOxide()); 
 
                if (aModel->hasAPinGateArea()) {
                    for (i = 0; i < aModel->numAPinGateArea();
                       i++) {
                       fprintf(fout, "ANTENNAPINGATEAREA %d",
                               aModel->APinGateArea(i));
                       if (aModel->hasAPinGateAreaLayer(i))
                           fprintf(fout, " LAYER %s", aModel->APinGateAreaLayer(i));
                       fprintf(fout, "\n");
                    }
                }
                if (aModel->hasAPinMaxAreaCar()) {
                    for (i = 0;
                       i < aModel->numAPinMaxAreaCar(); i++) {
                       fprintf(fout, "ANTENNAPINMAXAREACAR %d",
                               aModel->APinMaxAreaCar(i));
                       if (aModel->hasAPinMaxAreaCarLayer(i))
                           fprintf(fout,
                               " LAYER %s", aModel->APinMaxAreaCarLayer(i));
                       fprintf(fout, "\n");
                    }
                }
                if (aModel->hasAPinMaxSideAreaCar()) {
                    for (i = 0;
                         i < aModel->numAPinMaxSideAreaCar(); 
                         i++) {
                       fprintf(fout, "ANTENNAPINMAXSIDEAREACAR %d",
                               aModel->APinMaxSideAreaCar(i));
                       if (aModel->hasAPinMaxSideAreaCarLayer(i))
                           fprintf(fout,
                               " LAYER %s", aModel->APinMaxSideAreaCarLayer(i));
                       fprintf(fout, "\n");
                    }
                }
                if (aModel->hasAPinMaxCutCar()) {
                    for (i = 0; i < aModel->numAPinMaxCutCar();
                       i++) {
                       fprintf(fout, "ANTENNAPINMAXCUTCAR %d",
                           aModel->APinMaxCutCar(i));
                       if (aModel->hasAPinMaxCutCarLayer(i))
                           fprintf(fout, " LAYER %s",
                           aModel->APinMaxCutCarLayer(i));
                       fprintf(fout, "\n");
                    }
                }
             }
             fprintf(fout, ";\n");
             --numObjs;
             if (numObjs <= 0)
                 fprintf(fout, "END PINS\n");
         }
         break;
  case defrDefaultCapCbkType :
         i = (long)cl;
         fprintf(fout, "DEFAULTCAP %d\n", i);
         numObjs = i;
         break;
  case defrRowCbkType :
         row = (defiRow*)cl;
         fprintf(fout, "ROW %s %s %g %g %s ", ignoreRowNames ? "XXX" : row->name(),
                 row->macro(), row->x(), row->y(),
                 orientStr(row->orient()));
         if (row->hasDo()) {
             fprintf(fout, "DO %g BY %g ",
                     row->xNum(), row->yNum());
             if (row->hasDoStep())
                 fprintf(fout, "STEP %g %g ;\n",
                         row->xStep(), row->yStep());
             else
                 fprintf(fout, ";\n");
         } else
            fprintf(fout, ";\n");
         if (row->numProps() > 0) {
            for (i = 0; i < row->numProps(); i++) {
                fprintf(fout, "  + PROPERTY %s %s ",
                        row->propName(i),
                        row->propValue(i));
                switch (row->propType(i)) {
                   case 'R': fprintf(fout, "REAL ");
                             break;
                   case 'I': fprintf(fout, "INTEGER ");
                             break;
                   case 'S': fprintf(fout, "STRING ");
                             break;
                   case 'Q': fprintf(fout, "QUOTESTRING ");
                             break;
                   case 'N': fprintf(fout, "NUMBER ");
                             break;
                }
            }
            fprintf(fout, ";\n");
         }
         break;
  case defrTrackCbkType :
         track = (defiTrack*)cl;
	 if (track->firstTrackMask()) {
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
	 }
         for (i = 0; i < track->numLayers(); i++)
            fprintf(fout, "%s ", track->layer(i));
         fprintf(fout, ";\n"); 
         break;
  case defrGcellGridCbkType :
         gcg = (defiGcellGrid*)cl;
         fprintf(fout, "GCELLGRID %s %d DO %d STEP %g ;\n",
                 gcg->macro(), gcg->x(),
                 gcg->xNum(), gcg->xStep());
         break;
  case defrViaCbkType :
         via = (defiVia*)cl;
         if (testDebugPrint) {
             via->print(fout);
         } else {
             fprintf(fout, "- %s ", via->name());
             if (via->hasPattern())
                 fprintf(fout, "+ PATTERNNAME %s ", via->pattern());
             for (i = 0; i < via->numLayers(); i++) {
                 via->layer(i, &name, &xl, &yl, &xh, &yh);
                 int rectMask = via->rectMask(i);

                 if (rectMask) {
                     fprintf(fout, "+ RECT %s + MASK %d %d %d %d %d \n",
                             name, rectMask, xl, yl, xh, yh);
                 } else {
                     fprintf(fout, "+ RECT %s %d %d %d %d \n",
                             name, xl, yl, xh, yh);
                 }
             }
             // POLYGON
             if (via->numPolygons()) {
                 defiPoints points;
                 for (i = 0; i < via->numPolygons(); i++) {
                     int polyMask = via->polyMask(i);

                     if (polyMask) {
                         fprintf(fout, "\n  + POLYGON %s + MASK %d ", 
                                 via->polygonName(i), polyMask);
                     } else {
                         fprintf(fout, "\n  + POLYGON %s ", via->polygonName(i));
                     }
                     points = via->getPolygon(i);
                     for (j = 0; j < points.numPoints; j++)
                         fprintf(fout, "%d %d ", points.x[j], points.y[j]);
                 }
             }
             fprintf(fout, " ;\n");
             if (via->hasViaRule()) {
                 char *vrn, *bl, *cl, *tl;
                 int xs, ys, xcs, ycs, xbe, ybe, xte, yte;
                 int cr, cc, xo, yo, xbo, ybo, xto, yto;
                 (void)via->viaRule(&vrn, &xs, &ys, &bl, &cl, &tl, &xcs,
                                             &ycs, &xbe, &ybe, &xte, &yte);
                 fprintf(fout, "+ VIARULE '%s'\n", ignoreViaNames ? "XXX" : vrn);
                 fprintf(fout, "  + CUTSIZE %d %d\n", xs, ys);
                 fprintf(fout, "  + LAYERS %s %s %s\n", bl, cl, tl);
                 fprintf(fout, "  + CUTSPACING %d %d\n", xcs, ycs);
                 fprintf(fout, "  + ENCLOSURE %d %d %d %d\n", xbe, ybe, xte, yte);
                 if (via->hasRowCol()) {
                    (void)via->rowCol(&cr, &cc);
                    fprintf(fout, "  + ROWCOL %d %d\n", cr, cc);
                 }
                 if (via->hasOrigin()) {
                    (void)via->origin(&xo, &yo);
                    fprintf(fout, "  + ORIGIN %d %d\n", xo, yo);
                 }
                 if (via->hasOffset()) {
                    (void)via->offset(&xbo, &ybo, &xto, &yto);
                    fprintf(fout, "  + OFFSET %d %d %d %d\n", xbo, ybo, xto, yto);
                 }
                 if (via->hasCutPattern())
                    fprintf(fout, "  + PATTERN '%s'\n", via->cutPattern());
             }
             --numObjs;
             if (numObjs <= 0)
                 fprintf(fout, "END VIAS\n");
         }
         break;
  case defrRegionCbkType :
         re = (defiRegion*)cl;
         fprintf(fout, "- %s ", re->name());
         for (i = 0; i < re->numRectangles(); i++)
             fprintf(fout, "%d %d %d %d \n", re->xl(i),
                     re->yl(i), re->xh(i),
                     re->yh(i));
         if (re->hasType())
             fprintf(fout, "+ TYPE %s\n", re->type());
         if (re->numProps()) {
             for (i = 0; i < re->numProps(); i++) {
                 fprintf(fout, "+ PROPERTY %s %s ", re->propName(i),
                         re->propValue(i));
                 switch (re->propType(i)) {
                    case 'R': fprintf(fout, "REAL ");
                              break;
                    case 'I': fprintf(fout, "INTEGER ");
                              break;
                    case 'S': fprintf(fout, "STRING ");
                              break;
                    case 'Q': fprintf(fout, "QUOTESTRING ");
                              break;
                    case 'N': fprintf(fout, "NUMBER ");
                              break;
                 }
             }
         }
         fprintf(fout, ";\n"); 
         --numObjs;
         if (numObjs <= 0) {
             fprintf(fout, "END REGIONS\n");
         }
         break;
  case defrGroupNameCbkType :
         if ((char*)cl) {
             fprintf(fout, "- %s", (char*)cl);
         }
         break;
  case defrGroupMemberCbkType :
         if ((char*)cl) {
             fprintf(fout, " %s", (char*)cl);
         }
         break;
  case defrComponentMaskShiftLayerCbkType :
	 fprintf(fout, "COMPONENTMASKSHIFT ");
        
        for (i = 0; i < maskShiftLayer->numMaskShiftLayers(); i++) {
           fprintf(fout, "%s ", maskShiftLayer->maskShiftLayer(i));
        } 
        fprintf(fout, ";\n");
	 break;
  case defrGroupCbkType :
         group = (defiGroup*)cl;
         if (group->hasMaxX() | group->hasMaxY()
             | group->hasPerim()) {
             fprintf(fout, "\n  + SOFT ");
             if (group->hasPerim()) 
                 fprintf(fout, "MAXHALFPERIMETER %d ",
                         group->perim());
             if (group->hasMaxX())
                 fprintf(fout, "MAXX %d ", group->maxX());
             if (group->hasMaxY()) 
                 fprintf(fout, "MAXY %d ", group->maxY());
         } 
         if (group->hasRegionName())
             fprintf(fout, "\n  + REGION %s ", group->regionName());
         if (group->hasRegionBox()) {
             int *gxl, *gyl, *gxh, *gyh;
             int size;
             group->regionRects(&size, &gxl, &gyl, &gxh, &gyh);
             for (i = 0; i < size; i++)
                 fprintf(fout, "REGION %d %d %d %d ", gxl[i], gyl[i],
                         gxh[i], gyh[i]);
         }
         if (group->numProps()) {
             for (i = 0; i < group->numProps(); i++) {
                 fprintf(fout, "\n  + PROPERTY %s %s ",
                         group->propName(i),
                         group->propValue(i));
                 switch (group->propType(i)) {
                    case 'R': fprintf(fout, "REAL ");
                              break;
                    case 'I': fprintf(fout, "INTEGER ");
                              break;
                    case 'S': fprintf(fout, "STRING ");
                              break;
                    case 'Q': fprintf(fout, "QUOTESTRING ");
                              break;
                    case 'N': fprintf(fout, "NUMBER ");
                              break;
                 }
             }
         }
         fprintf(fout, " ;\n");
         --numObjs;
         if (numObjs <= 0)
             fprintf(fout, "END GROUPS\n");
         break;
  case defrScanchainCbkType :
         sc = (defiScanchain*)cl;
         fprintf(fout, "- %s\n", sc->name());
         if (sc->hasStart()) {
             sc->start(&a1, &b1);
             fprintf(fout, "  + START %s %s\n", a1, b1);
         }
         if (sc->hasStop()) {
             sc->stop(&a1, &b1);
             fprintf(fout, "  + STOP %s %s\n", a1, b1);
         }
         if (sc->hasCommonInPin() ||
             sc->hasCommonOutPin()) {
             fprintf(fout, "  + COMMONSCANPINS ");
             if (sc->hasCommonInPin())
                fprintf(fout, " ( IN %s ) ", sc->commonInPin());
             if (sc->hasCommonOutPin())
                fprintf(fout, " ( OUT %s ) ",sc->commonOutPin());
             fprintf(fout, "\n");
         }
         if (sc->hasFloating()) {
            sc->floating(&size, &inst, &inPin, &outPin, &bits);
            if (size > 0)
                fprintf(fout, "  + FLOATING\n");
            for (i = 0; i < size; i++) {
                fprintf(fout, "    %s ", inst[i]);
                if (inPin[i])
                   fprintf(fout, "( IN %s ) ", inPin[i]);
                if (outPin[i])
                   fprintf(fout, "( OUT %s ) ", outPin[i]);
                if (bits[i] != -1)
                   fprintf(fout, "( BITS %d ) ", bits[i]);
                fprintf(fout, "\n");
            }
         }

         if (sc->hasOrdered()) {
            for (i = 0; i < sc->numOrderedLists(); i++) {
                sc->ordered(i, &size, &inst, &inPin, &outPin,
                                           &bits);
                if (size > 0)
                    fprintf(fout, "  + ORDERED\n");
                for (j = 0; j < size; j++) {
                    fprintf(fout, "    %s ", inst[j]); 
                    if (inPin[j])
                       fprintf(fout, "( IN %s ) ", inPin[j]);
                    if (outPin[j])
                       fprintf(fout, "( OUT %s ) ", outPin[j]);
                    if (bits[j] != -1)
                       fprintf(fout, "( BITS %d ) ", bits[j]);
                    fprintf(fout, "\n");
                }
            }
         }

         if (sc->hasPartition()) {
            fprintf(fout, "  + PARTITION %s ",
                    sc->partitionName());
            if (sc->hasPartitionMaxBits())
              fprintf(fout, "MAXBITS %d ",
                      sc->partitionMaxBits());
         }
         fprintf(fout, ";\n");
         --numObjs;
         if (numObjs <= 0)
             fprintf(fout, "END SCANCHAINS\n");
         break;
  case defrIOTimingCbkType :
         iot = (defiIOTiming*)cl;
         fprintf(fout, "- ( %s %s )\n", iot->inst(),
                 iot->pin());
         if (iot->hasSlewRise())
             fprintf(fout, "  + RISE SLEWRATE %g %g\n",
                     iot->slewRiseMin(),
                     iot->slewRiseMax());
         if (iot->hasSlewFall())
             fprintf(fout, "  + FALL SLEWRATE %g %g\n",
                     iot->slewFallMin(),
                     iot->slewFallMax());
         if (iot->hasVariableRise())
             fprintf(fout, "  + RISE VARIABLE %g %g\n",
                     iot->variableRiseMin(),
                     iot->variableRiseMax());
         if (iot->hasVariableFall())
             fprintf(fout, "  + FALL VARIABLE %g %g\n",
                     iot->variableFallMin(),
                     iot->variableFallMax());
         if (iot->hasCapacitance())
             fprintf(fout, "  + CAPACITANCE %g\n",
                     iot->capacitance());
         if (iot->hasDriveCell()) {
             fprintf(fout, "  + DRIVECELL %s ",
                     iot->driveCell());
             if (iot->hasFrom())
                 fprintf(fout, "  FROMPIN %s ",
                         iot->from());
             if (iot->hasTo())
                 fprintf(fout, "  TOPIN %s ",
                         iot->to());
             if (iot->hasParallel())
                 fprintf(fout, "PARALLEL %g",
                         iot->parallel());
             fprintf(fout, "\n");
         }
         fprintf(fout, ";\n");
         --numObjs;
         if (numObjs <= 0)
             fprintf(fout, "END IOTIMINGS\n");
         break;
  case defrFPCCbkType :
         fpc = (defiFPC*)cl;
         fprintf(fout, "- %s ", fpc->name());
         if (fpc->isVertical())
             fprintf(fout, "VERTICAL ");
         if (fpc->isHorizontal())
             fprintf(fout, "HORIZONTAL ");
         if (fpc->hasAlign())
             fprintf(fout, "ALIGN ");
         if (fpc->hasMax())
             fprintf(fout, "%g ", fpc->alignMax());
         if (fpc->hasMin())
             fprintf(fout, "%g ", fpc->alignMin());
         if (fpc->hasEqual())
             fprintf(fout, "%g ", fpc->equal());
         for (i = 0; i < fpc->numParts(); i++) {
             fpc->getPart(i, &corner, &typ, &name);
             if (corner == 'B')
                 fprintf(fout, "BOTTOMLEFT ");
             else
                 fprintf(fout, "TOPRIGHT ");
             if (typ == 'R')
                 fprintf(fout, "ROWS %s ", name);
             else
                 fprintf(fout, "COMPS %s ", name);
         }
         fprintf(fout, ";\n");
         --numObjs;
         if (numObjs <= 0)
             fprintf(fout, "END FLOORPLANCONSTRAINTS\n");
         break;
  case defrTimingDisableCbkType :
         td = (defiTimingDisable*)cl;
         if (td->hasFromTo())
             fprintf(fout, "- FROMPIN %s %s ",
                     td->fromInst(),
                     td->fromPin(),
                     td->toInst(),
                     td->toPin());
         if (td->hasThru())
             fprintf(fout, "- THRUPIN %s %s ",
                     td->thruInst(),
                     td->thruPin());
         if (td->hasMacroFromTo())
             fprintf(fout, "- MACRO %s FROMPIN %s %s ",
                     td->macroName(),
                     td->fromPin(),
                     td->toPin());
         if (td->hasMacroThru())
             fprintf(fout, "- MACRO %s THRUPIN %s %s ",
                     td->macroName(),
                     td->fromPin());
         fprintf(fout, ";\n");
         break;
  case defrPartitionCbkType :
         part = (defiPartition*)cl;
         fprintf(fout, "- %s ", part->name());
         if (part->isSetupRise() |
             part->isSetupFall() |
             part->isHoldRise() |
             part->isHoldFall()) {
             // has turnoff 
             fprintf(fout, "TURNOFF "); 
             if (part->isSetupRise())
                 fprintf(fout, "SETUPRISE "); 
             if (part->isSetupFall())
                 fprintf(fout, "SETUPFALL "); 
             if (part->isHoldRise())
                 fprintf(fout, "HOLDRISE "); 
             if (part->isHoldFall())
                 fprintf(fout, "HOLDFALL "); 
         }
         itemT = part->itemType();
         dir = part->direction();
         if (strcmp(itemT, "CLOCK") == 0) {
             if (dir == 'T')    // toclockpin
                 fprintf(fout, "+ TOCLOCKPIN %s %s ",
                         part->instName(),
                         part->pinName());
             if (dir == 'F')    // fromclockpin
                 fprintf(fout, "+ FROMCLOCKPIN %s %s ",
                         part->instName(),
                         part->pinName());
             if (part->hasMin())
                 fprintf(fout, "MIN %g %g ",
                         part->partitionMin(),
                         part->partitionMax());
             if (part->hasMax())
                 fprintf(fout, "MAX %g %g ",
                         part->partitionMin(),
                         part->partitionMax());
             fprintf(fout, "PINS ");
             for (i = 0; i < part->numPins(); i++)
                  fprintf(fout, "%s ", part->pin(i));
         } else if (strcmp(itemT, "IO") == 0) {
             if (dir == 'T')    // toiopin
                 fprintf(fout, "+ TOIOPIN %s %s ",
                         part->instName(),
                         part->pinName());
             if (dir == 'F')    // fromiopin
                 fprintf(fout, "+ FROMIOPIN %s %s ",
                         part->instName(),
                         part->pinName());
         } else if (strcmp(itemT, "COMP") == 0) {
             if (dir == 'T')    // tocomppin
                 fprintf(fout, "+ TOCOMPPIN %s %s ",
                         part->instName(),
                         part->pinName());
             if (dir == 'F')    // fromcomppin
                 fprintf(fout, "+ FROMCOMPPIN %s %s ",
                         part->instName(),
                         part->pinName());
         }
         fprintf(fout, ";\n");
         --numObjs;
         if (numObjs <= 0)
             fprintf(fout, "END PARTITIONS\n");
         break;

  case defrPinPropCbkType :
         pprop = (defiPinProp*)cl;
         if (pprop->isPin())
            fprintf(fout, "- PIN %s ", pprop->pinName());
         else 
            fprintf(fout, "- %s %s ",
                    pprop->instName(),
                    pprop->pinName());
         fprintf(fout, ";\n");
         if (pprop->numProps() > 0) {
            for (i = 0; i < pprop->numProps(); i++) {
                fprintf(fout, "  + PROPERTY %s %s ",
                        pprop->propName(i),
                        pprop->propValue(i));
                switch (pprop->propType(i)) {
                   case 'R': fprintf(fout, "REAL ");
                             break;
                   case 'I': fprintf(fout, "INTEGER ");
                             break;
                   case 'S': fprintf(fout, "STRING ");
                             break;
                   case 'Q': fprintf(fout, "QUOTESTRING ");
                             break;
                   case 'N': fprintf(fout, "NUMBER ");
                             break;
                }
            }
            fprintf(fout, ";\n");
         }
         --numObjs;
         if (numObjs <= 0)
             fprintf(fout, "END PINPROPERTIES\n");
         break;
  case defrBlockageCbkType :
         block = (defiBlockage*)cl;
         if (testDebugPrint) {
             block->print(fout);
         } else {
             if (block->hasLayer()) {
                fprintf(fout, "- LAYER %s\n", block->layerName());
                if (block->hasComponent())
                   fprintf(fout, "   + COMPONENT %s\n",
                           block->layerComponentName());
                if (block->hasSlots())
                   fprintf(fout, "   + SLOTS\n");
                if (block->hasFills())
                   fprintf(fout, "   + FILLS\n");
                if (block->hasPushdown())
                   fprintf(fout, "   + PUSHDOWN\n");
                if (block->hasExceptpgnet())
                   fprintf(fout, "   + EXCEPTPGNET\n");
                if (block->hasMask())
                   fprintf(fout, "   + MASK %d\n", block->mask());
                if (block->hasSpacing())
                   fprintf(fout, "   + SPACING %d\n",
                           block->minSpacing());
                if (block->hasDesignRuleWidth())
                   fprintf(fout, "   + DESIGNRULEWIDTH %d\n",
                           block->designRuleWidth());
             }
             else if (block->hasPlacement()) {
                fprintf(fout, "- PLACEMENT\n");
                if (block->hasSoft())
                   fprintf(fout, "   + SOFT\n");
                if (block->hasPartial())
                   fprintf(fout, "   + PARTIAL %g\n",
                           block->placementMaxDensity());
                if (block->hasComponent())
                   fprintf(fout, "   + COMPONENT %s\n",
                           block->placementComponentName());
                if (block->hasPushdown())
                   fprintf(fout, "   + PUSHDOWN\n");
             }

             for (i = 0; i < block->numRectangles(); i++) {
                fprintf(fout, "   RECT %d %d %d %d\n", 
                        block->xl(i), block->yl(i),
                        block->xh(i), block->yh(i));
             } 

             for (i = 0; i < block->numPolygons(); i++) {
                fprintf(fout, "   POLYGON ");
                points = block->getPolygon(i);
                for (j = 0; j < points.numPoints; j++)
                   fprintf(fout, "%d %d ", points.x[j], points.y[j]);
                fprintf(fout, "\n");
             }
             fprintf(fout, ";\n");
             --numObjs;
             if (numObjs <= 0)
                 fprintf(fout, "END BLOCKAGES\n");
         }
         break;
  case defrSlotCbkType :
         slots = (defiSlot*)cl;
         if (slots->hasLayer())
            fprintf(fout, "- LAYER %s\n", slots->layerName());

         for (i = 0; i < slots->numRectangles(); i++) {
            fprintf(fout, "   RECT %d %d %d %d\n", 
                    slots->xl(i), slots->yl(i),
                    slots->xh(i), slots->yh(i));
         } 
         for (i = 0; i < slots->numPolygons(); i++) {
            fprintf(fout, "   POLYGON ");
            points = slots->getPolygon(i);
            for (j = 0; j < points.numPoints; j++)
              fprintf(fout, "%d %d ", points.x[j], points.y[j]);
            fprintf(fout, ";\n");
         }
         fprintf(fout, ";\n");
         --numObjs;
         if (numObjs <= 0)
             fprintf(fout, "END SLOTS\n");
         break;
  case defrFillCbkType :
         fills = (defiFill*)cl;
         if (testDebugPrint) {
              fills->print(fout);
         } else {
             if (fills->hasLayer()) {
                fprintf(fout, "- LAYER %s", fills->layerName());
                if (fills->layerMask()) {
                    fprintf(fout, " + MASK %d", fills->layerMask());
                }
                if (fills->hasLayerOpc())
                   fprintf(fout, " + OPC");
                fprintf(fout, "\n");

                for (i = 0; i < fills->numRectangles(); i++) {
                   fprintf(fout, "   RECT %d %d %d %d\n", 
                           fills->xl(i), fills->yl(i),
                           fills->xh(i), fills->yh(i));
                } 
                for (i = 0; i < fills->numPolygons(); i++) {
                   fprintf(fout, "   POLYGON "); 
                   points = fills->getPolygon(i);
                   for (j = 0; j < points.numPoints; j++)
                     fprintf(fout, "%d %d ", points.x[j], points.y[j]);
                   fprintf(fout, ";\n");
                } 
                fprintf(fout, ";\n");
             }
             --numObjs;
             if (fills->hasVia()) {
                fprintf(fout, "- VIA %s", fills->viaName());
                if (fills->viaTopMask() || fills->viaCutMask()
                    || fills->viaBottomMask()) {
                    fprintf(fout, " + MASK %d%d%d", 
                    fills->viaTopMask(),
                           fills->viaCutMask(),
                           fills->viaBottomMask());
                }
                if (fills->hasViaOpc())
                   fprintf(fout, " + OPC");
                fprintf(fout, "\n");

                for (i = 0; i < fills->numViaPts(); i++) {
                   points = fills->getViaPts(i);
                   for (j = 0; j < points.numPoints; j++)
                      fprintf(fout, " %d %d", points.x[j], points.y[j]);
                   fprintf(fout, ";\n"); 
                }
                fprintf(fout, ";\n");
             }
             if (numObjs <= 0)
                 fprintf(fout, "END FILLS\n");
         }
         break;
  case defrStylesCbkType :
//         defiPoints points;
         styles = (defiStyles*)cl;
         fprintf(fout, "- STYLE %d ", styles->style());
         points = styles->getPolygon();
         for (j = 0; j < points.numPoints; j++)
            fprintf(fout, "%d %d ", points.x[j], points.y[j]);
         fprintf(fout, ";\n");
         --numObjs;
         if (numObjs <= 0)
             fprintf(fout, "END STYLES\n");
         break;

  default: fprintf(fout, "BOGUS callback to cls.\n"); return 1;
  }
  return 0;
}


int dn(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
  fprintf(fout, "DIVIDERCHAR \"%s\" ;\n",h);
  return 0;
}


int ext(defrCallbackType_e t, const char* c, defiUserData ud) {
  char* name;

  checkType(t);
  if (ud != userData) dataError();

  switch (t) {
  case defrNetExtCbkType : name = address("net"); break;
  case defrComponentExtCbkType : name = address("component"); break;
  case defrPinExtCbkType : name = address("pin"); break;
  case defrViaExtCbkType : name = address("via"); break;
  case defrNetConnectionExtCbkType : name = address("net connection"); break;
  case defrGroupExtCbkType : name = address("group"); break;
  case defrScanChainExtCbkType : name = address("scanchain"); break;
  case defrIoTimingsExtCbkType : name = address("io timing"); break;
  case defrPartitionsExtCbkType : name = address("partition"); break;
  default: name = address("BOGUS"); return 1;
  }
  fprintf(fout, "  %s extension %s\n", name, c);
  return 0;
}

int extension(defrCallbackType_e c, const char* extsn, defiUserData ud) {
  checkType(c);
  if (ud != userData) dataError();
  fprintf(fout, "BEGINEXT %s\n", extsn);
  return 0;
}

void* mallocCB(size_t size) {
  return malloc(size);
}

void* reallocCB(void* name, size_t size) {
  return realloc(name, size);
}

void freeCB(void* name) {
  free(name);
  return;
}


BEGIN_LEFDEF_PARSER_NAMESPACE
extern long long nlines;
END_LEFDEF_PARSER_NAMESPACE
static int ccr1131444 = 0;

void lineNumberCB(long long lineNo) {

    // The CCR 1131444 tests ability of the DEF parser to count 
    // input line numbers out of 32-bit int range. On the first callback 
    // call 10G lines will be added to line counter. It should be 
    // reflected in output.
    if (ccr1131444) {
        lineNo += 10000000000LL;
        defrSetNLines(lineNo);
        ccr1131444 = 0;
    }

#ifdef _WIN32
  fprintf(fout, "Parsed %I64d number of lines!!\n", lineNo);
#else 
  fprintf(fout, "Parsed %lld number of lines!!\n", lineNo);
#endif
}

int unUsedCB(defrCallbackType_e, void*, defiUserData) {
  fprintf(fout, "This callback is not used.\n");
  return 0;
}

void printWarning(const char *str)
{
    fprintf(stderr, "%s\n", str);
}

int main(int argc, char** argv) {
  int num = 99;
  char* inFile[6];
  char* outFile;
  FILE* f;
  int res;
  int noCalls = 0;
//  long start_mem;
  int retStr = 0;
  int numInFile = 0;
  int fileCt = 0;
  int test1 = 0;
  int test2 = 0;
  int noNetCb = 0;
  int ccr749853 = 0;
  int line_num_print_interval = 50;

#ifdef WIN32
    // Enable two-digit exponent format
    _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

//  start_mem = (long)sbrk(0);

  strcpy(defaultName, "def.in");
  strcpy(defaultOut, "list");
  inFile[0] = defaultName;
  outFile = defaultOut;
  fout = stdout;
  userData = (void*) 0x01020304;
  argc--;
  argv++;

  if (argc == 0) {
    fprintf(stderr, "Type 'defrw --help' for the help.\n");
    return 2;
  }

  while (argc--) {
    if (strcmp(*argv, "-d") == 0) {
      argv++;
      argc--;
      sscanf(*argv, "%d", &num);
      defiSetDebug(num, 1);
    } else if (strcmp(*argv, "-nc") == 0) {
      noCalls = 1;
    } else if (strcmp(*argv, "-o") == 0) {
      argv++;
      argc--;
      outFile = *argv;
      if ((fout = fopen(outFile, "w")) == 0) {
	        fprintf(stderr, "ERROR: could not open output file\n");
	        return 2;
        }
    } else if (strcmp(*argv, "-verStr") == 0) {
        /* New to set the version callback routine to return a string    */
        /* instead of double.                                            */
        retStr = 1;
    } else if (strcmp(*argv, "-i") == 0) {
        argv++;
        argc--;
        line_num_print_interval = atoi(*argv);
    } else if (strcmp(*argv, "-test1") == 0) {
      test1 = 1;
    } else if (strcmp(*argv, "-test2") == 0) {
      test2 = 1;
    } else if (strcmp(*argv, "-noNet") == 0) {
      noNetCb = 1;
    } else if (strcmp(*argv, "-ccr749853") == 0) {
      ccr749853 = 1;
    } else if (strcmp(*argv, "-ccr1131444") == 0) {
        ccr1131444 = 1;
    } else if (strcmp(*argv, "-testDebugPrint") == 0) {
        testDebugPrint = 1;
    } else if (strcmp(*argv, "-sessionless") == 0) {
        isSessionless = 1;
    } else if (strcmp(*argv, "-ignoreRowNames") == 0) {
        ignoreRowNames = 1;
	} else if (strcmp(*argv, "-ignoreViaNames") == 0) {
        ignoreViaNames = 1;
	} else if (argv[0][0] != '-') {
      if (numInFile >= 6) {
        fprintf(stderr, "ERROR: too many input files, max = 6.\n");
        return 2;
      }
      inFile[numInFile++] = *argv;
    } else if ((strcmp(*argv, "-h") == 0) || (strcmp(*argv, "--help") == 0)) {
      fprintf(stderr, "Usage: defrw (<option>|<file>)* \n");
      fprintf(stderr, "Files:\n");
      fprintf(stderr, "\tupto 6 DEF files many be supplied.\n");
      fprintf(stderr, "\tif <defFileName> is set to 'STDIN' - takes data from stdin.\n");
      fprintf(stderr, "Options:\n");
      fprintf(stderr, "\t-i <num_lines> -- sets processing msg interval (default: 50 lines).\n");
      fprintf(stderr, "\t-nc            -- no functional callbacks will be called.\n");
      fprintf(stderr, "\t-o <out_file>  -- write output to the file.\n");
      fprintf(stderr, "\t-ignoreRowNames   -- don't output row names.\n");
      fprintf(stderr, "\t-ignoreViaNames   -- don't output via names.\n");
      return 2;
    } else if (strcmp(*argv, "-setSNetWireCbk") == 0) {
      setSNetWireCbk = 1;
    } else {
      fprintf(stderr, "ERROR: Illegal command line option: '%s'\n", *argv);
      return 2;
    }

    argv++;
  }

  //defrSetLogFunction(myLogFunction);
  //defrSetWarningLogFunction(myWarningLogFunction);

  if (isSessionless) {
      defrInitSession(0);
	  defrSetLongLineNumberFunction(lineNumberCB);
	  defrSetDeltaNumberLines(line_num_print_interval);
  }

  defrInitSession(isSessionless ? 0 : 1);

  if (noCalls == 0) {

    defrSetWarningLogFunction(printWarning);


    defrSetUserData((void*)3);
    defrSetDesignCbk(dname);
    defrSetTechnologyCbk(tname);
    defrSetExtensionCbk(extension);
    defrSetDesignEndCbk(done);
    defrSetPropDefStartCbk(propstart);
    defrSetPropCbk(prop);
    defrSetPropDefEndCbk(propend);
    /* Test for CCR 766289*/
    if (!noNetCb)
      defrSetNetCbk(netf);
    defrSetNetNameCbk(netNamef);
    defrSetNetNonDefaultRuleCbk(nondefRulef);
    defrSetNetSubnetNameCbk(subnetNamef);
    defrSetNetPartialPathCbk(netpath);
    defrSetSNetCbk(snetf);
    defrSetSNetPartialPathCbk(snetpath);
    if (setSNetWireCbk)
      defrSetSNetWireCbk(snetwire);
    defrSetComponentMaskShiftLayerCbk(compMSL);
    defrSetComponentCbk(compf);
    defrSetAddPathToNet();
    defrSetHistoryCbk(hist);
    defrSetConstraintCbk(constraint);
    defrSetAssertionCbk(constraint);
    defrSetArrayNameCbk(an);
    defrSetFloorPlanNameCbk(fn);
    defrSetDividerCbk(dn);
    defrSetBusBitCbk(bbn);
    defrSetNonDefaultCbk(ndr);

    defrSetAssertionsStartCbk(constraintst);
    defrSetConstraintsStartCbk(constraintst);
    defrSetComponentStartCbk(cs);
    defrSetPinPropStartCbk(cs);
    defrSetNetStartCbk(cs);
    defrSetStartPinsCbk(cs);
    defrSetViaStartCbk(cs);
    defrSetRegionStartCbk(cs);
    defrSetSNetStartCbk(cs);
    defrSetGroupsStartCbk(cs);
    defrSetScanchainsStartCbk(cs);
    defrSetIOTimingsStartCbk(cs);
    defrSetFPCStartCbk(cs);
    defrSetTimingDisablesStartCbk(cs);
    defrSetPartitionsStartCbk(cs);
    defrSetBlockageStartCbk(cs);
    defrSetSlotStartCbk(cs);
    defrSetFillStartCbk(cs);
    defrSetNonDefaultStartCbk(cs);
    defrSetStylesStartCbk(cs);

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
    if (!retStr)
       defrSetVersionCbk(vers);
    else
       defrSetVersionStrCbk(versStr);
    defrSetCaseSensitiveCbk(casesens);

    // The following calls are an example of using one function "cls"
    // to be the callback for many DIFFERENT types of constructs.
    // We have to cast the function type to meet the requirements
    // of each different set function.
    defrSetSiteCbk((defrSiteCbkFnType)cls);
    defrSetCanplaceCbk((defrSiteCbkFnType)cls);
    defrSetCannotOccupyCbk((defrSiteCbkFnType)cls);
    defrSetDieAreaCbk((defrBoxCbkFnType)cls);
    defrSetPinCapCbk((defrPinCapCbkFnType)cls);
    defrSetPinCbk((defrPinCbkFnType)cls);
    defrSetPinPropCbk((defrPinPropCbkFnType)cls);
    defrSetDefaultCapCbk((defrIntegerCbkFnType)cls);
    defrSetRowCbk((defrRowCbkFnType)cls);
    defrSetTrackCbk((defrTrackCbkFnType)cls);
    defrSetGcellGridCbk((defrGcellGridCbkFnType)cls);
    defrSetViaCbk((defrViaCbkFnType)cls);
    defrSetRegionCbk((defrRegionCbkFnType)cls);
    defrSetGroupNameCbk((defrStringCbkFnType)cls);
    defrSetGroupMemberCbk((defrStringCbkFnType)cls);
    defrSetGroupCbk((defrGroupCbkFnType)cls);
    defrSetScanchainCbk((defrScanchainCbkFnType)cls);
    defrSetIOTimingCbk((defrIOTimingCbkFnType)cls);
    defrSetFPCCbk((defrFPCCbkFnType)cls);
    defrSetTimingDisableCbk((defrTimingDisableCbkFnType)cls);
    defrSetPartitionCbk((defrPartitionCbkFnType)cls);
    defrSetBlockageCbk((defrBlockageCbkFnType)cls);
    defrSetSlotCbk((defrSlotCbkFnType)cls);
    defrSetFillCbk((defrFillCbkFnType)cls);
    defrSetStylesCbk((defrStylesCbkFnType)cls);

    defrSetAssertionsEndCbk(endfunc);
    defrSetComponentEndCbk(endfunc);
    defrSetConstraintsEndCbk(endfunc);
    defrSetNetEndCbk(endfunc);
    defrSetFPCEndCbk(endfunc);
    defrSetFPCEndCbk(endfunc);
    defrSetGroupsEndCbk(endfunc);
    defrSetIOTimingsEndCbk(endfunc);
    defrSetNetEndCbk(endfunc);
    defrSetPartitionsEndCbk(endfunc);
    defrSetRegionEndCbk(endfunc);
    defrSetSNetEndCbk(endfunc);
    defrSetScanchainsEndCbk(endfunc);
    defrSetPinEndCbk(endfunc);
    defrSetTimingDisablesEndCbk(endfunc);
    defrSetViaEndCbk(endfunc);
    defrSetPinPropEndCbk(endfunc);
    defrSetBlockageEndCbk(endfunc);
    defrSetSlotEndCbk(endfunc);
    defrSetFillEndCbk(endfunc);
    defrSetNonDefaultEndCbk(endfunc);
    defrSetStylesEndCbk(endfunc);

    defrSetMallocFunction(mallocCB);
    defrSetReallocFunction(reallocCB);
    defrSetFreeFunction(freeCB);

    //defrSetRegisterUnusedCallbacks();

    // Testing to set the number of warnings
    defrSetAssertionWarnings(3);
    defrSetBlockageWarnings(3);
    defrSetCaseSensitiveWarnings(3);
    defrSetComponentWarnings(3);
    defrSetConstraintWarnings(0);
    defrSetDefaultCapWarnings(3);
    defrSetGcellGridWarnings(3);
    defrSetIOTimingWarnings(3);
    defrSetNetWarnings(3);
    defrSetNonDefaultWarnings(3);
    defrSetPinExtWarnings(3);
    defrSetPinWarnings(3);
    defrSetRegionWarnings(3);
    defrSetRowWarnings(3);
    defrSetScanchainWarnings(3);
    defrSetSNetWarnings(3);
    defrSetStylesWarnings(3);
    defrSetTrackWarnings(3);
    defrSetUnitsWarnings(3);
    defrSetVersionWarnings(3);
    defrSetViaWarnings(3);
  }

  if (! isSessionless) {
	  defrSetLongLineNumberFunction(lineNumberCB);
	  defrSetDeltaNumberLines(line_num_print_interval);
  }

  (void) defrSetOpenLogFileAppend();

  if (ccr749853) {
    defrSetTotalMsgLimit (5);
    defrSetLimitPerMsg (6008, 2);

  } 

  if (test1) {  // for special tests
     for (fileCt = 0; fileCt < numInFile; fileCt++) {
       if ((f = fopen(inFile[fileCt],"r")) == 0) {
         fprintf(stderr,"Couldn't open input file '%s'\n", inFile[fileCt]);
         return(2);
       }
       // Set case sensitive to 0 to start with, in History & PropertyDefinition
       // reset it to 1.
       res = defrRead(f, inFile[fileCt], userData, 1);

       if (res)
          fprintf(stderr, "Reader returns bad status.\n", inFile[fileCt]);

       (void)defrPrintUnusedCallbacks(fout);
       (void)defrReleaseNResetMemory();
       (void)defrUnsetNonDefaultCbk(); 
       (void)defrUnsetNonDefaultStartCbk(); 
       (void)defrUnsetNonDefaultEndCbk(); 
     }
  }
  else if (test2) {  // for special tests
    // this test is design to test the 3 APIs, defrDisableParserMsgs,
    // defrEnableParserMsgs & defrEnableAllMsgs
    // It uses the file ccr523879.def.  This file will parser 3 times
    // 1st it will have defrDisableParserMsgs set to both 7010 & 7016
    // 2nd will enable 7016 by calling defrEnableParserMsgs
    // 3rd enable all msgs by call defrEnableAllMsgs

    int nMsgs = 2;
    int dMsgs[2];

    for (fileCt = 0; fileCt < numInFile; fileCt++) {
       if (fileCt == 0) {
          dMsgs[0] = 7010;
          dMsgs[1] = 7016;
          defrDisableParserMsgs (2, (int*)dMsgs);
       } else if (fileCt == 1) {
          dMsgs[0] = 7016;
          defrEnableParserMsgs (1, (int*)dMsgs);
       } else
          defrEnableAllMsgs();
 
       if ((f = fopen(inFile[fileCt],"r")) == 0) {
         fprintf(stderr,"Couldn't open input file '%s'\n", inFile[fileCt]);
         return(2);
       }
 
       res = defrRead(f, inFile[fileCt], userData, 1);
 
       if (res)
          fprintf(stderr, "Reader returns bad status.\n", inFile[fileCt]);
 
       (void)defrPrintUnusedCallbacks(fout);
       (void)defrReleaseNResetMemory();
       (void)defrUnsetNonDefaultCbk();
       (void)defrUnsetNonDefaultStartCbk();
       (void)defrUnsetNonDefaultEndCbk();
    }
  } else {
     for (fileCt = 0; fileCt < numInFile; fileCt++) {
       if (strcmp(inFile[fileCt], "STDIN") == 0) {
            f = stdin;
       } else if ((f = fopen(inFile[fileCt],"r")) == 0) {
             fprintf(stderr,"Couldn't open input file '%s'\n", inFile[fileCt]);
             return(2);
       }
       // Set case sensitive to 0 to start with, in History & PropertyDefinition
       // reset it to 1.

       res = defrRead(f, inFile[fileCt], userData, 1);

       if (res)
           fprintf(stderr, "Reader returns bad status.\n", inFile[fileCt]);

       // Testing the aliases API.
       defrAddAlias ("alias1", "aliasValue1", 1);

       defiAlias_itr aliasStore;
       const char    *alias1Value = NULL;

       while (aliasStore.Next()) {
           if (strcmp(aliasStore.Key(), "alias1") == 0) {
                alias1Value = aliasStore.Data();
           }
       } 

       if (!alias1Value || strcmp(alias1Value, "aliasValue1")) {
             fprintf(stderr, "ERROR: Aliases don't work\n");
       }

       (void)defrPrintUnusedCallbacks(fout);
       (void)defrReleaseNResetMemory();
    }
    (void)defrUnsetCallbacks();
    (void)defrSetUnusedCallbacks(unUsedCB);
  }

  // Unset all the callbacks
  defrUnsetArrayNameCbk ();
  defrUnsetAssertionCbk ();
  defrUnsetAssertionsStartCbk ();
  defrUnsetAssertionsEndCbk ();
  defrUnsetBlockageCbk ();
  defrUnsetBlockageStartCbk ();
  defrUnsetBlockageEndCbk ();
  defrUnsetBusBitCbk ();
  defrUnsetCannotOccupyCbk ();
  defrUnsetCanplaceCbk ();
  defrUnsetCaseSensitiveCbk ();
  defrUnsetComponentCbk ();
  defrUnsetComponentExtCbk ();
  defrUnsetComponentStartCbk ();
  defrUnsetComponentEndCbk ();
  defrUnsetConstraintCbk ();
  defrUnsetConstraintsStartCbk ();
  defrUnsetConstraintsEndCbk ();
  defrUnsetDefaultCapCbk ();
  defrUnsetDesignCbk ();
  defrUnsetDesignEndCbk ();
  defrUnsetDieAreaCbk ();
  defrUnsetDividerCbk ();
  defrUnsetExtensionCbk ();
  defrUnsetFillCbk ();
  defrUnsetFillStartCbk ();
  defrUnsetFillEndCbk ();
  defrUnsetFPCCbk ();
  defrUnsetFPCStartCbk ();
  defrUnsetFPCEndCbk ();
  defrUnsetFloorPlanNameCbk ();
  defrUnsetGcellGridCbk ();
  defrUnsetGroupCbk ();
  defrUnsetGroupExtCbk ();
  defrUnsetGroupMemberCbk ();
  defrUnsetComponentMaskShiftLayerCbk ();
  defrUnsetGroupNameCbk ();
  defrUnsetGroupsStartCbk ();
  defrUnsetGroupsEndCbk ();
  defrUnsetHistoryCbk ();
  defrUnsetIOTimingCbk ();
  defrUnsetIOTimingsStartCbk ();
  defrUnsetIOTimingsEndCbk ();
  defrUnsetIOTimingsExtCbk ();
  defrUnsetNetCbk ();
  defrUnsetNetNameCbk ();
  defrUnsetNetNonDefaultRuleCbk ();
  defrUnsetNetConnectionExtCbk ();
  defrUnsetNetExtCbk ();
  defrUnsetNetPartialPathCbk ();
  defrUnsetNetSubnetNameCbk ();
  defrUnsetNetStartCbk ();
  defrUnsetNetEndCbk ();
  defrUnsetNonDefaultCbk ();
  defrUnsetNonDefaultStartCbk ();
  defrUnsetNonDefaultEndCbk ();
  defrUnsetPartitionCbk ();
  defrUnsetPartitionsExtCbk ();
  defrUnsetPartitionsStartCbk ();
  defrUnsetPartitionsEndCbk ();
  defrUnsetPathCbk ();
  defrUnsetPinCapCbk ();
  defrUnsetPinCbk ();
  defrUnsetPinEndCbk ();
  defrUnsetPinExtCbk ();
  defrUnsetPinPropCbk ();
  defrUnsetPinPropStartCbk ();
  defrUnsetPinPropEndCbk ();
  defrUnsetPropCbk ();
  defrUnsetPropDefEndCbk ();
  defrUnsetPropDefStartCbk ();
  defrUnsetRegionCbk ();
  defrUnsetRegionStartCbk ();
  defrUnsetRegionEndCbk ();
  defrUnsetRowCbk ();
  defrUnsetScanChainExtCbk ();
  defrUnsetScanchainCbk ();
  defrUnsetScanchainsStartCbk ();
  defrUnsetScanchainsEndCbk ();
  defrUnsetSiteCbk ();
  defrUnsetSlotCbk ();
  defrUnsetSlotStartCbk ();
  defrUnsetSlotEndCbk ();
  defrUnsetSNetWireCbk ();
  defrUnsetSNetCbk ();
  defrUnsetSNetStartCbk ();
  defrUnsetSNetEndCbk ();
  defrUnsetSNetPartialPathCbk ();
  defrUnsetStartPinsCbk ();
  defrUnsetStylesCbk ();
  defrUnsetStylesStartCbk ();
  defrUnsetStylesEndCbk ();
  defrUnsetTechnologyCbk ();
  defrUnsetTimingDisableCbk ();
  defrUnsetTimingDisablesStartCbk ();
  defrUnsetTimingDisablesEndCbk ();
  defrUnsetTrackCbk ();
  defrUnsetUnitsCbk ();
  defrUnsetVersionCbk ();
  defrUnsetVersionStrCbk ();
  defrUnsetViaCbk ();
  defrUnsetViaExtCbk ();
  defrUnsetViaStartCbk ();
  defrUnsetViaEndCbk ();

  fclose(fout);

  // Release allocated singleton data.
  defrClear();

  return res;
}

