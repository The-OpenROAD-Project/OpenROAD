// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2013, Cadence Design Systems
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

#include <stdlib.h>
#include <string.h>
#include "lex.h"
#include "lefiNonDefault.hpp"
#include "lefiDebug.hpp"
#include "lefrCallBacks.hpp"

#include "lefrData.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

// 6/16/2000 - Wanda da Rosa
// Make these variables in globals.  Can't use those defined
// in the class because it generates warning when it casts.
// Can't assign for example lefrViaCbkFnType to oldViaCbk_
// in the class because it requires to include lefrReader.hpp
// in lefiNonDefault.hpp.  But in lefrReader.hpp, it requires
// include lefiNonDefault.hpp, this creates a loop and is
// problematic...

// *****************************************************************************
// lefiNonDefault
// *****************************************************************************

lefiNonDefault::lefiNonDefault()
: nameSize_(0),
  name_(NULL),
  numLayers_(0),
  layersAllocated_(0),
  layerName_(NULL),
  width_(NULL),
  spacing_(NULL),
  wireExtension_(NULL),
  hasWidth_(NULL),
  hasSpacing_(NULL),
  hasWireExtension_(NULL),
  resistance_(NULL),
  capacitance_(NULL),
  edgeCap_(NULL),
  hasResistance_(NULL),
  hasCapacitance_(NULL),
  hasEdgeCap_(NULL),
  diagWidth_(NULL),
  hasDiagWidth_(NULL),
  numVias_(0),
  allocatedVias_(0),
  viaRules_(NULL),
  numSpacing_(0),
  allocatedSpacing_(0),
  spacingRules_(NULL),
  hardSpacing_(0),
  numUseVias_(0),
  allocatedUseVias_(0),
  useViaName_(NULL),
  numUseViaRules_(0),
  allocatedUseViaRules_(0),
  useViaRuleName_(NULL),
  numMinCuts_(0),
  allocatedMinCuts_(0),
  cutLayerName_(NULL),
  numCuts_(NULL),
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
lefiNonDefault::Init()
{
    nameSize_ = 16;
    name_ = (char*) lefMalloc(16);

    layersAllocated_ = 2;
    numLayers_ = 0;
    layerName_ = (char**) lefMalloc(sizeof(char*) * 2);
    width_ = (double*) lefMalloc(sizeof(double) * 2);
    diagWidth_ = (double*) lefMalloc(sizeof(double) * 2);
    spacing_ = (double*) lefMalloc(sizeof(double) * 2);
    wireExtension_ = (double*) lefMalloc(sizeof(double) * 2);
    resistance_ = (double*) lefMalloc(sizeof(double) * 2);
    capacitance_ = (double*) lefMalloc(sizeof(double) * 2);
    edgeCap_ = (double*) lefMalloc(sizeof(double) * 2);
    hasWidth_ = (char*) lefMalloc(sizeof(char) * 2);
    hasDiagWidth_ = (char*) lefMalloc(sizeof(char) * 2);
    hasSpacing_ = (char*) lefMalloc(sizeof(char) * 2);
    hasWireExtension_ = (char*) lefMalloc(sizeof(char) * 2);
    hasResistance_ = (char*) lefMalloc(sizeof(char) * 2);
    hasCapacitance_ = (char*) lefMalloc(sizeof(char) * 2);
    hasEdgeCap_ = (char*) lefMalloc(sizeof(char) * 2);

    allocatedVias_ = 2;
    numVias_ = 0;
    viaRules_ = (lefiVia**) lefMalloc(sizeof(lefiVia*) * 2);

    allocatedSpacing_ = 2;
    numSpacing_ = 0;
    spacingRules_ = (lefiSpacing**) lefMalloc(sizeof(lefiSpacing*) * 2);

    numProps_ = 0;
    propsAllocated_ = 1;
    names_ = (char**) lefMalloc(sizeof(char*));
    values_ = (char**) lefMalloc(sizeof(char*));
    dvalues_ = (double*) lefMalloc(sizeof(double));
    types_ = (char*) lefMalloc(sizeof(char));

    hardSpacing_ = 0;
    numUseVias_ = 0;             // Won't be allocated until they are used
    allocatedUseVias_ = 0;
    numUseViaRules_ = 0;
    allocatedUseViaRules_ = 0;
    numMinCuts_ = 0;
    allocatedMinCuts_ = 0;
}

void
lefiNonDefault::Destroy()
{
    clear();

    lefFree(name_);

    lefFree((char*) (layerName_));
    lefFree((char*) (width_));
    lefFree((char*) (diagWidth_));
    lefFree((char*) (spacing_));
    lefFree((char*) (wireExtension_));
    lefFree((char*) (resistance_));
    lefFree((char*) (capacitance_));
    lefFree((char*) (edgeCap_));
    lefFree(hasWidth_);
    lefFree(hasDiagWidth_);
    lefFree(hasSpacing_);
    lefFree(hasWireExtension_);
    lefFree(hasResistance_);
    lefFree(hasCapacitance_);
    lefFree(hasEdgeCap_);

    lefFree((char*) (viaRules_));

    lefFree((char*) (spacingRules_));
    lefFree((char*) (names_));
    lefFree((char*) (values_));
    lefFree((char*) (dvalues_));
    lefFree((char*) (types_));
    if (allocatedUseVias_)
        lefFree((char*) (useViaName_));
    if (allocatedUseViaRules_)
        lefFree((char*) (useViaRuleName_));
    if (allocatedMinCuts_) {
        lefFree((char*) (cutLayerName_));
        lefFree((char*) (numCuts_));
    }
    allocatedUseVias_ = 0;
    allocatedUseViaRules_ = 0;
    allocatedMinCuts_ = 0;
}

lefiNonDefault::~lefiNonDefault()
{
    Destroy();
}

void
lefiNonDefault::clear()
{
    int         i;
    lefiSpacing *sr;
    lefiVia     *vr;

    for (i = 0; i < numProps_; i++) {
        lefFree(names_[i]);
        lefFree(values_[i]);
        dvalues_[i] = 0;
    }
    numProps_ = 0;
    for (i = 0; i < numLayers_; i++) {
        lefFree(layerName_[i]);
        layerName_[i] = 0;
    }
    numLayers_ = 0;
    for (i = 0; i < numVias_; i++) {
        vr = viaRules_[i];
        vr->Destroy();
        lefFree((char*) (viaRules_[i]));
        viaRules_[i] = 0;
    }
    numVias_ = 0;
    for (i = 0; i < numSpacing_; i++) {
        sr = spacingRules_[i];
        sr->Destroy();
        lefFree((char*) (spacingRules_[i]));
        spacingRules_[i] = 0;
    }
    numSpacing_ = 0;

    hardSpacing_ = 0;
    for (i = 0; i < numUseVias_; i++) {
        lefFree((char*) (useViaName_[i]));
    }
    numUseVias_ = 0;
    for (i = 0; i < numUseViaRules_; i++) {
        lefFree((char*) (useViaRuleName_[i]));
    }
    numUseViaRules_ = 0;
    for (i = 0; i < numMinCuts_; i++) {
        lefFree((char*) (cutLayerName_[i]));
    }
    numMinCuts_ = 0;
}

void
lefiNonDefault::addViaRule(lefiVia *v)
{
    if (numVias_ == allocatedVias_) {
        int     i;
        lefiVia **nv;

        if (allocatedVias_ == 0)
            allocatedVias_ = 2;
        else
            allocatedVias_ *= 2;
        nv = (lefiVia**) lefMalloc(sizeof(lefiVia*) * allocatedVias_);
        for (i = 0; i < numVias_; i++) {
            nv[i] = viaRules_[i];
        }
        lefFree((char*) (viaRules_));
        viaRules_ = nv;
    }
    viaRules_[numVias_++] = v->clone();
}

void
lefiNonDefault::addSpacingRule(lefiSpacing *s)
{
    if (numSpacing_ == allocatedSpacing_) {
        int         i;
        lefiSpacing **ns;

        if (allocatedSpacing_ == 0)
            allocatedSpacing_ = 2;
        else
            allocatedSpacing_ *= 2;
        ns = (lefiSpacing**) lefMalloc(sizeof(lefiSpacing*) *
                                       allocatedSpacing_);
        for (i = 0; i < numSpacing_; i++) {
            ns[i] = spacingRules_[i];
        }
        lefFree((char*) (spacingRules_));
        spacingRules_ = ns;
    }
    spacingRules_[numSpacing_++] = s->clone();
}

void
lefiNonDefault::setName(const char *name)
{
    int len = strlen(name) + 1;
    clear();

    // Use our callback functions because a via and spacing
    // rule is really part of the non default section.
    // oldViaCbk_ = (void*)lefrViaCbk;
    // oldSpacingCbk_ = (void*)lefrSpacingCbk;
    // oldSpacingBeginCbk_ = (void*)lefrSpacingBeginCbk;
    // oldSpacingEndCbk_ = (void*)lefrSpacingEndCbk;
    //oldUserData_ = lefrGetUserData();
    //oldViaCbk = lefCallbacks->ViaCbk;
    //oldSpacingCbk = lefCallbacks->SpacingCbk;
    //oldSpacingBeginCbk = lefCallbacks->SpacingBeginCbk;
    //oldSpacingEndCbk = lefCallbacks->SpacingEndCbk;
    //lefCallbacks->ViaCbk = lefiNonDefaultViaCbk;
    //lefCallbacks->SpacingCbk = lefiNonDefaultSpacingCbk;
    //lefCallbacks->SpacingBeginCbk = 0;
    // lefCallbacks->SpacingEndCbk = 0;
    // pcr 909010, instead of saving the pointer in userData,
    // save it in the global lefData->nd.
    // lefrSetUserData((lefiUserData)this);

    lefData->nd = this;

    if (len > nameSize_) {
        lefFree(name_);
        name_ = (char*) lefMalloc(len);
        nameSize_ = len;
    }
    strcpy(name_, CASE(name));
}

void
lefiNonDefault::addLayer(const char *name)
{
    int len = strlen(name) + 1;
    if (numLayers_ == layersAllocated_) {
        int     i;
        char    **newl;
        double  *neww;
        double  *newd;
        double  *news;
        double  *newe;
        double  *newc;
        double  *newr;
        double  *newec;
        char    *newhw;
        char    *newhd;
        char    *newhs;
        char    *newhe;
        char    *newhc;
        char    *newhr;
        char    *newhec;

        if (layersAllocated_ == 0)
            layersAllocated_ = 2;
        else
            layersAllocated_ *= 2;
        newl = (char**) lefMalloc(sizeof(char*) *
                                  layersAllocated_);
        newe = (double*) lefMalloc(sizeof(double) *
                                   layersAllocated_);
        neww = (double*) lefMalloc(sizeof(double) *
                                   layersAllocated_);
        newd = (double*) lefMalloc(sizeof(double) *
                                   layersAllocated_);
        news = (double*) lefMalloc(sizeof(double) *
                                   layersAllocated_);
        newc = (double*) lefMalloc(sizeof(double) *
                                   layersAllocated_);
        newr = (double*) lefMalloc(sizeof(double) *
                                   layersAllocated_);
        newec = (double*) lefMalloc(sizeof(double) *
                                    layersAllocated_);
        newhe = (char*) lefMalloc(sizeof(char) *
                                  layersAllocated_);
        newhw = (char*) lefMalloc(sizeof(char) *
                                  layersAllocated_);
        newhd = (char*) lefMalloc(sizeof(char) *
                                  layersAllocated_);
        newhs = (char*) lefMalloc(sizeof(char) *
                                  layersAllocated_);
        newhc = (char*) lefMalloc(sizeof(char) *
                                  layersAllocated_);
        newhr = (char*) lefMalloc(sizeof(char) *
                                  layersAllocated_);
        newhec = (char*) lefMalloc(sizeof(char) *
                                   layersAllocated_);
        for (i = 0; i < numLayers_; i++) {
            newl[i] = layerName_[i];
            neww[i] = width_[i];
            newd[i] = diagWidth_[i];
            news[i] = spacing_[i];
            newe[i] = wireExtension_[i];
            newc[i] = capacitance_[i];
            newr[i] = resistance_[i];
            newec[i] = edgeCap_[i];
            newhe[i] = hasWireExtension_[i];
            newhw[i] = hasWidth_[i];
            newhd[i] = hasDiagWidth_[i];
            newhs[i] = hasSpacing_[i];
            newhc[i] = hasCapacitance_[i];
            newhr[i] = hasResistance_[i];
            newhec[i] = hasEdgeCap_[i];
        }
        lefFree((char*) (layerName_));
        lefFree((char*) (width_));
        lefFree((char*) (diagWidth_));
        lefFree((char*) (spacing_));
        lefFree((char*) (wireExtension_));
        lefFree((char*) (capacitance_));
        lefFree((char*) (resistance_));
        lefFree((char*) (edgeCap_));
        lefFree((char*) (hasWireExtension_));
        lefFree((char*) (hasWidth_));
        lefFree((char*) (hasDiagWidth_));
        lefFree((char*) (hasSpacing_));
        lefFree((char*) (hasCapacitance_));
        lefFree((char*) (hasResistance_));
        lefFree((char*) (hasEdgeCap_));
        layerName_ = newl;
        width_ = neww;
        diagWidth_ = newd;
        spacing_ = news;
        wireExtension_ = newe;
        capacitance_ = newc;
        resistance_ = newr;
        edgeCap_ = newec;
        hasWidth_ = newhw;
        hasDiagWidth_ = newhd;
        hasSpacing_ = newhs;
        hasWireExtension_ = newhe;
        hasCapacitance_ = newhc;
        hasResistance_ = newhr;
        hasEdgeCap_ = newhec;
    }
    layerName_[numLayers_] = (char*) lefMalloc(len);
    strcpy(layerName_[numLayers_], CASE(name));
    width_[numLayers_] = 0.0;
    diagWidth_[numLayers_] = 0.0;
    spacing_[numLayers_] = 0.0;
    wireExtension_[numLayers_] = 0.0;
    capacitance_[numLayers_] = 0.0;
    resistance_[numLayers_] = 0.0;
    edgeCap_[numLayers_] = 0.0;
    hasWidth_[numLayers_] = '\0';
    hasDiagWidth_[numLayers_] = '\0';
    hasSpacing_[numLayers_] = '\0';
    hasWireExtension_[numLayers_] = '\0';
    hasCapacitance_[numLayers_] = '\0';
    hasResistance_[numLayers_] = '\0';
    hasEdgeCap_[numLayers_] = '\0';
    numLayers_ += 1;
}

void
lefiNonDefault::addWidth(double num)
{
    width_[numLayers_ - 1] = num;
    hasWidth_[numLayers_ - 1] = 1;
}

void
lefiNonDefault::addDiagWidth(double num)
{
    diagWidth_[numLayers_ - 1] = num;
    hasDiagWidth_[numLayers_ - 1] = 1;
}

void
lefiNonDefault::addSpacing(double num)
{
    spacing_[numLayers_ - 1] = num;
    hasSpacing_[numLayers_ - 1] = 1;
}

void
lefiNonDefault::addWireExtension(double num)
{
    wireExtension_[numLayers_ - 1] = num;
    hasWireExtension_[numLayers_ - 1] = 1;
}

void
lefiNonDefault::addCapacitance(double num)
{
    capacitance_[numLayers_ - 1] = num;
    hasCapacitance_[numLayers_ - 1] = 1;
}

void
lefiNonDefault::addResistance(double num)
{
    resistance_[numLayers_ - 1] = num;
    hasResistance_[numLayers_ - 1] = 1;
}

void
lefiNonDefault::addEdgeCap(double num)
{
    edgeCap_[numLayers_ - 1] = num;
    hasEdgeCap_[numLayers_ - 1] = 1;
}

void
lefiNonDefault::setHardspacing()
{
    hardSpacing_ = 1;
}

void
lefiNonDefault::addUseVia(const char *name)
{
    if (numUseVias_ == allocatedUseVias_) {
        int     i;
        char    **vn;

        if (allocatedUseVias_ == 0)
            allocatedUseVias_ = 2;
        else
            allocatedUseVias_ *= 2;
        vn = (char**) lefMalloc(sizeof(char*) * allocatedUseVias_);
        for (i = 0; i < numUseVias_; i++) {
            vn[i] = useViaName_[i];
        }
        if (numUseVias_)
            lefFree((char*) (useViaName_));
        useViaName_ = vn;
    }
    useViaName_[numUseVias_] = (char*) lefMalloc(strlen(name) + 1);
    strcpy(useViaName_[numUseVias_], CASE(name));
    numUseVias_ += 1;
}

void
lefiNonDefault::addUseViaRule(const char *name)
{
    if (numUseViaRules_ == allocatedUseViaRules_) {
        int     i;
        char    **vn;

        if (allocatedUseViaRules_ == 0)
            allocatedUseViaRules_ = 2;
        else
            allocatedUseViaRules_ *= 2;
        vn = (char**) lefMalloc(sizeof(char*) * allocatedUseViaRules_);
        for (i = 0; i < numUseViaRules_; i++) {
            vn[i] = useViaRuleName_[i];
        }
        if (numUseViaRules_)
            lefFree((char*) (useViaRuleName_));
        useViaRuleName_ = vn;
    }
    useViaRuleName_[numUseViaRules_] = (char*) lefMalloc(strlen(name) + 1);
    strcpy(useViaRuleName_[numUseViaRules_], CASE(name));
    numUseViaRules_ += 1;
}

void
lefiNonDefault::addMinCuts(const char   *name,
                           int          numCuts)
{
    if (numMinCuts_ == allocatedMinCuts_) {
        int     i;
        char    **cn;
        int     *nc;

        if (allocatedMinCuts_ == 0)
            allocatedMinCuts_ = 2;
        else
            allocatedMinCuts_ *= 2;
        cn = (char**) lefMalloc(sizeof(char*) * allocatedMinCuts_);
        nc = (int*) lefMalloc(sizeof(int) * allocatedMinCuts_);
        for (i = 0; i < numMinCuts_; i++) {
            cn[i] = cutLayerName_[i];
            nc[i] = numCuts_[i];
        }
        if (numMinCuts_) {
            lefFree((char*) (cutLayerName_));
            lefFree((char*) (numCuts_));
        }
        cutLayerName_ = cn;
        numCuts_ = nc;
    }
    cutLayerName_[numMinCuts_] = (char*) lefMalloc(strlen(name) + 1);
    strcpy(cutLayerName_[numMinCuts_], CASE(name));
    numCuts_[numMinCuts_] = numCuts;
    numMinCuts_ += 1;
}

void
lefiNonDefault::end()
{
    // Return the callbacks to their normal state.
    // lefrSetViaCbk((lefrViaCbkFnType)oldViaCbk_);
    // lefrSetSpacingCbk((lefrSpacingCbkFnType)oldSpacingCbk_);
    // lefrSetSpacingBeginCbk((lefrVoidCbkFnType)oldSpacingBeginCbk_);
    // lefrSetSpacingEndCbk((lefrVoidCbkFnType)oldSpacingEndCbk_);
    //lefrSetViaCbk(oldViaCbk);
    //lefrSetSpacingCbk(oldSpacingCbk);
    //lefrSetSpacingBeginCbk(oldSpacingBeginCbk);
    //lefrSetSpacingEndCbk(oldSpacingEndCbk);
    // pcr 909010 - global var lefData->nd is used to pass nondefault rule data
    // lefrSetUserData(oldUserData_);
    lefData->nd = 0;
}

int
lefiNonDefault::numLayers() const
{
    return numLayers_;
}

const char *
lefiNonDefault::layerName(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0;
    }
    return layerName_[index];
}

int
lefiNonDefault::hasLayerWidth(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0;
    }
    return hasWidth_[index];
}

double
lefiNonDefault::layerWidth(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0.0;
    }
    return width_[index];
}

int
lefiNonDefault::hasLayerDiagWidth(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0;
    }
    return hasDiagWidth_[index];
}

double
lefiNonDefault::layerDiagWidth(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0.0;
    }
    return diagWidth_[index];
}

int
lefiNonDefault::hasLayerWireExtension(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0;
    }
    return hasWireExtension_[index];
}

int
lefiNonDefault::hasLayerSpacing(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0;
    }
    return hasSpacing_[index];
}

double
lefiNonDefault::layerWireExtension(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0.0;
    }
    return wireExtension_[index];
}

double
lefiNonDefault::layerSpacing(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0.0;
    }
    return spacing_[index];
}

int
lefiNonDefault::hasLayerResistance(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0;
    }
    return hasResistance_[index];
}

double
lefiNonDefault::layerResistance(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0.0;
    }
    return resistance_[index];
}

int
lefiNonDefault::hasLayerCapacitance(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0;
    }
    return hasCapacitance_[index];
}

double
lefiNonDefault::layerCapacitance(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0.0;
    }
    return capacitance_[index];
}

int
lefiNonDefault::hasLayerEdgeCap(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0;
    }
    return hasEdgeCap_[index];
}

double
lefiNonDefault::layerEdgeCap(int index) const
{
    char msg[160];
    if (index < 0 || index >= numLayers_) {
        sprintf(msg, "ERROR (LEFPARS-1402): The index number %d given for the NONDEFAULT LAYER is invalid.\nValid index is from 0 to %d", index, numLayers_);
        lefiError(0, 1402, msg);
        return 0.0;
    }
    return edgeCap_[index];
}

int
lefiNonDefault::numVias() const
{
    return numVias_;
}

lefiVia *
lefiNonDefault::viaRule(int index) const
{
    char msg[160];
    if (index < 0 || index >= numVias_) {
        sprintf(msg, "ERROR (LEFPARS-1403): The index number %d given for the NONDEFAULT VIA is invalid.\nValid index is from 0 to %d", index, numVias_);
        lefiError(0, 1403, msg);
        return 0;
    }
    return viaRules_[index];
}

int
lefiNonDefault::numSpacingRules() const
{
    return numSpacing_;
}

lefiSpacing *
lefiNonDefault::spacingRule(int index) const
{
    char msg[160];
    if (index < 0 || index >= numSpacing_) {
        sprintf(msg, "ERROR (LEFPARS-1404): The index number %d given for the NONDEFAULT SPACING is invalid.\nValid index is from 0 to %d", index, numSpacing_);
        lefiError(0, 1404, msg);
        return 0;
    }
    return spacingRules_[index];
}

const char *
lefiNonDefault::name() const
{
    return name_;
}

int
lefiNonDefault::hasHardspacing() const
{
    return hardSpacing_;
}

int
lefiNonDefault::numUseVia() const
{
    return numUseVias_;
}

const char *
lefiNonDefault::viaName(int index) const
{
    char msg[160];
    if (index < 0 || index >= numUseVias_) {
        sprintf(msg, "ERROR (LEFPARS-1405): The index number %d given for the NONDEFAULT USE VIA is invalid.\nValid index is from 0 to %d", index, numUseVias_);
        lefiError(0, 1405, msg);
        return 0;
    }
    return useViaName_[index];
}

int
lefiNonDefault::numUseViaRule() const
{
    return numUseViaRules_;
}

const char *
lefiNonDefault::viaRuleName(int index) const
{
    char msg[160];
    if (index < 0 || index >= numUseViaRules_) {
        sprintf(msg, "ERROR (LEFPARS-1406): The index number %d given for the NONDEFAULT USE VIARULE is invalid.\nValid index is from 0 to %d", index, numUseViaRules_);
        lefiError(0, 1406, msg);
        return 0;
    }
    return useViaRuleName_[index];
}

int
lefiNonDefault::numMinCuts() const
{
    return numMinCuts_;
}

const char *
lefiNonDefault::cutLayerName(int index) const
{
    char msg[160];
    if (index < 0 || index >= numMinCuts_) {
        sprintf(msg, "ERROR (LEFPARS-1407): The index number %d given for the NONDEFAULT CUT is invalid.\nValid index is from 0 to %d", index, numMinCuts_);
        lefiError(0, 1407, msg);
        return 0;
    }
    return cutLayerName_[index];
}

int
lefiNonDefault::numCuts(int index) const
{
    char msg[160];
    if (index < 0 || index >= numMinCuts_) {
        sprintf(msg, "ERROR (LEFPARS-1407): The index number %d given for the NONDEFAULT CUT is invalid.\nValid index is from 0 to %d", index, numMinCuts_);
        lefiError(0, 1407, msg);
        return 0;
    }
    return numCuts_[index];
}

void
lefiNonDefault::print(FILE *f)
{
    int         i;
    lefiSpacing *s;
    lefiVia     *v;

    fprintf(f, "Nondefault rule %s\n",
            name());
    fprintf(f, "%d layers   %d vias   %d spacing rules\n",
            numLayers(),
            numVias(),
            numSpacingRules());

    for (i = 0; i < numLayers(); i++) {
        fprintf(f, "  Layer %s\n", layerName(i));
        if (hasLayerWidth(i))
            fprintf(f, "    WIDTH %g\n", layerWidth(i));
        if (hasLayerDiagWidth(i))
            fprintf(f, "    DIAGWIDTH %g\n", layerDiagWidth(i));
        if (hasLayerSpacing(i))
            fprintf(f, "    SPACING %g\n", layerSpacing(i));
        if (hasLayerWireExtension(i))
            fprintf(f, "    WIREEXTENSION %g",
                    layerWireExtension(i));
        if (hasLayerResistance(i))
            fprintf(f, "    RESISTANCE RPERSQ %g\n",
                    layerResistance(i));
        if (hasLayerCapacitance(i))
            fprintf(f, "    CAPACITANCE CPERSQDIST %g\n",
                    layerCapacitance(i));
        if (hasLayerEdgeCap(i))
            fprintf(f, "    EDGECAPACITANCE %g\n",
                    layerEdgeCap(i));
    }

    for (i = 0; i < numVias(); i++) {
        v = viaRule(i);
        v->print(f);
    }

    for (i = 0; i < numSpacingRules(); i++) {
        s = spacingRule(i);
        s->print(f);
    }
}

int
lefiNonDefault::numProps() const
{
    return numProps_;
}

void
lefiNonDefault::addProp(const char  *name,
                        const char  *value,
                        const char  type)
{
    int len = strlen(name) + 1;
    if (numProps_ == propsAllocated_) {
        int     i;
        int     max;
        int     lim = numProps_;
        char    **nn;
        char    **nv;
        double  *nD;
        char    *nt;

        if (propsAllocated_ == 0)
            max = propsAllocated_ = 2;
        else
            max = propsAllocated_ *= 2;
        nn = (char**) lefMalloc(sizeof(char*) * max);
        nv = (char**) lefMalloc(sizeof(char*) * max);
        nD = (double*) lefMalloc(sizeof(double) * max);
        nt = (char*) lefMalloc(sizeof(char) * max);
        for (i = 0; i < lim; i++) {
            nn[i] = names_[i];
            nv[i] = values_[i];
            nD[i] = dvalues_[i];
            nt[i] = types_[i];
        }
        lefFree((char*) (names_));
        lefFree((char*) (values_));
        lefFree((char*) (dvalues_));
        lefFree((char*) (types_));
        names_ = nn;
        values_ = nv;
        dvalues_ = nD;
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
lefiNonDefault::addNumProp(const char   *name,
                           const double d,
                           const char   *value,
                           const char   type)
{
    int len = strlen(name) + 1;
    if (numProps_ == propsAllocated_) {
        int     i;
        int     max;
        int     lim = numProps_;
        char    **nn;
        char    **nv;
        double  *nD;
        char    *nt;

        if (propsAllocated_ == 0)
            max = propsAllocated_ = 2;
        else
            max = propsAllocated_ *= 2;
        nn = (char**) lefMalloc(sizeof(char*) * max);
        nv = (char**) lefMalloc(sizeof(char*) * max);
        nD = (double*) lefMalloc(sizeof(double) * max);
        nt = (char*) lefMalloc(sizeof(char) * max);
        for (i = 0; i < lim; i++) {
            nn[i] = names_[i];
            nv[i] = values_[i];
            nD[i] = dvalues_[i];
            nt[i] = types_[i];
        }
        lefFree((char*) (names_));
        lefFree((char*) (values_));
        lefFree((char*) (dvalues_));
        lefFree((char*) (types_));
        names_ = nn;
        values_ = nv;
        dvalues_ = nD;
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
lefiNonDefault::propName(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1408): The index number %d given for the NONDEFAULT PROPERTY is invalid.\nValid index is from 0 to %d", index, numProps_);
        lefiError(0, 1408, msg);
        return 0;
    }
    return names_[index];
}

const char *
lefiNonDefault::propValue(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1408): The index number %d given for the NONDEFAULT PROPERTY is invalid.\nValid index is from 0 to %d", index, numProps_);
        lefiError(0, 1408, msg);
        return 0;
    }
    return values_[index];
}

double
lefiNonDefault::propNumber(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1408): The index number %d given for the NONDEFAULT PROPERTY is invalid.\nValid index is from 0 to %d", index, numProps_);
        lefiError(0, 1408, msg);
        return 0;
    }
    return dvalues_[index];
}

char
lefiNonDefault::propType(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1408): The index number %d given for the NONDEFAULT PROPERTY is invalid.\nValid index is from 0 to %d", index, numProps_);
        lefiError(0, 1408, msg);
        return 0;
    }
    return types_[index];
}

int
lefiNonDefault::propIsNumber(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1408): The index number %d given for the NONDEFAULT PROPERTY is invalid.\nValid index is from 0 to %d", index, numProps_);
        lefiError(0, 1408, msg);
        return 0;
    }
    return dvalues_[index] ? 1 : 0;
}

int
lefiNonDefault::propIsString(int index) const
{
    char msg[160];
    if (index < 0 || index >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1408): The index number %d given for the NONDEFAULT PROPERTY is invalid.\nValid index is from 0 to %d", index, numProps_);
        lefiError(0, 1408, msg);
        return 0;
    }
    return dvalues_[index] ? 0 : 1;
}
END_LEFDEF_PARSER_NAMESPACE

