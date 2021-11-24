// *****************************************************************************
// *****************************************************************************
// Copyright 2013, Cadence Design Systems
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
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#ifndef defiSite_h
#define defiSite_h

#include "defiKRDefs.hpp"
#include "defiMisc.hpp"
#include <stdio.h>

BEGIN_LEFDEF_PARSER_NAMESPACE

class defrData;

/*
 * Struct holds the data for one site.
 * It is also used for a canplace and cannotoccupy.
 */
class defiSite {
public:
  defiSite(defrData *data);
  void Init();

  ~defiSite();
  void Destroy();

  void clear();

  void setName(const char* name);
  void setLocation(double xorg, double yorg);
  void setOrient(int orient);
  void setDo(double x_num, double y_num, double x_step, double y_step);

  double x_num() const;
  double y_num() const;
  double x_step() const;
  double y_step() const;
  double x_orig() const;
  double y_orig() const;
  int orient() const;
  const char* orientStr() const;
  const char* name() const;

  void print(FILE* f) const;

  void bumpName(int size);

protected:
  char* siteName_;     // Name of this.
  int nameSize_;       // allocated size of siteName_
  double x_orig_, y_orig_;  // Origin
  double x_step_, y_step_;  // Array step size.
  double x_num_, y_num_; 
  int orient_;         // orientation

  defrData *defData;
};



/* Struct holds the data for a Box */
class defiBox {
public:
  // Use the default destructor and constructor.
  // 5.6 changed to use it own constructor & destructor

  defiBox();
  void Init();

  DEF_COPY_CONSTRUCTOR_H( defiBox );
  DEF_ASSIGN_OPERATOR_H( defiBox );

  void Destroy();
  ~defiBox();

  // NOTE: 5.6
  // The following methods are still here for backward compatibility
  // For new reader they should use numPoints & getPoint to get the
  // data.
  int xl() const;
  int yl() const;
  int xh() const;
  int yh() const;

  void addPoint(defiGeometries* geom);
  defiPoints getPoint() const;

  void print(FILE* f) const;

protected:
  int xl_, yl_;
  int xh_, yh_;
  defiPoints* points_;    // 5.6
};


END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
