// *****************************************************************************
// *****************************************************************************
// Copyright 2013 - 2015, Cadence Design Systems
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

#include <string.h>
#include <stdlib.h>
#include "defiSite.hpp"
#include "defiDebug.hpp"
#include "lex.h"
#include "defiUtil.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

//////////////////////////////////////////////
//////////////////////////////////////////////
//
//     defiSite
//
//////////////////////////////////////////////
//////////////////////////////////////////////


defiSite::defiSite(defrData *data)
 : defData(data)
{
  Init();
}


defiSite::~defiSite() {
  Destroy();
}


void defiSite::Init() {
  siteName_ = (char*)malloc(32);
  nameSize_ = 32;
  clear();
}


void defiSite::Destroy() {
  free(siteName_);
}


void defiSite::clear() {
  if (siteName_)
     *siteName_ = '\0';
  x_num_ = 0.0;
  y_num_ = 0.0;
  x_step_ = 0.0;
  y_step_ = 0.0;
  orient_ = 0;
}


void defiSite::setName(const char* name) {
  int len = 1;
  char* from = (char*)name;
  clear();
  while (*from++) len++;
  if (nameSize_ < len) bumpName(len);
  strcpy(siteName_, defData->DEFCASE(name));
}


void defiSite::setLocation(double xorg, double yorg) {
  x_orig_ = xorg;
  y_orig_ = yorg;
}


void defiSite::setOrient(int orient) {
  orient_ = orient;
}


void defiSite::setDo(double x_num, double y_num, double x_step, double y_step) {
  x_num_ = x_num;
  y_num_ = y_num;
  x_step_ = x_step;
  y_step_ = y_step;
}


double defiSite::x_num() const {
  return x_num_;
}


double defiSite::y_num() const {
  return y_num_;
}


double defiSite::x_step() const {
  return x_step_;
}


double defiSite::y_step() const {
  return y_step_;
}


double defiSite::x_orig() const {
  return x_orig_;
}


double defiSite::y_orig() const {
  return y_orig_;
}


int defiSite::orient() const {
  return orient_;
}


const char* defiSite::orientStr() const {
  return (defiOrientStr(orient_));
}


const char* defiSite::name() const {
  return siteName_;
}


void defiSite::bumpName(int size) {
  free(siteName_);
  siteName_ = (char*)malloc(size);
  nameSize_ = size;
  *siteName_ = '\0';
}


void defiSite::print(FILE* f) const {
  fprintf(f, "Site '%p' %s\n", name(),
     orientStr());
  fprintf(f, "  DO X %g %g BY %g\n",
     x_orig(),
     x_num(),
     x_step());
  fprintf(f, "  DO Y %g %g BY %g\n",
     y_orig(),
     y_num(),
     y_step());

}


//////////////////////////////////////////////
//////////////////////////////////////////////
//
//     defiBox
//
//////////////////////////////////////////////
//////////////////////////////////////////////

defiBox::defiBox() {
  Init();
}

void defiBox::Init() {
  xl_ = 0;
  yl_ = 0;
  xh_ = 0;
  yh_ = 0;
  points_ = 0;
}

DEF_COPY_CONSTRUCTOR_C( defiBox ) {
    this->Init();
   
    DEF_COPY_FUNC( xl_ );
    DEF_COPY_FUNC( yl_ );
    DEF_COPY_FUNC( xh_ );
    DEF_COPY_FUNC( yh_ );
    
    DEF_MALLOC_FUNC_WITH_OPERATOR( points_, defiPoints, sizeof(defiPoints)*1 );
}

DEF_ASSIGN_OPERATOR_C( defiBox ) {
    CHECK_SELF_ASSIGN
    this->Init();
   
    DEF_COPY_FUNC( xl_ );
    DEF_COPY_FUNC( yl_ );
    DEF_COPY_FUNC( xh_ );
    DEF_COPY_FUNC( yh_ );
    
    DEF_MALLOC_FUNC_WITH_OPERATOR( points_, defiPoints, sizeof(defiPoints)*1 );
    return *this;
}

defiBox::~defiBox() {
  Destroy();
}

void defiBox::Destroy() {
  struct defiPoints* p;

  p = points_;
  if (p) {
    free((char*)(p->x));
    free((char*)(p->y));
    free((char*)(points_));
  }
}

void defiBox::addPoint(defiGeometries* geom) {
  struct defiPoints* p;
  struct defiPoints* tp;
  int x, y;
  int i;

  p = (struct defiPoints*)malloc(sizeof(struct defiPoints));
  p->numPoints = geom->numPoints();
  p->x = (int*)malloc(sizeof(int)*p->numPoints);
  p->y = (int*)malloc(sizeof(int)*p->numPoints);
  for (i = 0; i < p->numPoints; i++) {
    geom->points(i, &x, &y);
    p->x[i] = x;
    p->y[i] = y;
    // for backward compatibility assign the first 2 points to xl, yl, xh & yh
    if (i == 0) {
      xl_ = x;
      yl_ = y;
    } else if (i == 1) {
      xh_ = x;
      yh_ = y;
    }
  }
  if (points_) {
     tp = points_;
     free((char*)(tp->x));
     free((char*)(tp->y));
     free((char*)(tp));
  }
  points_ = p;
}

int defiBox::xl() const {
  return xl_;
}


int defiBox::yl() const {
  return yl_;
}


int defiBox::xh() const {
  return xh_;
}


int defiBox::yh() const {
  return yh_;
}


struct defiPoints defiBox::getPoint() const {
  return *(points_);
}

void defiBox::print(FILE* f) const {
  fprintf(f, "Box %d,%d %d %d\n",
    xl(),
    yl(),
    xh(),
    yh());
}


END_LEFDEF_PARSER_NAMESPACE

