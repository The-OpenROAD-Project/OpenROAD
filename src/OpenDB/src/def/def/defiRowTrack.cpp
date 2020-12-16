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
#include "defiRowTrack.hpp"
#include "defiDebug.hpp"
#include "lex.h"
#include "defiUtil.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

//////////////////////////////////////////////
//////////////////////////////////////////////
//
//   defiRow
//
//////////////////////////////////////////////
//////////////////////////////////////////////


defiRow::defiRow(defrData *data)
 : defData(data)
{
  Init();
}


void defiRow::Init() {
  nameLength_ = 0;
  name_ = 0;
  macroLength_ = 0;
  macro_ = 0;
  orient_ = 0;
  x_ = 0.0;
  y_ = 0.0;
  xStep_ = 0.0;
  yStep_ = 0.0;
  xNum_ = 0.0;
  yNum_ = 0.0;
  hasDo_ = 0;
  hasDoStep_ = 0;
  numProps_ = 0;
  propsAllocated_ = 2;
  propNames_ = (char**)malloc(sizeof(char*)*2);
  propValues_ = (char**)malloc(sizeof(char*)*2);
  propDValues_ = (double*)malloc(sizeof(double)*2);
  propTypes_ = (char*)malloc(sizeof(char)*2);
}


DEF_COPY_CONSTRUCTOR_C( defiRow ) {

  this->Init();
    DEF_COPY_FUNC( nameLength_ );
    DEF_MALLOC_FUNC( name_, char, sizeof(char) * (strlen(prev.name_) +1));
    DEF_COPY_FUNC( macroLength_ );
    DEF_MALLOC_FUNC( macro_, char, sizeof(char) * (strlen(prev.macro_) +1));
    DEF_COPY_FUNC( x_ );
    DEF_COPY_FUNC( y_ );
    DEF_COPY_FUNC( xNum_ );
    DEF_COPY_FUNC( yNum_ );
    DEF_COPY_FUNC( orient_ );
    DEF_COPY_FUNC( xStep_ );
    DEF_COPY_FUNC( yStep_ );
    DEF_COPY_FUNC( hasDo_ );
    DEF_COPY_FUNC( hasDoStep_ );
    DEF_COPY_FUNC( numProps_ );
    DEF_COPY_FUNC( propsAllocated_ );
    DEF_MALLOC_FUNC_FOR_2D_STR( propNames_, numProps_);
    DEF_MALLOC_FUNC_FOR_2D_STR( propValues_, numProps_);
    DEF_MALLOC_FUNC( propDValues_, double, sizeof(double) * numProps_);
    DEF_MALLOC_FUNC( propTypes_, char, sizeof(char) * numProps_ );

}

DEF_ASSIGN_OPERATOR_C( defiRow ) {
    CHECK_SELF_ASSIGN
  this->Init();
    DEF_COPY_FUNC( nameLength_ );
    DEF_MALLOC_FUNC( name_, char, sizeof(char) * (strlen(prev.name_) +1));
    DEF_COPY_FUNC( macroLength_ );
    DEF_MALLOC_FUNC( macro_, char, sizeof(char) * (strlen(prev.macro_) +1));
    DEF_COPY_FUNC( x_ );
    DEF_COPY_FUNC( y_ );
    DEF_COPY_FUNC( xNum_ );
    DEF_COPY_FUNC( yNum_ );
    DEF_COPY_FUNC( orient_ );
    DEF_COPY_FUNC( xStep_ );
    DEF_COPY_FUNC( yStep_ );
    DEF_COPY_FUNC( hasDo_ );
    DEF_COPY_FUNC( hasDoStep_ );
    DEF_COPY_FUNC( numProps_ );
    DEF_COPY_FUNC( propsAllocated_ );
    DEF_MALLOC_FUNC_FOR_2D_STR( propNames_, numProps_);
    DEF_MALLOC_FUNC_FOR_2D_STR( propValues_, numProps_);
    DEF_MALLOC_FUNC( propDValues_, double, sizeof(double) * numProps_);
    DEF_MALLOC_FUNC( propTypes_, char, sizeof(char) * numProps_ );
    return *this;
}

defiRow::~defiRow() {
  Destroy();
}


void defiRow::Destroy() {
  clear();
  if (name_) free(name_);
  if (macro_) free(macro_);
  free((char*)(propNames_));
  free((char*)(propValues_));
  free((char*)(propDValues_));
  free((char*)(propTypes_));
}


void defiRow::clear() {
  int i;
  for (i = 0; i < numProps_; i++) {
    free(propNames_[i]);
    free(propValues_[i]);
    propDValues_[i] = 0;
  }
  hasDo_ = 0;
  hasDoStep_ = 0;
  numProps_ = 0;
}


void defiRow::setup(const char* name, const char* macro, double x, double y,
		 int orient) {
  int len = strlen(name) + 1;

  clear();

  if (len > nameLength_) {
    if (name_) free(name_);
    nameLength_ = len;
    name_ = (char*)malloc(len);
  }
  strcpy(name_, defData->DEFCASE(name));

  len = strlen(macro) + 1;
  if (len > macroLength_) {
    if (macro_) free(macro_);
    macroLength_ = len;
    macro_ = (char*)malloc(len);
  }
  strcpy(macro_, defData->DEFCASE(macro));

  x_ = x;
  y_ = y;
  xStep_ = 0.0;
  yStep_ = 0.0;
  xNum_ = 0.0;
  yNum_ = 0.0;
  orient_ = orient;

}


void defiRow::setDo(double x_num, double y_num,
		    double x_step, double y_step) {
  xStep_ = x_step;
  yStep_ = y_step;
  xNum_ = x_num;
  yNum_ = y_num;
  hasDo_ = 1;
}


void defiRow::setHasDoStep() {
  hasDoStep_ = 1;
}


void defiRow::addProperty(const char* name, const char* value, const char type) 
{
  int len;
  if (numProps_ == propsAllocated_) {
    int i;
    char**  nn;
    char**  nv;
    double* nd;
    char*   nt;
    propsAllocated_ *= 2;
    nn = (char**)malloc(sizeof(char*)*propsAllocated_);
    nv = (char**)malloc(sizeof(char*)*propsAllocated_);
    nd = (double*)malloc(sizeof(double)*propsAllocated_);
    nt = (char*)malloc(sizeof(char)*propsAllocated_);
    for (i = 0; i < numProps_; i++) {
      nn[i] = propNames_[i];
      nv[i] = propValues_[i];
      nd[i] = propDValues_[i];
      nt[i] = propTypes_[i];
    }
    free((char*)(propNames_));
    free((char*)(propValues_));
    free((char*)(propDValues_));
    free((char*)(propTypes_));
    propNames_ = nn;
    propValues_ = nv;
    propDValues_ = nd;
    propTypes_ = nt;
  }
  len = strlen(name) + 1;
  propNames_[numProps_] = (char*)malloc(len);
  strcpy(propNames_[numProps_], defData->DEFCASE(name));
  len = strlen(value) + 1;
  propValues_[numProps_] = (char*)malloc(len);
  strcpy(propValues_[numProps_], defData->DEFCASE(value));
  propDValues_[numProps_] = 0;
  propTypes_[numProps_] = type;
  numProps_ += 1;
}


void defiRow::addNumProperty(const char* name, const double d,
                             const char* value, const char type) 
{
  int len;
  if (numProps_ == propsAllocated_) {
    int i;
    char** nn;
    char** nv;
    double* nd;
    char*  nt;
    propsAllocated_ *= 2;
    nn = (char**)malloc(sizeof(char*)*propsAllocated_);
    nv = (char**)malloc(sizeof(char*)*propsAllocated_);
    nd = (double*)malloc(sizeof(double)*propsAllocated_);
    nt = (char*)malloc(sizeof(char)*propsAllocated_);
    for (i = 0; i < numProps_; i++) {
      nn[i] = propNames_[i];
      nv[i] = propValues_[i];
      nd[i] = propDValues_[i];
      nt[i] = propTypes_[i];
    }
    free((char*)(propNames_));
    free((char*)(propValues_));
    free((char*)(propDValues_));
    free((char*)(propTypes_));
    propNames_ = nn;
    propValues_ = nv;
    propDValues_ = nd;
    propTypes_ = nt;
  }
  len = strlen(name) + 1;
  propNames_[numProps_] = (char*)malloc(len);
  strcpy(propNames_[numProps_], defData->DEFCASE(name));
  len = strlen(value) + 1;
  propValues_[numProps_] = (char*)malloc(len);
  strcpy(propValues_[numProps_], defData->DEFCASE(value));
  propDValues_[numProps_] = d;
  propTypes_[numProps_] = type;
  numProps_ += 1;
}


int defiRow::numProps() const {
  return numProps_;
}


const char* defiRow::propName(int index) const {
  char msg[256];
  if (index < 0 || index >= numProps_) {
     sprintf (msg, "ERROR (DEFPARS-6140): The index number %d specified for the VIA LAYER RECTANGLE is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numProps_);
     defiError(0, 6140, msg, defData);
     return 0;
  }
  return propNames_[index];
}


const char* defiRow::propValue(int index) const {
  char msg[256];
  if (index < 0 || index >= numProps_) {
     sprintf (msg, "ERROR (DEFPARS-6140): The index number %d specified for the VIA LAYER RECTANGLE is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numProps_);
     defiError(0, 6140, msg, defData);
     return 0;
  }
  return propValues_[index];
}

double defiRow::propNumber(int index) const {
  char msg[256];
  if (index < 0 || index >= numProps_) {
     sprintf (msg, "ERROR (DEFPARS-6140): The index number %d specified for the VIA LAYER RECTANGLE is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numProps_);
     defiError(0, 6140, msg, defData);
     return 0;
  }
  return propDValues_[index];
}

char defiRow::propType(int index) const {
  char msg[256];
  if (index < 0 || index >= numProps_) {
     sprintf (msg, "ERROR (DEFPARS-6140): The index number %d specified for the VIA LAYER RECTANGLE is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numProps_);
     defiError(0, 6140, msg, defData);
     return 0;
  }
  return propTypes_[index];
}

int defiRow::propIsNumber(int index) const {
  char msg[256];
  if (index < 0 || index >= numProps_) {
     sprintf (msg, "ERROR (DEFPARS-6140): The index number %d specified for the VIA LAYER RECTANGLE is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numProps_);
     defiError(0, 6140, msg, defData);
     return 0;
  } 
  return propDValues_[index] ? 1 : 0;
}

int defiRow::propIsString(int index) const {
  char msg[256];
  if (index < 0 || index >= numProps_) {
     sprintf (msg, "ERROR (DEFPARS-6140): The index number %d specified for the VIA LAYER RECTANGLE is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numProps_);
     defiError(0, 6140, msg, defData);
     return 0;
  } 
  return propDValues_[index] ? 0 : 1;
}

const char* defiRow::name() const {
  return name_;
}


const char* defiRow::macro() const {
  return macro_;
}


double defiRow::x() const {
  return x_;
}


double defiRow::y() const {
  return y_;
}


double defiRow::xNum() const {
  return xNum_;
}


double defiRow::yNum() const {
  return yNum_;
}


int defiRow::orient() const {
  return orient_;
}


const char* defiRow::orientStr() const {
  return (defiOrientStr(orient_));
}


int defiRow::hasDo() const {
  return hasDo_;
}


int defiRow::hasDoStep() const {
  return hasDoStep_;
}


double defiRow::xStep() const {
  return xStep_;
}


double defiRow::yStep() const {
  return yStep_;
}


void defiRow::print(FILE* f) const {
  fprintf(f, "Row '%s' '%s' %g,%g  orient %s\n",
      name(), macro(),
      x(), y(), orientStr());
  fprintf(f, "  DO X %g STEP %g\n", xNum(),
      xStep());
  fprintf(f, "  DO Y %g STEP %g\n", yNum(),
      yStep());
}


//////////////////////////////////////////////
//////////////////////////////////////////////
//
//   defiTrack
//
//////////////////////////////////////////////
//////////////////////////////////////////////


defiTrack::defiTrack(defrData *data)
 : defData(data)
{
  Init();
}


void defiTrack::Init() {
  macro_ = 0;
  macroLength_ = 0;
  x_ = 0.0;
  xNum_ = 0.0;
  xStep_ = 0.0;
  layersLength_ = 0;
  numLayers_ = 0;
  layers_ = 0;
  firstTrackMask_=0;
  samemask_ = 0;
}


DEF_COPY_CONSTRUCTOR_C( defiTrack ) {
    this->Init();
    DEF_COPY_FUNC( macroLength_ );
    DEF_MALLOC_FUNC( macro_, char, sizeof(char) * (strlen(prev.macro_) +1));
    DEF_COPY_FUNC( x_ );
    DEF_COPY_FUNC( xNum_ );
    DEF_COPY_FUNC( xStep_ );
    DEF_COPY_FUNC( layersLength_ );
    DEF_COPY_FUNC( numLayers_ );
    DEF_MALLOC_FUNC_FOR_2D_STR( layers_, numLayers_ );
    DEF_COPY_FUNC( firstTrackMask_ );
    DEF_COPY_FUNC( samemask_ );

}

DEF_ASSIGN_OPERATOR_C( defiTrack ) {
    CHECK_SELF_ASSIGN
    this->Init();
    DEF_COPY_FUNC( macroLength_ );
    DEF_MALLOC_FUNC( macro_, char, sizeof(char) * (strlen(prev.macro_) +1));
    DEF_COPY_FUNC( x_ );
    DEF_COPY_FUNC( xNum_ );
    DEF_COPY_FUNC( xStep_ );
    DEF_COPY_FUNC( layersLength_ );
    DEF_COPY_FUNC( numLayers_ );
    DEF_MALLOC_FUNC_FOR_2D_STR( layers_, numLayers_ );
    DEF_COPY_FUNC( firstTrackMask_ );
    DEF_COPY_FUNC( samemask_ );
    return *this;
}

defiTrack::~defiTrack() {
  Destroy();
}


void defiTrack::Destroy() {
  int i;

  if (macro_) free(macro_);

  if (layers_) {
    for (i = 0; i < numLayers_; i++)
      if (layers_[i]) free(layers_[i]);
    free((char*)(layers_));
  }
}


void defiTrack::setup(const char* macro) {
  int i;
  int len = strlen(macro) + 1;

  if (len > macroLength_) {
    if (macro_) free(macro_);
    macroLength_ = len;
    macro_ = (char*)malloc(len);
  }
  strcpy(macro_, defData->DEFCASE(macro));

  if (layers_) {
    for (i = 0; i < numLayers_; i++)
      if (layers_[i]) {
        free(layers_[i]);
        layers_[i] = 0;
      }
  }
  numLayers_ = 0;
  x_ = 0.0;
  xStep_ = 0.0;
  xNum_ = 0.0;

  firstTrackMask_=0;
  samemask_=0;
}


void defiTrack::setDo(double x, double x_num, double x_step) {
  x_ = x;
  xStep_ = x_step;
  xNum_ = x_num;
}


void defiTrack::addLayer(const char* layer) {
  char* l;
  int len;

  if (numLayers_ >= layersLength_) {
    int i;
    char** newl;
    layersLength_ = layersLength_ ? 2 * layersLength_ : 8;
    newl = (char**)malloc(layersLength_* sizeof(char*));
    for (i = 0; i < numLayers_; i++)
      newl[i] = layers_[i];
    if (layers_) free((char*)(layers_));
    layers_ = newl;
  }

  len = strlen(layer) + 1;
  l = (char*)malloc(len);
  strcpy(l, defData->DEFCASE(layer));
  layers_[numLayers_++] = l;
}

void defiTrack::addMask(int colorMask, int sameMask) {
   samemask_=sameMask;
   firstTrackMask_= colorMask;
}


const char* defiTrack::macro() const {
  return macro_;
}


double defiTrack::x() const {
  return x_;
}


double defiTrack::xNum() const {
  return xNum_;
}


double defiTrack::xStep() const {
  return xStep_;
}


int defiTrack::numLayers() const {
  return numLayers_;
}


const char* defiTrack::layer(int index) const {
  if (index >= 0 && index < numLayers_) {
    return layers_[index];
  }

  return 0;
}

int defiTrack::firstTrackMask() const {
    return firstTrackMask_;
}

int defiTrack::sameMask() const {
    return samemask_;
}

void defiTrack::print(FILE* f) const {
  int i;

  fprintf(f, "Track '%s'\n", macro());
  fprintf(f, "  DO %g %g STEP %g\n",
      x(),
      xNum(),
      xStep());
  fprintf(f, "  %d layers ", numLayers());
  for (i = 0; i < numLayers(); i++) {
    fprintf(f, " '%s'", layer(i));
  }
  fprintf(f, "\n");
}


//////////////////////////////////////////////
//////////////////////////////////////////////
//
//   defiGcellGrid
//
//////////////////////////////////////////////
//////////////////////////////////////////////


defiGcellGrid::defiGcellGrid(defrData *data)
 : defData(data)
{
  Init();
}


void defiGcellGrid::Init() {
  macro_ = 0;
  macroLength_ = 0;
  x_ = 0;
  xNum_ = 0;
  xStep_ = 0;
}

DEF_COPY_CONSTRUCTOR_C( defiGcellGrid ) {
    this->Init();
    DEF_COPY_FUNC( macroLength_ );
    DEF_MALLOC_FUNC( macro_, char, sizeof(char) * (strlen(prev.macro_) +1));
    DEF_COPY_FUNC( x_ );
    DEF_COPY_FUNC( xNum_ );
    DEF_COPY_FUNC( xStep_ );
}

DEF_ASSIGN_OPERATOR_C( defiGcellGrid ) {
    CHECK_SELF_ASSIGN
    this->Init();
    DEF_COPY_FUNC( macroLength_ );
    DEF_MALLOC_FUNC( macro_, char, sizeof(char) * (strlen(prev.macro_) +1));
    DEF_COPY_FUNC( x_ );
    DEF_COPY_FUNC( xNum_ );
    DEF_COPY_FUNC( xStep_ );
    return *this;
}

defiGcellGrid::~defiGcellGrid() {
  Destroy();
}


void defiGcellGrid::Destroy() {
  if (macro_) free(macro_);
}


void defiGcellGrid::setup(const char* macro, int x, int xNum, double xStep) {
  int len = strlen(macro) + 1;
  if (len > macroLength_) {
    if (macro_) free(macro_);
    macroLength_ = len;
    macro_ = (char*)malloc(len);
  }
  strcpy(macro_, defData->DEFCASE(macro));

  x_ = x;
  xNum_ = xNum;
  xStep_ = xStep;
}


int defiGcellGrid::x() const {
  return x_;
}


int defiGcellGrid::xNum() const {
  return xNum_;
}


double defiGcellGrid::xStep() const {
  return xStep_;
}


const char* defiGcellGrid::macro() const {
  return macro_;
}


void defiGcellGrid::print(FILE* f) const {
  fprintf(f, "GcellGrid '%s'\n", macro());
  fprintf(f, "  DO %d %d STEP %5.1f\n",
      x(),
      xNum(),
      xStep());
}


END_LEFDEF_PARSER_NAMESPACE

