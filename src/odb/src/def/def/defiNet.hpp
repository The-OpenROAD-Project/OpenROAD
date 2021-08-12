// *****************************************************************************
// *****************************************************************************
// Copyright 2013 - 2017, Cadence Design Systems
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
//  $Author: icftcm $
//  $Revision: #2 $
//  $Date: 2017/06/19 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#ifndef defiNet_h
#define defiNet_h

#include <stdio.h>
#include "defiKRDefs.hpp"
#include "defiPath.hpp"
#include "defiMisc.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

class defrData;

/* Return codes for defiNet::viaOrient 
    DEF_ORIENT_N  0
    DEF_ORIENT_W  1
    DEF_ORIENT_S  2
    DEF_ORIENT_E  3
    DEF_ORIENT_FN 4
    DEF_ORIENT_FW 5
    DEF_ORIENT_FS 6
    DEF_ORIENT_FE 7
*/

class defiWire {
public:
  defiWire(defrData *data);
  ~defiWire();
  DEF_COPY_CONSTRUCTOR_H( defiWire );
  DEF_ASSIGN_OPERATOR_H( defiWire );

  void Init(const char* type, const char* wireShieldName);
  void Destroy();
  void clear();
  void addPath(defiPath *p, int reset, int netOsnet, int *needCbk);

  const char* wireType() const;
  const char* wireShieldNetName() const;
  int         numPaths() const;

  defiPath*   path(int index);
  const defiPath*   path(int index) const;

  void bumpPaths(long long size);

protected:
  char*      type_;
  char*      wireShieldName_;    // It only set from specialnet SHIELD, 5.4
  int        numPaths_;
  long long  pathsAllocated_;
  defiPath** paths_;

  defrData  *defData;
};



class defiSubnet {
public:
  defiSubnet(defrData *data);
  void Init();

  DEF_COPY_CONSTRUCTOR_H( defiSubnet );
  DEF_ASSIGN_OPERATOR_H( defiSubnet );

  void Destroy();
  ~defiSubnet();

  void setName(const char* name);
  void setNonDefault(const char* name);
  void addPin(const char* instance, const char* pin, int syn);
  void addMustPin(const char* instance, const char* pin, int syn);

  // WMD -- the following will be removed by the next release
  void setType(const char* typ);  // Either FIXED COVER ROUTED
  void addPath(defiPath* p, int reset, int netOsnet, int *needCbk);

  // NEW: a net can have more than 1 wire
  void addWire(const char *typ); 
  void addWirePath(defiPath *p, int reset, int netOsnet, int *needCbk);

  // Debug printing
  void print(FILE* f) const;

  const char* name() const;
  int numConnections() const;
  const char* instance(int index) const;
  const char* pin(int index) const;
  int pinIsSynthesized(int index) const;
  int pinIsMustJoin(int index) const;

  // WMD -- the following will be removed by the next release
  int isFixed() const;
  int isRouted() const;
  int isCover() const;

  int hasNonDefaultRule() const;

  // WMD -- the following will be removed by the next release
  int numPaths() const;
  defiPath* path(int index);
  const defiPath* path(int index) const;

  const char* nonDefaultRule() const;

  int         numWires() const;
  defiWire*   wire(int index);
  const defiWire*   wire(int index) const;

  void bumpName(long long size);
  void bumpPins(long long  size);
  void bumpPaths(long long  size);
  void clear();

protected:
  char*         name_;            // name.
  int           nameSize_;          // allocated size of name.
  int           numPins_;           // number of pins used in array.
  long long     pinsAllocated_;     // number of pins allocated in array.
  char**        instances_;      // instance names for connections
  char**        pins_;           // pin names for connections
  char*         synthesized_;     // synthesized flags for pins
  char*         musts_;           // must-join flags

  // WMD -- the following will be removed by the next release
  char       isFixed_;        // net type
  char       isRouted_;
  char       isCover_;
  defiPath** paths_;          // paths for this subnet
  int        numPaths_;       // number of paths used
  long long  pathsAllocated_; // allocated size of paths array

  int        numWires_;          // number of wires defined in the subnet
  long long  wiresAllocated_;    // number of wires allocated in the subnet
  defiWire** wires_;             // this replace the paths
  char*      nonDefaultRule_;

  defrData *defData;
};



class defiVpin {
public:
  defiVpin(defrData *data);
  ~defiVpin();

  DEF_COPY_CONSTRUCTOR_H( defiVpin );
  DEF_ASSIGN_OPERATOR_H( defiVpin );

  void Init(const char* name);
  void Destroy();
  void setLayer(const char* name);
  void setBounds(int xl, int yl, int xh, int yh);
  void setOrient(int orient);
  void setLoc(int x, int y);
  void setStatus(char st);

  int xl() const ;
  int yl() const ;
  int xh() const ;
  int yh() const ;
  char status() const;      /* P-placed, F-fixed, C-cover, ' ' - not set */
  int orient() const ;
  const char* orientStr() const ;
  int xLoc() const;
  int yLoc() const;
  const char* name() const;
  const char* layer() const;

protected:
  int xl_;
  int yl_;
  int xh_;
  int yh_;
  int orient_;  /* 0-7  -1 is no orient */
  char status_; /* P-placed  F-fixed  C-cover  ' '- none */
  int xLoc_;
  int yLoc_;
  char* name_;
  char* layer_;

  defrData *defData;
};



// Pre 5.4
class defiShield {
public:
  defiShield(defrData *data);
  ~defiShield();

  DEF_COPY_CONSTRUCTOR_H( defiShield );
  DEF_ASSIGN_OPERATOR_H( defiShield );

  void Init(const char* name);
  void Destroy();
  void clear();
  void addPath(defiPath *p, int reset, int netOsnet, int *needCbk);

  const char* shieldName() const;
  int         numPaths() const;

  defiPath*         path(int index);
  const defiPath*   path(int index) const;

  void bumpPaths(long long size);

protected:
  char*      name_;
  int        numPaths_;
  long long  pathsAllocated_;
  defiPath** paths_;

  defrData *defData;
};




// Struct holds the data for one component.
class defiNet {
public:
  defiNet(defrData *data);
  void Init();

  DEF_COPY_CONSTRUCTOR_H( defiNet );
  DEF_ASSIGN_OPERATOR_H( defiNet );

  void Destroy();
  ~defiNet();

  // Routines used by YACC to set the fields in the net.
  void setName(const char* name);
  void addPin(const char* instance, const char* pin, int syn);
  void addMustPin(const char* instance, const char* pin, int syn);
  void setWeight(int w);

  // WMD -- the following will be removed by the next release
  void setType(const char* typ);  // Either FIXED COVER ROUTED

  void addProp(const char* name, const char* value, const char type);
  void addNumProp(const char* name, const double d,
                  const char* value, const char type);
  void addSubnet(defiSubnet* subnet);
  // NEW: a net can have more than 1 wire
  void addWire(const char *typ, const char* wireShieldName);
  void addWirePath(defiPath* p, int reset, int netOsnet, int *needCbk);
  void addShape(const char *shapeType);         // 5.8
  void setSource(const char* typ);
  void setFixedbump();                          // 5.4.1
  void setFrequency(double frequency);          // 5.4.1
  void setOriginal(const char* typ);
  void setPattern(const char* typ);
  void setCap(double w);
  void setUse(const char* typ);
  void setNonDefaultRule(const char* typ);
  void setStyle(int style);
  void addShield(const char* shieldNetName);    // pre 5.4
  void addNoShield(const char* shieldNetName);  // pre 5.4
  void addShieldNet(const char* shieldNetName);

  void addShieldPath(defiPath* p, int reset, int netOsnet, int *needCbk);
  void clear();
  void setWidth(const char* layer, double dist);
  void setSpacing(const char* layer, double dist);
  void setVoltage(double num);
  void setRange(double left, double right);
  void setXTalk(int num);
  void addVpin(const char* name);
  void addVpinLayer(const char* name);
  void addVpinLoc(const char* status, int x, int y, int orient);
  void addVpinBounds(int xl, int yl, int xh, int yh);
  // 5.6
  void addPolygon(const char* layerName, defiGeometries* geom, int *needCbk,
	          int mask, const char* routeStatus,
		  const char* shapeType,
                  const char* shieldNetName);
  void addRect(const char* layerName, int xl, int yl, int xh, int yh,
               int *needCbk, int mask, const char* routeStatus,
	       const char* shapeType,
               const char* shieldNetName); // 5.6
  void addPts(const char* viaName, int o, defiGeometries* geom,
	      int *needCbk, int mask, const char* routeStatus,
	      const char* shapeType,
              const char* shieldNetName);  //VIA 5.8

  // For OA to modify the netName, id & pinName
  void changeNetName(const char* name);
  void changeInstance(const char* name, int index);
  void changePin(const char* name, int index);

  // Routines to return the value of net data.
  const char*  name() const;
  int          weight() const;
  int          numProps() const;
  const char*  propName(int index) const;
  const char*  propValue(int index) const;
  double propNumber(int index) const;
  char   propType(int index) const;
  int    propIsNumber(int index) const;
  int    propIsString(int index) const;
  int          numConnections() const;
  const char*  instance(int index) const;
  const char*  pin(int index) const;
  int          pinIsMustJoin(int index) const;
  int          pinIsSynthesized(int index) const;
  int          numSubnets() const;

  defiSubnet*  subnet(int index);
  const defiSubnet*  subnet(int index) const;

  // WMD -- the following will be removed by the next release
  int         isFixed() const;
  int         isRouted() const;
  int         isCover() const;

  /* The following routines are for wiring */
  int         numWires() const;

  defiWire*   wire(int index);
  const defiWire*   wire(int index) const;

  /* Routines to get the information about Virtual Pins. */
  int       numVpins() const;
  
  defiVpin* vpin(int index);
  const defiVpin* vpin(int index) const;

  int hasProps() const;
  int hasWeight() const;
  int hasSubnets() const;
  int hasSource() const;
  int hasFixedbump() const;                          // 5.4.1
  int hasFrequency() const;                          // 5.4.1
  int hasPattern() const;
  int hasOriginal() const;
  int hasCap() const;
  int hasUse() const;
  int hasStyle() const;
  int hasNonDefaultRule() const;
  int hasVoltage() const;
  int hasSpacingRules() const;
  int hasWidthRules() const;
  int hasXTalk() const;

  int numSpacingRules() const;
  void spacingRule(int index, char** layer, double* dist, double* left,
                   double* right) const;
  int numWidthRules() const;
  void widthRule(int index, char** layer, double* dist) const;
  double voltage() const;

  int            XTalk() const;
  const char*    source() const;
  double         frequency() const;
  const char*    original() const;
  const char*    pattern() const;
  double         cap() const;
  const char*    use() const;
  int            style() const;
  const char*    nonDefaultRule() const;

  // WMD -- the following will be removed by the next release
  int            numPaths() const;
  
  defiPath*            path(int index);
  const defiPath*      path(int index) const;

  int            numShields() const;          // pre 5.4

  defiShield*    shield(int index);           // pre 5.4
  const defiShield*    shield(int index) const ;           // pre 5.4

  int            numShieldNets() const;
  const char*    shieldNet(int index) const;
  int            numNoShields() const;        // pre 5.4

  defiShield*    noShield(int index);         // pre 5.4
  const defiShield*    noShield(int index) const;         // pre 5.4

  // 5.6
  int            numPolygons() const;                 // 5.6
  const  char*   polygonName(int index) const;        // 5.6
  struct defiPoints getPolygon(int index) const;      // 5.6
  int            polyMask(int index) const;
  const char*    polyRouteStatus(int index) const;
  const char*    polyRouteStatusShieldName(int index) const;
  const char*    polyShapeType(int index) const;


  int  numRectangles() const;                         // 5.6
  const  char* rectName(int index) const;             // 5.6
  int  xl(int index)const;                            // 5.6
  int  yl(int index)const;                            // 5.6
  int  xh(int index)const;                            // 5.6
  int  yh(int index)const;                            // 5.6
  int  rectMask(int index)const;
  const char* rectRouteStatus(int index) const;
  const char* rectRouteStatusShieldName(int index) const;
  const char* rectShapeType(int index) const;
  

  // 5.8
  int  numViaSpecs() const;
  struct defiPoints getViaPts(int index) const;                       
  const char* viaName(int index) const;
  int viaOrient(int index) const;
  const char* viaOrientStr(int index) const;
  int topMaskNum(int index) const;
  int cutMaskNum(int index) const;
  int bottomMaskNum(int index) const;
  const char* viaRouteStatus(int index) const;
  const char* viaRouteStatusShieldName(int index) const;
  const char* viaShapeType(int index) const;

  // Debug printing
  void print(FILE* f) const;


  void bumpName(long long size);
  void bumpPins(long long size);
  void bumpProps(long long size);
  void bumpSubnets(long long size);
  void bumpPaths(long long  size);
  void bumpShieldNets(long long size);

  // The method freeWire() is added is user select to have a callback
  // per wire within a net This is an internal method and is not public
  void freeWire();
  void freeShield();

  // Clear the rectangles & polygons data if partial path callback is set
  void clearRectPolyNPath();
  void clearRectPoly();
  void clearVia();

protected:
  char*     name_;          // name.
  int       nameSize_;      // allocated size of name.
  int       numPins_;       // number of pins used in array.
  long long pinsAllocated_; // number of pins allocated in array.
  char**    instances_;     // instance names for connections
  char**    pins_;          // pin names for connections
  char*     musts_;         // must-join flags for pins
  char*     synthesized_;   // synthesized flags for pins
  int       weight_;        // net weight
  char      hasWeight_;     // flag for optional weight

  // WMD -- the following will be removed by the nex release
  char isFixed_;        // net type
  char isRouted_;
  char isCover_;

  char hasCap_;         // file supplied a capacitance value
  char hasFrequency_;   // file supplied a frequency value
  char hasVoltage_;
  int numProps_;        // num of props in array
  char**  propNames_;   // Prop names
  char**  propValues_;  // Prop values All in strings!
  double* propDValues_; // Prop values in numbers!
  char*   propTypes_;   // Prop types, 'I' - Integer, 'R' - Real, 'S' - String

  long long    propsAllocated_;   // allocated size of props array
  int          numSubnets_;       // num of subnets in array
  defiSubnet** subnets_;          // Prop names
  long long    subnetsAllocated_; // allocated size of props array
  double       cap_;              // cap value
  char*        source_;
  int          fixedbump_;     // 5.4.1
  double       frequency_;     // 5.4.1
  char* pattern_;
  char* original_;
  char* use_;
  char* nonDefaultRule_;
  int   style_;

  // WMD -- the following will be removed by the nex release
  defiPath** paths_;          // paths for this subnet
  int        numPaths_;       // number of paths used
  long long  pathsAllocated_; // allocated size of paths array

  double voltage_;

  int         numWires_;         // number of wires defined in the net
  long long   wiresAllocated_;   // allocated size of wire paths array
  defiWire**  wires_;            // this replace the paths

  long long   widthsAllocated_;
  int         numWidths_;
  char**      wlayers_;
  double*     wdist_;

  long long   spacingAllocated_;
  int         numSpacing_;
  char**      slayers_;
  double*     sdist_;
  double*     sleft_;
  double*     sright_;
  int         xTalk_;

  int         numVpins_;
  long long   vpinsAllocated_;
  defiVpin**  vpins_;

  int          numShields_;            // number of SHIELD paths used
  long long    shieldsAllocated_;      // allocated size of SHIELD paths array
  defiShield** shields_;               // SHIELD data 
  int          numNoShields_;          // number of NOSHIELD paths used

  int          numShieldNet_;          // number of SHIELDNETS used in array.
  long long    shieldNetsAllocated_;   // number of SHIELDNETS allocated in array.
  char**       shieldNet_;             // name of the SHIELDNET

  int          numPolys_;              // 5.6
  char**       polygonNames_;          // 5.6 layerName for POLYGON
  long long    polysAllocated_;        // 5.6
  struct defiPoints** polygons_;       // 5.6
  int*         polyMasks_;
  char** polyRouteStatus_;
  char** polyShapeTypes_;
  char** polyRouteStatusShieldNames_;

  int        numRects_;                    // 5.6
  long long  rectsAllocated_;              // 5.6
  char**     rectNames_;                   // 5.6
  int* xl_;
  int* yl_;
  int* xh_;
  int* yh_;
  int* rectMasks_;
  char** rectRouteStatus_;
  char** rectRouteStatusShieldNames_;
  char** rectShapeTypes_;
  

  struct defiPoints** viaPts_;      // 5.8                  
  char**              viaNames_;                   
  int                 numPts_;                    
  long long           ptsAllocated_;
  int*                viaOrients_;
  int*                viaMasks_;
  char**              viaRouteStatus_;
  char**              viaRouteStatusShieldNames_;
  char**              viaShapeTypes_;

  defrData *defData;
};

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
