// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2015, Cadence Design Systems
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lex.h"
#include "lefiViaRule.hpp"
#include "lefiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

// *****************************************************************************
// lefiViaRuleLayer
// *****************************************************************************

lefiViaRuleLayer::lefiViaRuleLayer()
: name_(NULL),
  direction_(0),
  overhang1_(0.0),
  overhang2_(0.0),
  hasWidth_(0),
  hasResistance_(0),
  hasOverhang_(0),
  hasMetalOverhang_(0),
  hasSpacing_(0),
  hasRect_(0),
  widthMin_(0.0),
  widthMax_(0.0),
  overhang_(0.0),
  metalOverhang_(0.0),
  resistance_(0.0),
  spacingStepX_(0.0),
  spacingStepY_(0.0),
  xl_(0.0),
  yl_(0.0),
  xh_(0.0),
  yh_(0.0)
{
    Init();
}

void
lefiViaRuleLayer::Init()
{
    name_ = 0;
    overhang1_ = -1;
    overhang2_ = -1;
}

void
lefiViaRuleLayer::clearLayerOverhang()
{
    overhang1_ = -1;
    overhang2_ = -1;
}

void
lefiViaRuleLayer::setName(const char *name)
{
    int len = strlen(name) + 1;
    if (name_)
        lefFree(name_);
    name_ = (char*) lefMalloc(len);
    strcpy(name_, CASE(name));
    direction_ = '\0';
    // overhang1_ = -1;    already reset in clearLayerOverhang
    // overhang2_ = -1;
    hasWidth_ = 0;
    hasResistance_ = 0;
    hasOverhang_ = 0;
    hasMetalOverhang_ = 0;
    hasSpacing_ = 0;
    hasRect_ = 0;
}

void
lefiViaRuleLayer::Destroy()
{
    if (name_)
        lefFree(name_);
}

lefiViaRuleLayer::~lefiViaRuleLayer()
{
    // Destroy will be called explicitly
    // so do nothing here.
}

void
lefiViaRuleLayer::setHorizontal()
{
    direction_ = 'H';
}

void
lefiViaRuleLayer::setVertical()
{
    direction_ = 'V';
}

// 5.5
void
lefiViaRuleLayer::setEnclosure(double   overhang1,
                               double   overhang2)
{
    overhang1_ = overhang1;
    overhang2_ = overhang2;
}

void
lefiViaRuleLayer::setWidth(double   minW,
                           double   maxW)
{
    hasWidth_ = 1;
    widthMin_ = minW;
    widthMax_ = maxW;
}

void
lefiViaRuleLayer::setOverhang(double d)
{
    hasOverhang_ = 1;
    overhang_ = d;
}

// 5.6
void
lefiViaRuleLayer::setOverhangToEnclosure(double d)
{
    if ((overhang1_ != -1) && (overhang2_ != -1))
        return;                    // both overhang1_ & overhang2_ are set
    if (overhang1_ == -1)
        overhang1_ = d;      // set value to overhang1_
    else
        overhang2_ = d;      // overhang1_ already set, set to overhang2_
    return;
}

// 5.5
void
lefiViaRuleLayer::setMetalOverhang(double d)
{
    hasMetalOverhang_ = 1;
    metalOverhang_ = d;
}

void
lefiViaRuleLayer::setResistance(double d)
{
    hasResistance_ = 1;
    resistance_ = d;
}

void
lefiViaRuleLayer::setSpacing(double x,
                             double y)
{
    hasSpacing_ = 1;
    spacingStepX_ = x;
    spacingStepY_ = y;
}

void
lefiViaRuleLayer::setRect(double    xl,
                          double    yl,
                          double    xh,
                          double    yh)
{
    hasRect_ = 1;
    xl_ = xl;
    yl_ = yl;
    xh_ = xh;
    yh_ = yh;
}

int
lefiViaRuleLayer::hasRect() const
{
    return hasRect_;
}

int
lefiViaRuleLayer::hasDirection() const
{
    return direction_ ? 1 : 0;
}

// 5.5
int
lefiViaRuleLayer::hasEnclosure() const
{
    return overhang1_ == -1 ? 0 : 1;
}

int
lefiViaRuleLayer::hasWidth() const
{
    return hasWidth_;
}

int
lefiViaRuleLayer::hasResistance() const
{
    return hasResistance_;
}

int
lefiViaRuleLayer::hasOverhang() const
{
    return hasOverhang_;
}

int
lefiViaRuleLayer::hasMetalOverhang() const
{
    return hasMetalOverhang_;
}

int
lefiViaRuleLayer::hasSpacing() const
{
    return hasSpacing_;
}

char *
lefiViaRuleLayer::name() const
{
    return name_;
}

int
lefiViaRuleLayer::isHorizontal() const
{
    return direction_ == 'H' ? 1 : 0;
}

int
lefiViaRuleLayer::isVertical() const
{
    return direction_ == 'V' ? 1 : 0;
}

// 5.5
double
lefiViaRuleLayer::enclosureOverhang1() const
{
    return overhang1_;
}

// 5.5
double
lefiViaRuleLayer::enclosureOverhang2() const
{
    return overhang2_;
}

double
lefiViaRuleLayer::widthMin() const
{
    return widthMin_;
}

double
lefiViaRuleLayer::widthMax() const
{
    return widthMax_;
}

double
lefiViaRuleLayer::overhang() const
{
    return overhang_;
}

double
lefiViaRuleLayer::metalOverhang() const
{
    return metalOverhang_;
}

double
lefiViaRuleLayer::resistance() const
{
    return resistance_;
}

double
lefiViaRuleLayer::spacingStepX() const
{
    return spacingStepX_;
}

double
lefiViaRuleLayer::spacingStepY() const
{
    return spacingStepY_;
}

double
lefiViaRuleLayer::xl() const
{
    return xl_;
}

double
lefiViaRuleLayer::yl() const
{
    return yl_;
}

double
lefiViaRuleLayer::xh() const
{
    return xh_;
}

double
lefiViaRuleLayer::yh() const
{
    return yh_;
}

void
lefiViaRuleLayer::print(FILE *f) const
{
    fprintf(f, "  Layer %s", name_);

    if (isHorizontal())
        fprintf(f, " HORIZONTAL");
    if (isVertical())
        fprintf(f, " VERTICAL");
    fprintf(f, "\n");

    if (hasWidth())
        fprintf(f, "    WIDTH %g %g\n", widthMin(),
                widthMax());

    if (hasResistance())
        fprintf(f, "    RESISTANCE %g\n", resistance());

    if (hasOverhang())
        fprintf(f, "    OVERHANG %g\n", overhang());

    if (hasMetalOverhang())
        fprintf(f, "    METALOVERHANG %g\n",
                metalOverhang());

    if (hasSpacing())
        fprintf(f, "    SPACING %g %g\n",
                spacingStepX(),
                spacingStepY());

    if (hasRect())
        fprintf(f, "    RECT %g,%g %g,%g\n",
                xl(), yl(),
                xh(), yh());
}

// *****************************************************************************
// lefiViaRule
// *****************************************************************************

lefiViaRule::lefiViaRule()
: name_(NULL),
  nameSize_(0),
  hasGenerate_(0),
  hasDefault_(0),
  numLayers_(0),
  numVias_(0),
  viasAllocated_(0),
  vias_(NULL),
  numProps_(0),
  propsAllocated_(0),
  names_(NULL),
  values_(NULL),
  dvalues_(NULL),
  types_(NULL)
{
    Init();
}

void
lefiViaRule::Init()
{
    nameSize_ = 16;
    name_ = (char*) lefMalloc(16);
    viasAllocated_ = 2;
    vias_ = (char**) lefMalloc(sizeof(char*) * 2);
    layers_[0].Init();
    layers_[1].Init();
    layers_[2].Init();
    numLayers_ = 0;
    numVias_ = 0;
    numProps_ = 0;
    propsAllocated_ = 1;
    names_ = (char**) lefMalloc(sizeof(char*));
    values_ = (char**) lefMalloc(sizeof(char*));
    dvalues_ = (double*) lefMalloc(sizeof(double));
    types_ = (char*) lefMalloc(sizeof(char));
}

void
lefiViaRule::clear()
{
    int i;
    hasGenerate_ = 0;
    hasDefault_ = 0;
    for (i = 0; i < numProps_; i++) {
        lefFree(names_[i]);
        lefFree(values_[i]);
        dvalues_[i] = 0;
    }
    numProps_ = 0;
    numLayers_ = 0;
    for (i = 0; i < numVias_; i++) {
        lefFree(vias_[i]);
    }
    numVias_ = 0;
}

void
lefiViaRule::clearLayerOverhang()
{
    layers_[0].clearLayerOverhang();
    layers_[1].clearLayerOverhang();
}

void
lefiViaRule::setName(const char *name)
{
    int len = strlen(name) + 1;
    if (len > nameSize_) {
        lefFree(name_);
        name_ = (char*) lefMalloc(len);
        nameSize_ = len;
    }
    strcpy(name_, CASE(name));
    clear();
}

void
lefiViaRule::Destroy()
{
    clear();
    lefFree(name_);
    lefFree((char*) (vias_));
    lefFree((char*) (names_));
    lefFree((char*) (values_));
    lefFree((char*) (dvalues_));
    lefFree((char*) (types_));
    layers_[0].Destroy();
    layers_[1].Destroy();
    layers_[2].Destroy();
}

lefiViaRule::~lefiViaRule()
{
    Destroy();
}

void
lefiViaRule::setGenerate()
{
    hasGenerate_ = 1;
}

void
lefiViaRule::setDefault()
{
    hasDefault_ = 1;
}

void
lefiViaRule::addViaName(const char *name)
{
    // Add one of possibly many via names
    int len = strlen(name) + 1;
    if (numVias_ == viasAllocated_) {
        int     i;
        char    **nn;
        if (viasAllocated_ == 0)
            viasAllocated_ = 2;
        else
            viasAllocated_ *= 2;
        nn = (char**) lefMalloc(sizeof(char*) * viasAllocated_);
        for (i = 0; i < numVias_; i++)
            nn[i] = vias_[i];
        lefFree((char*) (vias_));
        vias_ = nn;
    }
    vias_[numVias_] = (char*) lefMalloc(len);
    strcpy(vias_[numVias_], CASE(name));
    numVias_ += 1;
}

void
lefiViaRule::setRect(double xl,
                     double yl,
                     double xh,
                     double yh)
{
    layers_[numLayers_ - 1].setRect(xl, yl, xh, yh);
}

void
lefiViaRule::setSpacing(double  x,
                        double  y)
{
    layers_[numLayers_ - 1].setSpacing(x, y);
}

void
lefiViaRule::setWidth(double    x,
                      double    y)
{
    layers_[numLayers_ - 1].setWidth(x, y);
}

void
lefiViaRule::setResistance(double d)
{
    layers_[numLayers_ - 1].setResistance(d);
}

void
lefiViaRule::setOverhang(double d)
{
    layers_[numLayers_ - 1].setOverhang(d);
}

// 5.6, try to set value to layers_[0] & layers_[1]
void
lefiViaRule::setOverhangToEnclosure(double d)
{
    layers_[0].setOverhangToEnclosure(d);
    layers_[1].setOverhangToEnclosure(d);
}

void
lefiViaRule::setMetalOverhang(double d)
{
    layers_[numLayers_ - 1].setMetalOverhang(d);
}

void
lefiViaRule::setVertical()
{
    layers_[numLayers_ - 1].setVertical();
}

void
lefiViaRule::setHorizontal()
{
    layers_[numLayers_ - 1].setHorizontal();
}

void
lefiViaRule::setEnclosure(double    overhang1,
                          double    overhang2)
{
    layers_[numLayers_ - 1].setEnclosure(overhang1,
                                         overhang2);
}

void
lefiViaRule::setLayer(const char *name)
{
    if (numLayers_ == 3) {
        lefiError(0, 1430, "ERROR (LEFPARS-1430): too many via rule layers");
        return;
    }
    // This routine sets and creates the active layer.
    layers_[numLayers_].setName(name);
    numLayers_ += 1;
}

int
lefiViaRule::hasGenerate() const
{
    return hasGenerate_;
}

int
lefiViaRule::hasDefault() const
{
    return hasDefault_;
}

int
lefiViaRule::numLayers() const
{
    // There are 2 or 3 layers in a rule.
    // numLayers() tells how many.
    // If a third layer exists then it is the cut layer.
    return numLayers_;
}

lefiViaRuleLayer *
lefiViaRule::layer(int index) const
{
    if (index < 0 || index > 2)
        return 0;
    return (lefiViaRuleLayer*) &(layers_[index]);
}

char *
lefiViaRule::name() const
{
    return name_;
}

void
lefiViaRule::print(FILE *f) const
{
    int i;
    fprintf(f, "VIA RULE %s", name());
    if (hasGenerate())
        fprintf(f, " GENERATE");
    fprintf(f, "\n");

    for (i = 0; i < numLayers(); i++) {
        layers_[i].print(f);
    }

    for (i = 0; i < numVias(); i++) {
        fprintf(f, "  Via %s\n", viaName(i));
    }
}

int
lefiViaRule::numVias() const
{
    return numVias_;
}

char *
lefiViaRule::viaName(int index) const
{
    if (index < 0 || index >= numVias_)
        return 0;
    return vias_[index];
}

int
lefiViaRule::numProps() const
{
    return numProps_;
}

void
lefiViaRule::addProp(const char *name,
                     const char *value,
                     const char type)
{
    int len = strlen(name) + 1;
    if (numProps_ == propsAllocated_) {
        int     i;
        int     max;
        int     lim;
        char    **nn;
        char    **nv;
        double  *nd;
        char    *nt;

        if (propsAllocated_ == 0)
            propsAllocated_ = 1;    // initialize propsAllocated_
        max = propsAllocated_ *= 2;
        lim = numProps_;
        nn = (char**) lefMalloc(sizeof(char*) * max);
        nv = (char**) lefMalloc(sizeof(char*) * max);
        nd = (double*) lefMalloc(sizeof(double) * max);
        nt = (char*) lefMalloc(sizeof(char) * max);
        for (i = 0; i < lim; i++) {
            nn[i] = names_[i];
            nv[i] = values_[i];
            nd[i] = dvalues_[i];
            nt[i] = types_[i];
        }
        lefFree((char*) (names_));
        lefFree((char*) (values_));
        lefFree((char*) (dvalues_));
        lefFree((char*) (types_));
        names_ = nn;
        values_ = nv;
        dvalues_ = nd;
        types_ = nt;
    }
    names_[numProps_] = (char*) lefMalloc(sizeof(char) * len);
    strcpy(names_[numProps_], name);
    len = strlen(value) + 1;
    values_[numProps_] = (char*) lefMalloc(sizeof(char) * len);
    strcpy(values_[numProps_], value);
    dvalues_[numProps_] = 0;
    types_[numProps_] = type;
    numProps_ += 1;
}

void
lefiViaRule::addNumProp(const char      *name,
                        const double    d,
                        const char      *value,
                        const char      type)
{
    int len = strlen(name) + 1;
    if (numProps_ == propsAllocated_) {
        int     i;
        int     max;
        int     lim;
        char    **nn;
        char    **nv;
        double  *nd;
        char    *nt;

        if (propsAllocated_ == 0)
            propsAllocated_ = 1;    // initialize propsAllocated_
        max = propsAllocated_ *= 2;
        lim = numProps_;
        nn = (char**) lefMalloc(sizeof(char*) * max);
        nv = (char**) lefMalloc(sizeof(char*) * max);
        nd = (double*) lefMalloc(sizeof(double) * max);
        nt = (char*) lefMalloc(sizeof(char) * max);
        for (i = 0; i < lim; i++) {
            nn[i] = names_[i];
            nv[i] = values_[i];
            nd[i] = dvalues_[i];
            nt[i] = types_[i];
        }
        lefFree((char*) (names_));
        lefFree((char*) (values_));
        lefFree((char*) (dvalues_));
        lefFree((char*) (types_));
        names_ = nn;
        values_ = nv;
        dvalues_ = nd;
        types_ = nt;
    }
    names_[numProps_] = (char*) lefMalloc(sizeof(char) * len);
    strcpy(names_[numProps_], name);
    len = strlen(value) + 1;
    values_[numProps_] = (char*) lefMalloc(sizeof(char) * len);
    strcpy(values_[numProps_], value);
    dvalues_[numProps_] = d;
    types_[numProps_] = type;
    numProps_ += 1;
}

const char *
lefiViaRule::propName(int i) const
{
    char msg[160];
    if (i < 0 || i >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1431): The index number %d given for the VIARULE PROPERTY is invalid.\nValid index is from 0 to %d", i, numProps_);
        lefiError(0, 1431, msg);
        return 0;
    }
    return names_[i];
}

const char *
lefiViaRule::propValue(int i) const
{
    char msg[160];
    if (i < 0 || i >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1431): The index number %d given for the VIARULE PROPERTY is invalid.\nValid index is from 0 to %d", i, numProps_);
        lefiError(0, 1431, msg);
        return 0;
    }
    return values_[i];
}

double
lefiViaRule::propNumber(int i) const
{
    char msg[160];
    if (i < 0 || i >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1431): The index number %d given for the VIARULE PROPERTY is invalid.\nValid index is from 0 to %d", i, numProps_);
        lefiError(0, 1431, msg);
        return 0;
    }
    return dvalues_[i];
}

char
lefiViaRule::propType(int i) const
{
    char msg[160];
    if (i < 0 || i >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1431): The index number %d given for the VIARULE PROPERTY is invalid.\nValid index is from 0 to %d", i, numProps_);
        lefiError(0, 1431, msg);
        return 0;
    }
    return types_[i];
}

int
lefiViaRule::propIsNumber(int i) const
{
    char msg[160];
    if (i < 0 || i >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1431): The index number %d given for the VIARULE PROPERTY is invalid.\nValid index is from 0 to %d", i, numProps_);
        lefiError(0, 1431, msg);
        return 0;
    }
    return dvalues_[i] ? 1 : 0;
}

int
lefiViaRule::propIsString(int i) const
{
    char msg[160];
    if (i < 0 || i >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1431): The index number %d given for the VIARULE PROPERTY is invalid.\nValid index is from 0 to %d", i, numProps_);
        lefiError(0, 1431, msg);
        return 0;
    }
    return dvalues_[i] ? 0 : 1;
}
END_LEFDEF_PARSER_NAMESPACE

