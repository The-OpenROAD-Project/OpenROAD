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
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include <stdlib.h>
#include <string.h>
#include "lex.h"
#include "lefiLayer.hpp"
#include "lefiDebug.hpp"
#include "lefrSettings.hpp"
#include "lefrCallBacks.hpp"
#include <set>
#include <iostream>
#include <typeinfo>

#include "lefrData.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE


 
// *****************************************************************************
// lefiAntennaPWL
// *****************************************************************************

lefiAntennaPWL::lefiAntennaPWL()
: d_(0),
  r_(0)
{
    Init();
}

lefiAntennaPWL*
lefiAntennaPWL::create()
{
    lefiAntennaPWL* pAntenna = (lefiAntennaPWL*)lefMalloc(sizeof(lefiAntennaPWL));
    pAntenna->d_ = 0;
    pAntenna->r_ = 0;
    pAntenna->Init();

    return pAntenna;
}

void
lefiAntennaPWL::Init()
{
    if (d_) {
        lefFree(d_);
    }

    if (r_) {
        lefFree(r_);
    }

    numAlloc_ = 2;
    d_ = (double*) lefMalloc(sizeof(double) * 2);
    r_ = (double*) lefMalloc(sizeof(double) * 2);
    clear();
}

lefiAntennaPWL::lefiAntennaPWL(const lefiAntennaPWL& prev) {
    LEF_COPY_FUNC(numAlloc_);
    LEF_COPY_FUNC(numPWL_);
    LEF_MALLOC_FUNC(d_, double, sizeof(double)*2); 
    LEF_MALLOC_FUNC(r_, double, sizeof(double)*2); 
}

lefiAntennaPWL::~lefiAntennaPWL()
{
    Destroy();
}

void
lefiAntennaPWL::Destroy()
{
    if (d_)
        lefFree((char*) (d_));
    if (r_)
        lefFree((char*) (r_));
}

// Clear will reset the numPWL_ to zero but keep array allocated
void
lefiAntennaPWL::clear()
{
    numPWL_ = 0;
}

void
lefiAntennaPWL::addAntennaPWL(double    d,
                              double    r)
{
    if (numPWL_ == numAlloc_) {
        int     i, len;
        double  *nd;
        double  *nr;

        if (numAlloc_ == 0)
            len = numAlloc_ = 2;
        else
            len = numAlloc_ *= 2;
        nd = (double*) lefMalloc(sizeof(double) * len);
        nr = (double*) lefMalloc(sizeof(double) * len);

        for (i = 0; i < numPWL_; i++) {
            nd[i] = d_[i];
            nr[i] = r_[i];
        }
        lefFree((char*) (d_));
        lefFree((char*) (r_));
        d_ = nd;
        r_ = nr;
    }
    d_[numPWL_] = d;
    r_[numPWL_] = r;
    numPWL_ += 1;
}

int
lefiAntennaPWL::numPWL() const
{
    return numPWL_;
}

double
lefiAntennaPWL::PWLdiffusion(int index)
{
    if (index < 0 || index >= numPWL_)
        return 0;
    return d_[index];
}

double
lefiAntennaPWL::PWLratio(int index)
{
    if (index < 0 || index >= numPWL_)
        return 0;
    return r_[index];
}

// *****************************************************************************
// lefiLayerDensity
// *****************************************************************************
lefiLayerDensity::lefiLayerDensity()
{
}

void
lefiLayerDensity::Init(const char *type)
{
    int len = strlen(type) + 1;
    type_ = (char*) lefMalloc(len);
    strcpy(type_, CASE(type));
    oneEntry_ = 0;
    numFrequency_ = 0;
    frequency_ = 0;
    numWidths_ = 0;
    widths_ = 0;
    numTableEntries_ = 0;
    tableEntries_ = 0;
    numCutareas_ = 0;
    cutareas_ = 0;
}

lefiLayerDensity::lefiLayerDensity(const lefiLayerDensity& prev) {
    LEF_COPY_FUNC(oneEntry_);
    LEF_COPY_FUNC(numFrequency_);
    LEF_COPY_FUNC(numWidths_);
    LEF_COPY_FUNC(numTableEntries_);
    LEF_COPY_FUNC(numCutareas_);
    LEF_MALLOC_FUNC( type_, char, strlen(prev.type_) );
    LEF_MALLOC_FUNC( frequency_, double, sizeof(double) * numFrequency_);
    LEF_MALLOC_FUNC( widths_, double, sizeof(double) * numWidths_ );
    LEF_MALLOC_FUNC( tableEntries_, double, sizeof(double) * numTableEntries_ );
    LEF_MALLOC_FUNC( cutareas_, double, sizeof(double) * numCutareas_ );
}

void
lefiLayerDensity::Destroy()
{
    if (type_)
        lefFree((char*) (type_));
    if (frequency_)
        lefFree((char*) (frequency_));
    if (widths_)
        lefFree((char*) (widths_));
    if (tableEntries_)
        lefFree((char*) (tableEntries_));
    if (cutareas_)
        lefFree((char*) (cutareas_));
}

lefiLayerDensity::~lefiLayerDensity()
{
    Destroy();
}

void
lefiLayerDensity::setOneEntry(double entry)
{
    oneEntry_ = entry;
}

void
lefiLayerDensity::addFrequency(int      num,
                               double   *frequency)
{
    numFrequency_ = num;
    frequency_ = frequency;
}

void
lefiLayerDensity::addWidth(int      num,
                           double   *width)
{
    numWidths_ = num;
    widths_ = width;
}

void
lefiLayerDensity::addTableEntry(int     num,
                                double  *entry)
{
    numTableEntries_ = num;
    tableEntries_ = entry;
}

void
lefiLayerDensity::addCutarea(int    num,
                             double *cutarea)
{
    numCutareas_ = num;
    cutareas_ = cutarea;
}

int
lefiLayerDensity::hasOneEntry() const
{
    return oneEntry_ ? 1 : 0;
}

char *
lefiLayerDensity::type() const
{
    return type_;
}

double
lefiLayerDensity::oneEntry() const
{
    return oneEntry_;
}

int
lefiLayerDensity::numFrequency() const
{
    return numFrequency_;
}

double
lefiLayerDensity::frequency(int index) const
{
    return frequency_[index];
}

int
lefiLayerDensity::numWidths() const
{
    return numWidths_;
}

double
lefiLayerDensity::width(int index) const
{
    return widths_[index];
}

int
lefiLayerDensity::numTableEntries() const
{
    return numTableEntries_;
}

double
lefiLayerDensity::tableEntry(int index) const
{
    return tableEntries_[index];
}

int
lefiLayerDensity::numCutareas() const
{
    return numCutareas_;
}

double
lefiLayerDensity::cutArea(int index) const
{
    return cutareas_[index];
}

// *****************************************************************************
// lefiParallel
// *****************************************************************************

lefiParallel::lefiParallel()
{
    Init();
}

void
lefiParallel::Init()
{
    numLength_ = 0;
    numWidth_ = 0;
    numWidthAllocated_ = 0;
}

void
lefiParallel::Destroy()
{

    if (numLength_)
        lefFree((char*) (length_));
    if (numWidth_) {
        lefFree((char*) (width_));
        lefFree((char*) (widthSpacing_));
    }
    Init();
}

LEF_COPY_CONSTRUCTOR_C(lefiParallel) {
    this->Init();
    LEF_COPY_FUNC(numLength_);
    LEF_COPY_FUNC(numWidth_);
    LEF_COPY_FUNC(numWidthAllocated_);

    LEF_MALLOC_FUNC(length_, double,  sizeof(double) * numLength_);
    LEF_MALLOC_FUNC(width_, double, sizeof(double) * numWidth_ );
    LEF_MALLOC_FUNC(widthSpacing_, double, sizeof(double) * ( (numLength_>0)? numWidthAllocated_ * numLength_ : numWidthAllocated_ ));
}

LEF_ASSIGN_OPERATOR_C(lefiParallel) {
    CHECK_SELF_ASSIGN
    this->Init();
    LEF_COPY_FUNC(numLength_);
    LEF_COPY_FUNC(numWidth_);
    LEF_COPY_FUNC(numWidthAllocated_);

    LEF_MALLOC_FUNC(length_, double,  sizeof(double) * numLength_);
    LEF_MALLOC_FUNC(width_, double, sizeof(double) * numWidth_ );
    LEF_MALLOC_FUNC(widthSpacing_, double, sizeof(double) * ( (numLength_>0)? numWidthAllocated_ * numLength_ : numWidthAllocated_ ));

    return *this;
}

lefiParallel::~lefiParallel()
{
    Destroy();
}

void
lefiParallel::addParallelLength(int     numLength,
                                double  *lengths)
{
    numLength_ = numLength;
    length_ = lengths;
}

void
lefiParallel::addParallelWidth(double width)
{
    if (numWidth_ == numWidthAllocated_) {
        int     i, numLength;
        double  *nw;
        double  *nws;

        numWidthAllocated_ = numWidthAllocated_ ?
            numWidthAllocated_ * 2 : 2;
        nw = (double*) lefMalloc(sizeof(double) * numWidthAllocated_);
        numLength = numLength_;
        if (numLength > 0)
            nws = (double*) lefMalloc(sizeof(double) * (numWidthAllocated_ *
                                                        numLength_));
        else {
            // still want to move anything already there 
            nws = (double*) lefMalloc(sizeof(double) * numWidthAllocated_);
            numLength = 1;
        }

        for (i = 0; i < numWidth_; i++)
            nw[i] = width_[i];
        for (i = 0; i < (numWidth_ * numLength); i++)
            nws[i] = widthSpacing_[i];
        if (numWidth_) {
            lefFree((char*) (width_));
            lefFree((char*) (widthSpacing_));
        }
        width_ = nw;
        widthSpacing_ = nws;
    }
    width_[numWidth_] = width;
    numWidth_ += 1;
}

void
lefiParallel::addParallelWidthSpacing(int       numSpacing,
                                      double    *spacings)
{
    int i;
    for (i = 0; i < numSpacing; i++)
        widthSpacing_[(numWidth_ - 1) * numLength_ + i] =
            spacings[i];
}

int
lefiParallel::numLength() const
{
    return numLength_;
}

int
lefiParallel::numWidth() const
{
    return numWidth_;
}

double
lefiParallel::length(int index) const
{
    return length_[index];
}

double
lefiParallel::width(int index) const
{
    return width_[index];
}

double
lefiParallel::widthSpacing(int  iWidth,
                           int  iWidthSpacing) const
{
    return widthSpacing_[iWidth * numLength_ + iWidthSpacing];
}

// *****************************************************************************
// lefiInfluence
// *****************************************************************************

lefiInfluence::lefiInfluence()
{
    Init();
}

void
lefiInfluence::Init()
{
    numAllocated_ = 0;
    numWidth_ = 0;
    numDistance_ = 0;
    numSpacing_ = 0;
}

LEF_COPY_CONSTRUCTOR_C( lefiInfluence ) {
    this->Init();
    LEF_COPY_FUNC(numAllocated_);
    LEF_COPY_FUNC(numWidth_);
    LEF_COPY_FUNC(numDistance_);
    LEF_COPY_FUNC(numSpacing_);
    LEF_MALLOC_FUNC( width_, double, sizeof(double) * numAllocated_ );
    LEF_MALLOC_FUNC( distance_, double, sizeof(double) * numAllocated_ );
    LEF_MALLOC_FUNC( spacing_, double, sizeof(double) * numAllocated_ );
}

LEF_ASSIGN_OPERATOR_C( lefiInfluence ) {
    CHECK_SELF_ASSIGN
    this->Init();
    LEF_COPY_FUNC(numAllocated_);
    LEF_COPY_FUNC(numWidth_);
    LEF_COPY_FUNC(numDistance_);
    LEF_COPY_FUNC(numSpacing_);
    LEF_MALLOC_FUNC( width_, double, sizeof(double) * numAllocated_ );
    LEF_MALLOC_FUNC( distance_, double, sizeof(double) * numAllocated_ );
    LEF_MALLOC_FUNC( spacing_, double, sizeof(double) * numAllocated_ );
    return *this;
}

void
lefiInfluence::Destroy()
{
    if (numWidth_) {
        lefFree((char*) (width_));
        lefFree((char*) (distance_));
        lefFree((char*) (spacing_));
    }
    Init();
}

lefiInfluence::~lefiInfluence()
{
    Destroy();
}

void
lefiInfluence::addInfluence(double  width,
                            double  distance,
                            double  spacing)
{
    int     i;
    double  *nw;
    double  *nd;
    double  *ns;

    if (numWidth_ == numAllocated_) {
        numAllocated_ = numAllocated_ ?
            numAllocated_ * 2 : 2;
        nw = (double*) lefMalloc(sizeof(double) * numAllocated_);
        nd = (double*) lefMalloc(sizeof(double) * numAllocated_);
        ns = (double*) lefMalloc(sizeof(double) * numAllocated_);

        for (i = 0; i < numWidth_; i++) {
            nw[i] = width_[i];
            nd[i] = distance_[i];
            ns[i] = spacing_[i];
        }
        if (numWidth_ > 0) {
            lefFree((char*) (width_));
            lefFree((char*) (distance_));
            lefFree((char*) (spacing_));
        }
        width_ = nw;
        distance_ = nd;
        spacing_ = ns;
    }
    width_[numWidth_] = width;
    distance_[numWidth_] = distance;
    spacing_[numWidth_] = spacing;
    numWidth_ += 1;
}

int
lefiInfluence::numInfluenceEntry() const
{
    return numWidth_;
}

double
lefiInfluence::width(int index) const
{
    return width_[index];
}

double
lefiInfluence::distance(int index) const
{
    return distance_[index];
}

double
lefiInfluence::spacing(int index) const
{
    return spacing_[index];
}

// *****************************************************************************
// lefiTwoWidths
// *****************************************************************************

lefiTwoWidths::lefiTwoWidths()
{
    Init();
}

void
lefiTwoWidths::Init()
{
    numWidth_ = 0;
    numWidthAllocated_ = 0;
}

lefiTwoWidths::lefiTwoWidths(const lefiTwoWidths& prev) {
    this->Init();
    LEF_COPY_FUNC( numWidth_ );
    LEF_COPY_FUNC( numWidthAllocated_ );
    LEF_MALLOC_FUNC( width_, double, sizeof(double) * numWidthAllocated_ );
    LEF_MALLOC_FUNC( prl_, double, sizeof(double) * numWidthAllocated_ );
    LEF_MALLOC_FUNC( hasPRL_, int, sizeof(int) * numWidthAllocated_ );
    LEF_MALLOC_FUNC( numWidthSpacing_, int, sizeof(int) * numWidthAllocated_ );
    LEF_MALLOC_FUNC( widthSpacing_, double, sizeof(double) * numWidthAllocated_ );
    LEF_MALLOC_FUNC( atNsp_, int, sizeof(int) * ( prev.atNsp_[numWidth_] ) );
}
void
lefiTwoWidths::Destroy()
{

    if (numWidth_) {
        lefFree((char*) (width_));
        lefFree((char*) (prl_));
        lefFree((char*) (widthSpacing_));
        lefFree((char*) (numWidthSpacing_));
        lefFree((char*) (atNsp_));
        lefFree((char*) (hasPRL_));
    }
    Init();
}

lefiTwoWidths::~lefiTwoWidths()
{
    Destroy();
}

void
lefiTwoWidths::addTwoWidths(double  width,
                            double  prl,
                            int     numSpacing,
                            double  *spacings,
                            int     hasPRL)
{
    int i;

    if (numWidth_ == numWidthAllocated_) {
        double  *nw;
        double  *np;
        int     *nnws;
        double  *nws;
        int     *nat;
        int     *nHasPrl;

        numWidthAllocated_ = numWidthAllocated_ ?
            numWidthAllocated_ * 2 : 2;
        nw = (double*) lefMalloc(sizeof(double) * numWidthAllocated_);
        np = (double*) lefMalloc(sizeof(double) * numWidthAllocated_);
        nnws = (int*) lefMalloc(sizeof(int) * numWidthAllocated_);
        nat = (int*) lefMalloc(sizeof(int) * numWidthAllocated_);
        nHasPrl = (int*) lefMalloc(sizeof(int) * numWidthAllocated_);

        for (i = 0; i < numWidth_; i++) {
            nw[i] = width_[i];
            np[i] = prl_[i];
            nnws[i] = numWidthSpacing_[i];
            nat[i] = atNsp_[i];
            nHasPrl[i] = hasPRL_[i];
        }
        // The lefData->last value in atNsp_ is the lefData->last total number of spacing
        if (numWidth_ > 0) {
            nws = (double*) lefMalloc(sizeof(double) * (atNsp_[numWidth_ - 1]
                                                        + numSpacing));
            for (i = 0; i < atNsp_[numWidth_ - 1]; i++) {
                nws[i] = widthSpacing_[i];
            }
        } else
            nws = (double*) lefMalloc(sizeof(double) * numSpacing);

        if (numWidth_) {
            lefFree((char*) (width_));
            lefFree((char*) (prl_));
            lefFree((char*) (numWidthSpacing_));
            lefFree((char*) (widthSpacing_));
            lefFree((char*) (atNsp_));
            lefFree((char*) (hasPRL_));
        }
        width_ = nw;
        prl_ = np;
        numWidthSpacing_ = nnws;
        widthSpacing_ = nws;
        atNsp_ = nat;
        hasPRL_ = nHasPrl;
    } else {  // need to allocate memory for widthSpacing_
        double *nws;
        nws = (double*) lefMalloc(sizeof(double) * (atNsp_[numWidth_ - 1]
                                                    + numSpacing));
        for (i = 0; i < atNsp_[numWidth_ - 1]; i++) {
            nws[i] = widthSpacing_[i];
        }
        lefFree((char*) (widthSpacing_));
        widthSpacing_ = nws;
    }
    width_[numWidth_] = width;
    prl_[numWidth_] = prl;
    hasPRL_[numWidth_] = hasPRL;
    numWidthSpacing_[numWidth_] = numSpacing;
    if (numWidth_ == 0) {
        for (i = 0; i < numSpacing; i++)
            widthSpacing_[i] = spacings[i];
        atNsp_[0] = numSpacing;
    } else {
        for (i = 0; i < numSpacing; i++)
            widthSpacing_[atNsp_[numWidth_ - 1] + i] = spacings[i];
        atNsp_[numWidth_] = atNsp_[numWidth_ - 1] + numSpacing;
    }
    numWidth_ += 1;
}

int
lefiTwoWidths::numWidth() const
{
    return numWidth_;
}

double
lefiTwoWidths::width(int index) const
{
    return width_[index];
}

int
lefiTwoWidths::hasWidthPRL(int index) const
{
    if (hasPRL_[index])
        return 1;
    return 0;
}

double
lefiTwoWidths::widthPRL(int index) const
{
    return prl_[index];
}

int
lefiTwoWidths::numWidthSpacing(int index) const
{
    return numWidthSpacing_[index];
}

double
lefiTwoWidths::widthSpacing(int iWidth,
                            int iWidthSpacing) const
{
    if (iWidth == 0)
        return widthSpacing_[iWidthSpacing];
    return widthSpacing_[atNsp_[iWidth - 1] + iWidthSpacing];
}

// *****************************************************************************
// lefiSpacingTable
// *****************************************************************************

lefiSpacingTable::lefiSpacingTable()
{
    Init();
}

void
lefiSpacingTable::Init()
{
    hasInfluence_ = 0;
    parallel_ = 0;
    influence_ = 0;
    twoWidths_ = 0;             // 5.7
}

LEF_COPY_CONSTRUCTOR_C( lefiSpacingTable ) {
    this->Init();
    LEF_COPY_FUNC( hasInfluence_ );
    LEF_MALLOC_FUNC_WITH_OPERATOR( influence_, lefiInfluence, sizeof(lefiInfluence));
    LEF_MALLOC_FUNC_WITH_OPERATOR( parallel_, lefiParallel, sizeof(lefiParallel) );
    LEF_MALLOC_FUNC_WITH_OPERATOR( twoWidths_, lefiTwoWidths, sizeof(lefiTwoWidths));    
}

LEF_ASSIGN_OPERATOR_C( lefiSpacingTable) {
    CHECK_SELF_ASSIGN
    this->Init();
    LEF_COPY_FUNC( hasInfluence_ );
    LEF_MALLOC_FUNC_WITH_OPERATOR( influence_, lefiInfluence, sizeof(lefiInfluence));
    LEF_MALLOC_FUNC_WITH_OPERATOR( parallel_, lefiParallel, sizeof(lefiParallel) );
    LEF_MALLOC_FUNC_WITH_OPERATOR( twoWidths_, lefiTwoWidths, sizeof(lefiTwoWidths));   
    return *this; 
}

void
lefiSpacingTable::Destroy()
{
    if ((hasInfluence_) && (influence_))
        influence_->Destroy();
    else if (parallel_) {
        parallel_->Destroy();
        lefFree((char*) parallel_);
    }
    if (influence_)
        lefFree((char*) influence_);
    if (twoWidths_) {
        twoWidths_->Destroy();
        lefFree((char*) twoWidths_);
    }
    Init();
}

lefiSpacingTable::~lefiSpacingTable()
{
    Destroy();
}

void
lefiSpacingTable::addParallelLength(int     numLength,
                                    double  *lengths)
{
    lefiParallel *parallel;

    if (parallel_ == 0) {
        parallel = (lefiParallel*) lefMalloc(sizeof(lefiParallel));
        parallel->Init();
        parallel_ = parallel;
    } else
        parallel = parallel_;
    parallel->addParallelLength(numLength, lengths);
}

void
lefiSpacingTable::addParallelWidth(double width)
{
    lefiParallel *parallel;

    parallel = parallel_;
    parallel->addParallelWidth(width);
}

void
lefiSpacingTable::addParallelWidthSpacing(int       numSpacing,
                                          double    *spacings)
{
    lefiParallel *parallel;

    parallel = parallel_;
    parallel->addParallelWidthSpacing(numSpacing, spacings);
}

void
lefiSpacingTable::setInfluence()
{
    lefiInfluence *influence;

    influence = (lefiInfluence*) lefMalloc(sizeof(lefiInfluence));
    influence->Init();
    influence_ = influence;
    hasInfluence_ = 1;
}

void
lefiSpacingTable::addInfluence(double   width,
                               double   distance,
                               double   spacing)
{
    lefiInfluence *influence;

    influence = influence_;
    influence->addInfluence(width, distance, spacing);
}

int
lefiSpacingTable::isInfluence() const
{
    return hasInfluence_ ? 1 : 0;
}

int
lefiSpacingTable::isParallel() const
{
    return parallel_ ? 1 : 0;
}

lefiInfluence *
lefiSpacingTable::influence() const
{
    return influence_;
}

lefiParallel *
lefiSpacingTable::parallel() const
{
    return parallel_;
}

lefiTwoWidths *
lefiSpacingTable::twoWidths() const
{
    return twoWidths_;
}

// 5.7
void
lefiSpacingTable::addTwoWidths(double   width,
                               double   runLength,
                               int      numSpacing,
                               double   *spacings,
                               int      hasPRL)
{
    lefiTwoWidths *twoWidths;

    if (twoWidths_ == 0) {
        twoWidths = (lefiTwoWidths*) lefMalloc(sizeof(lefiTwoWidths));
        twoWidths->Init();
        twoWidths_ = twoWidths;
    } else
        twoWidths = twoWidths_;
    twoWidths->addTwoWidths(width, runLength, numSpacing,
                            spacings, hasPRL);
}

// *****************************************************************************
// lefiOrthogonal
// *****************************************************************************

lefiOrthogonal::lefiOrthogonal()
{
    Init();
}

void
lefiOrthogonal::Init()
{
    numAllocated_ = 0;
    numCutOrtho_ = 0;
    cutWithin_ = 0;
    ortho_ = 0;
}

LEF_COPY_CONSTRUCTOR_C( lefiOrthogonal ) {
    this->Init();
    LEF_COPY_FUNC( numAllocated_ );
    LEF_COPY_FUNC( numCutOrtho_ );
    LEF_MALLOC_FUNC( cutWithin_, double, sizeof(double) * numCutOrtho_ );
    LEF_MALLOC_FUNC( ortho_, double, sizeof(double) * numCutOrtho_  ); 
}

LEF_ASSIGN_OPERATOR_C( lefiOrthogonal ) {
    CHECK_SELF_ASSIGN
    this->Init();
    LEF_COPY_FUNC( numAllocated_ );
    LEF_COPY_FUNC( numCutOrtho_ );
    LEF_MALLOC_FUNC( cutWithin_, double, sizeof(double) * numCutOrtho_ );
    LEF_MALLOC_FUNC( ortho_, double, sizeof(double) * numCutOrtho_ ); 
    return *this;
}




lefiOrthogonal::~lefiOrthogonal()
{
    Destroy();
}

void
lefiOrthogonal::Destroy()
{
    if (cutWithin_)
        lefFree((char*) (cutWithin_));
    if (ortho_)
        lefFree((char*) (ortho_));
    numAllocated_ = 0;
    numCutOrtho_ = 0;
}

void
lefiOrthogonal::addOrthogonal(double    cutWithin,
                              double    ortho)
{
    int     i, len;
    double  *cw;
    double  *ot;

    if (numAllocated_ == numCutOrtho_) {
        if (numAllocated_ == 0)
            len = numAllocated_ = 2;
        else
            len = numAllocated_ *= 2;
        cw = (double*) lefMalloc(sizeof(double) * len);
        ot = (double*) lefMalloc(sizeof(double) * len);

        for (i = 0; i < numCutOrtho_; i++) {
            cw[i] = cutWithin_[i];
            ot[i] = ortho_[i];
        }
        if (cutWithin_)
            lefFree((char*) (cutWithin_));
        if (ortho_)
            lefFree((char*) (ortho_));
        cutWithin_ = cw;
        ortho_ = ot;
    }
    cutWithin_[numCutOrtho_] = cutWithin;
    ortho_[numCutOrtho_] = ortho;
    numCutOrtho_ += 1;
}

int
lefiOrthogonal::numOrthogonal() const
{
    return numCutOrtho_;
}

double
lefiOrthogonal::cutWithin(int index) const
{
    if (index < 0 || index >= numCutOrtho_)
        return 0;
    return cutWithin_[index];
}

double
lefiOrthogonal::orthoSpacing(int index) const
{
    if (index < 0 || index >= numCutOrtho_)
        return 0;
    return ortho_[index];
}

// *****************************************************************************
// lefiAntennaModel
// *****************************************************************************

lefiAntennaModel::lefiAntennaModel()
{
    Init();
}

void
lefiAntennaModel::Init()
{
    hasAntennaAreaRatio_ = 0;
    hasAntennaDiffAreaRatio_ = 0;
    hasAntennaDiffAreaRatioPWL_ = 0;
    hasAntennaCumAreaRatio_ = 0;
    hasAntennaCumDiffAreaRatio_ = 0;
    hasAntennaCumDiffAreaRatioPWL_ = 0;
    hasAntennaAreaFactor_ = 0;
    hasAntennaAreaFactorDUO_ = 0;
    hasAntennaSideAreaRatio_ = 0;
    hasAntennaDiffSideAreaRatio_ = 0;
    hasAntennaDiffSideAreaRatioPWL_ = 0;
    hasAntennaCumSideAreaRatio_ = 0;
    hasAntennaCumDiffSideAreaRatio_ = 0;
    hasAntennaCumDiffSideAreaRatioPWL_ = 0;
    hasAntennaSideAreaFactor_ = 0;
    hasAntennaSideAreaFactorDUO_ = 0;
    hasAntennaCumRoutingPlusCut_ = 0;     // 5.7
    hasAntennaGatePlusDiff_ = 0;          // 5.7
    hasAntennaAreaMinusDiff_ = 0;         // 5.7
    antennaDiffAreaRatioPWL_ = 0;
    antennaCumDiffAreaRatioPWL_ = 0;
    antennaDiffSideAreaRatioPWL_ = 0;
    antennaCumDiffSideAreaRatioPWL_ = 0;
    antennaAreaDiffReducePWL_ = 0;         // 5.7
    oxide_ = 0;
}

lefiAntennaModel::lefiAntennaModel( const lefiAntennaModel& prev) {

    LEF_COPY_FUNC( hasAntennaAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaDiffAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaDiffAreaRatioPWL_ );
    LEF_COPY_FUNC( hasAntennaCumAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaCumDiffAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaCumDiffAreaRatioPWL_ );
    LEF_COPY_FUNC( hasAntennaAreaFactor_ );
    LEF_COPY_FUNC( hasAntennaAreaFactorDUO_ );
    LEF_COPY_FUNC( hasAntennaSideAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaDiffSideAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaDiffSideAreaRatioPWL_ );
    LEF_COPY_FUNC( hasAntennaCumSideAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaCumDiffSideAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaCumDiffSideAreaRatioPWL_ );
    LEF_COPY_FUNC( hasAntennaSideAreaFactor_ );
    LEF_COPY_FUNC( hasAntennaSideAreaFactorDUO_ );
    LEF_COPY_FUNC( hasAntennaCumRoutingPlusCut_ );
    LEF_COPY_FUNC( hasAntennaGatePlusDiff_ );
    LEF_COPY_FUNC( hasAntennaAreaMinusDiff_ );

    LEF_MALLOC_FUNC( oxide_, char, sizeof(char)*6 );
    LEF_COPY_FUNC( antennaAreaRatio_ );
    LEF_COPY_FUNC( antennaDiffAreaRatio_ );
    LEF_MALLOC_FUNC( antennaDiffAreaRatioPWL_, lefiAntennaPWL, sizeof(lefiAntennaPWL) );

    LEF_COPY_FUNC( antennaCumAreaRatio_ );
    LEF_COPY_FUNC( antennaCumDiffAreaRatio_ );
    LEF_MALLOC_FUNC( antennaCumDiffAreaRatioPWL_,  lefiAntennaPWL, sizeof(lefiAntennaPWL) );

    LEF_COPY_FUNC( antennaAreaFactor_ );
    LEF_COPY_FUNC( antennaSideAreaRatio_ );
    LEF_COPY_FUNC( antennaDiffSideAreaRatio_ );
    LEF_MALLOC_FUNC( antennaDiffSideAreaRatioPWL_,  lefiAntennaPWL, sizeof(lefiAntennaPWL) );

    LEF_COPY_FUNC( antennaCumSideAreaRatio_ );
    LEF_COPY_FUNC( antennaCumDiffSideAreaRatio_ );
    LEF_MALLOC_FUNC( antennaCumDiffSideAreaRatioPWL_,  lefiAntennaPWL, sizeof(lefiAntennaPWL) );

    LEF_COPY_FUNC( antennaSideAreaFactor_ );
    LEF_COPY_FUNC( antennaGatePlusDiff_ );
    LEF_COPY_FUNC( antennaAreaMinusDiff_ );
    LEF_MALLOC_FUNC( antennaAreaDiffReducePWL_,  lefiAntennaPWL, sizeof(lefiAntennaPWL) );

}

void
lefiAntennaModel::Destroy()
{
    if (oxide_) {
        lefFree((char*) (oxide_));
    }

    if (antennaDiffAreaRatioPWL_) {
        antennaDiffAreaRatioPWL_->Destroy();
        lefFree((char*) (antennaDiffAreaRatioPWL_));
        antennaDiffAreaRatioPWL_ = 0;
    }
    
    if (antennaCumDiffAreaRatioPWL_) {
        antennaCumDiffAreaRatioPWL_->Destroy();
        lefFree((char*) (antennaCumDiffAreaRatioPWL_));
        antennaCumDiffAreaRatioPWL_ = 0;
    }
    
    if (antennaDiffSideAreaRatioPWL_) {
        antennaDiffSideAreaRatioPWL_->Destroy();
        lefFree((char*) (antennaDiffSideAreaRatioPWL_));
        antennaDiffSideAreaRatioPWL_ = 0;
    }
    
    if (antennaCumDiffSideAreaRatioPWL_) {
        antennaCumDiffSideAreaRatioPWL_->Destroy();
        lefFree((char*) (antennaCumDiffSideAreaRatioPWL_));
        antennaCumDiffSideAreaRatioPWL_ = 0;
    }
    
    if (antennaAreaDiffReducePWL_) {    // 5.7
        antennaAreaDiffReducePWL_->Destroy();
        lefFree((char*) (antennaAreaDiffReducePWL_));
        antennaAreaDiffReducePWL_ = 0;
    }

    Init();
}

lefiAntennaModel::~lefiAntennaModel()
{
    Destroy();
}

// 5.5
void
lefiAntennaModel::setAntennaModel(int aOxide)
{
    if (oxide_)
        lefFree((char*) (oxide_));
    switch (aOxide) {
    case 2:
        oxide_ = strdup("OXIDE2");
        break;
    case 3:
        oxide_ = strdup("OXIDE3");
        break;
    case 4:
        oxide_ = strdup("OXIDE4");
        break;
    default:
        oxide_ = strdup("OXIDE1");
        break;
    }
}

// 3/23/2000 -- Wanda da Rosa.  The following are for 5.4 syntax
void
lefiAntennaModel::setAntennaAreaRatio(double value)
{
    antennaAreaRatio_ = value;
    hasAntennaAreaRatio_ = 1;
}

void
lefiAntennaModel::setAntennaCumAreaRatio(double value)
{
    antennaCumAreaRatio_ = value;
    hasAntennaCumAreaRatio_ = 1;
}

void
lefiAntennaModel::setAntennaAreaFactor(double value)
{
    antennaAreaFactor_ = value;
    hasAntennaAreaFactor_ = 1;
}

void
lefiAntennaModel::setAntennaSideAreaRatio(double value)
{
    antennaSideAreaRatio_ = value;
    hasAntennaSideAreaRatio_ = 1;
}

void
lefiAntennaModel::setAntennaCumSideAreaRatio(double value)
{
    antennaCumSideAreaRatio_ = value;
    hasAntennaCumSideAreaRatio_ = 1;
}

void
lefiAntennaModel::setAntennaSideAreaFactor(double value)
{
    antennaSideAreaFactor_ = value;
    hasAntennaSideAreaFactor_ = 1;
}

void
lefiAntennaModel::setAntennaValue(lefiAntennaEnum   antennaType,
                                  double            value)
{
    switch (antennaType) {
    case lefiAntennaDAR:
        antennaDiffAreaRatio_ = value;
        hasAntennaDiffAreaRatio_ = 1;
        break;
    case lefiAntennaCDAR:
        antennaCumDiffAreaRatio_ = value;
        hasAntennaCumDiffAreaRatio_ = 1;
        break;
    case lefiAntennaDSAR:
        antennaDiffSideAreaRatio_ = value;
        hasAntennaDiffSideAreaRatio_ = 1;
        break;
    case lefiAntennaCDSAR:
        antennaCumDiffSideAreaRatio_ = value;
        hasAntennaCumDiffSideAreaRatio_ = 1;
        break;
    default:
        break;
    }
}

void
lefiAntennaModel::setAntennaDUO(lefiAntennaEnum antennaType)
{
    switch (antennaType) {
    case lefiAntennaAF:
        hasAntennaAreaFactorDUO_ = 1;
        break;
    case lefiAntennaSAF:
        hasAntennaSideAreaFactorDUO_ = 1;
        break;
    default:
        break;
    }
}

// This function 'consumes' data of pwl pointer and send it to the new owner. 
// After calling the function 'pwl' should be set to NULL or assigned a new 
// value. 
void
lefiAntennaModel::setAntennaPWL(lefiAntennaEnum antennaType,
                                lefiAntennaPWL  *pwl)
{
    switch (antennaType) {
    case lefiAntennaDAR:
        if (antennaDiffAreaRatioPWL_) {
            antennaDiffAreaRatioPWL_->Destroy();
            lefFree(antennaDiffAreaRatioPWL_);
        }

        antennaDiffAreaRatioPWL_ = pwl;
        break;

    case lefiAntennaCDAR:
        if (antennaCumDiffAreaRatioPWL_) {
            antennaCumDiffAreaRatioPWL_->Destroy();
            lefFree(antennaCumDiffAreaRatioPWL_);
        }

        antennaCumDiffAreaRatioPWL_ = pwl;
        break;

    case lefiAntennaDSAR:
        if (antennaDiffSideAreaRatioPWL_) {
            antennaDiffSideAreaRatioPWL_->Destroy();
            lefFree(antennaDiffSideAreaRatioPWL_);
        }

        antennaDiffSideAreaRatioPWL_ = pwl;
        break;

    case lefiAntennaCDSAR:
        if (antennaCumDiffSideAreaRatioPWL_) {
            antennaCumDiffSideAreaRatioPWL_->Destroy();
            lefFree(antennaCumDiffSideAreaRatioPWL_);
        }

        antennaCumDiffSideAreaRatioPWL_ = pwl;
        break;

    case lefiAntennaADR:
        if (antennaAreaDiffReducePWL_) {
            antennaAreaDiffReducePWL_->Destroy();
            lefFree(antennaAreaDiffReducePWL_);
        }

        antennaAreaDiffReducePWL_ = pwl;
        break;

    default:
        pwl->Destroy();
        lefFree(pwl);
        break;
    }
}

// 5.7
void
lefiAntennaModel::setAntennaCumRoutingPlusCut()
{
    hasAntennaCumRoutingPlusCut_ = 1;
}

// 5.7
void
lefiAntennaModel::setAntennaGatePlusDiff(double value)
{
    antennaGatePlusDiff_ = value;
    hasAntennaGatePlusDiff_ = 1;
}

// 5.7
void
lefiAntennaModel::setAntennaAreaMinusDiff(double value)
{
    antennaAreaMinusDiff_ = value;
    hasAntennaAreaMinusDiff_ = 1;
}

int
lefiAntennaModel::hasAntennaAreaRatio() const
{
    return hasAntennaAreaRatio_;
}

int
lefiAntennaModel::hasAntennaDiffAreaRatio() const
{
    return hasAntennaDiffAreaRatio_;
}

int
lefiAntennaModel::hasAntennaCumAreaRatio() const
{
    return hasAntennaCumAreaRatio_;
}

int
lefiAntennaModel::hasAntennaCumDiffAreaRatio() const
{
    return hasAntennaCumDiffAreaRatio_;
}

int
lefiAntennaModel::hasAntennaAreaFactor() const
{
    return hasAntennaAreaFactor_;
}

int
lefiAntennaModel::hasAntennaSideAreaRatio() const
{
    return hasAntennaSideAreaRatio_;
}

int
lefiAntennaModel::hasAntennaDiffSideAreaRatio() const
{
    return hasAntennaDiffSideAreaRatio_;
}

int
lefiAntennaModel::hasAntennaCumSideAreaRatio() const
{
    return hasAntennaCumSideAreaRatio_;
}

int
lefiAntennaModel::hasAntennaCumDiffSideAreaRatio() const
{
    return hasAntennaCumDiffSideAreaRatio_;
}

int
lefiAntennaModel::hasAntennaSideAreaFactor() const
{
    return hasAntennaSideAreaFactor_;
}

int
lefiAntennaModel::hasAntennaDiffAreaRatioPWL() const
{
    return antennaDiffAreaRatioPWL_ ? 1 : 0;
}

int
lefiAntennaModel::hasAntennaCumDiffAreaRatioPWL() const
{
    return antennaCumDiffAreaRatioPWL_ ? 1 : 0;
}

int
lefiAntennaModel::hasAntennaDiffSideAreaRatioPWL() const
{
    return antennaDiffSideAreaRatioPWL_ ? 1 : 0;
}

int
lefiAntennaModel::hasAntennaCumDiffSideAreaRatioPWL() const
{
    return antennaCumDiffSideAreaRatioPWL_ ? 1 : 0;
}

int
lefiAntennaModel::hasAntennaAreaFactorDUO() const
{
    return hasAntennaAreaFactorDUO_;
}

int
lefiAntennaModel::hasAntennaSideAreaFactorDUO() const
{
    return hasAntennaSideAreaFactorDUO_;
}

// 5.7
int
lefiAntennaModel::hasAntennaCumRoutingPlusCut() const
{
    return hasAntennaCumRoutingPlusCut_;
}

// 5.7
int
lefiAntennaModel::hasAntennaGatePlusDiff() const
{
    return hasAntennaGatePlusDiff_;
}

// 5.7
int
lefiAntennaModel::hasAntennaAreaMinusDiff() const
{
    return hasAntennaAreaMinusDiff_;
}

// 5.7
int
lefiAntennaModel::hasAntennaAreaDiffReducePWL() const
{
    return antennaAreaDiffReducePWL_ ? 1 : 0;
}

// 5.5
char *
lefiAntennaModel::antennaOxide() const
{
    return oxide_;
}

double
lefiAntennaModel::antennaAreaRatio() const
{
    return antennaAreaRatio_;
}

double
lefiAntennaModel::antennaDiffAreaRatio() const
{
    return antennaDiffAreaRatio_;
}

double
lefiAntennaModel::antennaCumAreaRatio() const
{
    return antennaCumAreaRatio_;
}

double
lefiAntennaModel::antennaCumDiffAreaRatio() const
{
    return antennaCumDiffAreaRatio_;
}

double
lefiAntennaModel::antennaAreaFactor() const
{
    return antennaAreaFactor_;
}

double
lefiAntennaModel::antennaSideAreaRatio() const
{
    return antennaSideAreaRatio_;
}

double
lefiAntennaModel::antennaDiffSideAreaRatio() const
{
    return antennaDiffSideAreaRatio_;
}

double
lefiAntennaModel::antennaCumSideAreaRatio() const
{
    return antennaCumSideAreaRatio_;
}

double
lefiAntennaModel::antennaCumDiffSideAreaRatio() const
{
    return antennaCumDiffSideAreaRatio_;
}

double
lefiAntennaModel::antennaSideAreaFactor() const
{
    return antennaSideAreaFactor_;
}

lefiAntennaPWL *
lefiAntennaModel::antennaDiffAreaRatioPWL() const
{
    return antennaDiffAreaRatioPWL_;
}

lefiAntennaPWL *
lefiAntennaModel::antennaCumDiffAreaRatioPWL() const
{
    return antennaCumDiffAreaRatioPWL_;
}

lefiAntennaPWL *
lefiAntennaModel::antennaDiffSideAreaRatioPWL() const
{
    return antennaDiffSideAreaRatioPWL_;
}

lefiAntennaPWL *
lefiAntennaModel::antennaCumDiffSideAreaRatioPWL() const
{
    return antennaCumDiffSideAreaRatioPWL_;
}

// 5.7
double
lefiAntennaModel::antennaGatePlusDiff() const
{
    return antennaGatePlusDiff_;
}

// 5.7
double
lefiAntennaModel::antennaAreaMinusDiff() const
{
    return antennaAreaMinusDiff_;
}

// 5.7
lefiAntennaPWL *
lefiAntennaModel::antennaAreaDiffReducePWL() const
{
    return antennaAreaDiffReducePWL_;
}

// *****************************************************************************
// lefiLayer
// *****************************************************************************
lefiLayer::lefiLayer()
: name_(NULL),
  nameSize_(0),
  type_(NULL),
  typeSize_(0),
  layerType_(NULL),
  hasPitch_(0),
  hasMask_(0),
  hasOffset_(0),
  hasWidth_(0),
  hasArea_(0),
  hasSpacing_(0),
  hasDiagPitch_(0),
  hasDiagWidth_(0),
  hasDiagSpacing_(0),
  hasSpacingName_(NULL),
  hasSpacingLayerStack_(NULL),
  hasSpacingAdjacent_(NULL),
  hasSpacingRange_(NULL),
  hasSpacingUseLengthThreshold_(NULL),
  hasSpacingLengthThreshold_(NULL),
  hasSpacingCenterToCenter_(NULL),
  hasSpacingParallelOverlap_(NULL),
  hasSpacingCutArea_(NULL),
  hasSpacingEndOfLine_(NULL),
  hasSpacingParellelEdge_(NULL),
  hasSpacingTwoEdges_(NULL),
  hasSpacingAdjacentExcept_(NULL),
  hasSpacingSamenet_(NULL),
  hasSpacingSamenetPGonly_(NULL),
  hasArraySpacing_(0),
  hasDirection_(0),
  hasResistance_(0),
  hasCapacitance_(0),
  hasHeight_(0),
  hasWireExtension_(0),
  hasThickness_(0),
  hasShrinkage_(0),
  hasCapMultiplier_(0),
  hasEdgeCap_(0),
  hasAntennaArea_(0),
  hasAntennaLength_(0),
  hasCurrentDensityPoint_(0),
  currentDensity_(0.0),
  pitchX_(0.0),
  pitchY_(0.0),
  offsetX_(0.0),
  offsetY_(0.0),
  diagPitchX_(0.0),
  diagPitchY_(0.0),
  diagWidth_(0.0),
  diagSpacing_(0.0),
  width_(0.0),
  area_(0.0),
  wireExtension_(0.0),
  numSpacings_(0),
  spacingsAllocated_(0),
  maskNumber_(0),
  spacing_(NULL),
  spacingAdjacentCuts_(NULL),
  spacingAdjacentWithin_(NULL),
  spacingCutArea_(NULL),
  rangeMin_(NULL),
  rangeMax_(NULL),
  rangeInfluence_(NULL),
  rangeInfluenceRangeMin_(NULL),
  rangeInfluenceRangeMax_(NULL),
  rangeRangeMin_(NULL),
  rangeRangeMax_(NULL),
  lengthThreshold_(NULL),
  lengthThresholdRangeMin_(NULL),
  lengthThresholdRangeMax_(NULL),
  numMinimumcut_(0),
  minimumcutAllocated_(0),
  minimumcut_(NULL),
  minimumcutWidth_(NULL),
  hasMinimumcutWithin_(NULL),
  minimumcutWithin_(NULL),
  hasMinimumcutConnection_(NULL),
  minimumcutConnection_(NULL),
  hasMinimumcutNumCuts_(NULL),
  minimumcutLength_(NULL),
  minimumcutDistance_(NULL),
  maxwidth_(0.0),
  minwidth_(0.0),
  numMinenclosedarea_(0),
  minenclosedareaAllocated_(0),
  minenclosedarea_(NULL),
  minenclosedareaWidth_(NULL),
  protrusionWidth1_(0.0),
  protrusionLength_(0.0),
  protrusionWidth2_(0.0),
  numMinstep_(0),
  numMinstepAlloc_(0),
  minstep_(NULL),
  minstepType_(NULL),
  minstepLengthsum_(NULL),
  minstepMaxEdges_(NULL),
  minstepMinAdjLength_(NULL),
  minstepMinBetLength_(NULL),
  minstepXSameCorners_(NULL),
  direction_(NULL),
  resistance_(0.0),
  capacitance_(0.0),
  height_(0.0),
  thickness_(0.0),
  shrinkage_(0.0),
  capMultiplier_(0.0),
  edgeCap_(0.0),
  antennaArea_(0.0),
  antennaLength_(0.0),
  numCurrentPoints_(0),
  currentPointsAllocated_(0),
  currentWidths_(NULL),
  current_(NULL),
  numCapacitancePoints_(0),
  capacitancePointsAllocated_(0),
  capacitanceWidths_(NULL),
  capacitances_(NULL),
  numResistancePoints_(0),
  resistancePointsAllocated_(0),
  resistanceWidths_(NULL),
  resistances_(NULL),
  numProps_(0),
  propsAllocated_(0),
  dvalues_(NULL),
  types_(NULL),
  numAccurrents_(0),
  accurrentAllocated_(0),
  accurrents_(NULL),
  numDccurrents_(0),
  dccurrentAllocated_(0),
  dccurrents_(NULL),
  numNums_(0),
  numAllocated_(0),
  nums_(NULL),
  hasAntennaAreaRatio_(0),
  hasAntennaDiffAreaRatio_(0),
  hasAntennaDiffAreaRatioPWL_(0),
  hasAntennaCumAreaRatio_(0),
  hasAntennaCumDiffAreaRatio_(0),
  hasAntennaCumDiffAreaRatioPWL_(0),
  hasAntennaAreaFactor_(0),
  hasAntennaAreaFactorDUO_(0),
  hasAntennaSideAreaRatio_(0),
  hasAntennaDiffSideAreaRatio_(0),
  hasAntennaDiffSideAreaRatioPWL_(0),
  hasAntennaCumSideAreaRatio_(0),
  hasAntennaCumDiffSideAreaRatio_(0),
  hasAntennaCumDiffSideAreaRatioPWL_(0),
  hasAntennaSideAreaFactor_(0),
  hasAntennaSideAreaFactorDUO_(0),
  currentAntennaModel_(0),
  numAntennaModel_(0),
  antennaModelAllocated_(0),
  antennaModel_(NULL),
  hasSlotWireWidth_(0),
  hasSlotWireLength_(0),
  hasSlotWidth_(0),
  hasSlotLength_(0),
  hasMaxAdjacentSlotSpacing_(0),
  hasMaxCoaxialSlotSpacing_(0),
  hasMaxEdgeSlotSpacing_(0),
  hasSplitWireWidth_(0),
  hasMinimumDensity_(0),
  hasMaximumDensity_(0),
  hasDensityCheckWindow_(0),
  hasDensityCheckStep_(0),
  hasFillActiveSpacing_(0),
  hasTwoWidthPRL_(0),
  slotWireWidth_(0.0),
  slotWireLength_(0.0),
  slotWidth_(0.0),
  slotLength_(0.0),
  maxAdjacentSlotSpacing_(0.0),
  maxCoaxialSlotSpacing_(0.0),
  maxEdgeSlotSpacing_(0.0),
  splitWireWidth_(0.0),
  minimumDensity_(0.0),
  maximumDensity_(0.0),
  densityCheckWindowLength_(0.0),
  densityCheckWindowWidth_(0.0),
  densityCheckStep_(0.0),
  fillActiveSpacing_(0.0),
  numSpacingTable_(0),
  spacingTableAllocated_(0),
  spacingTable_(NULL),
  numEnclosure_(0),
  enclosureAllocated_(0),
  enclosureRules_(NULL),
  overhang1_(NULL),
  overhang2_(NULL),
  encminWidth_(NULL),
  cutWithin_(NULL),
  minLength_(NULL),
  numPreferEnclosure_(0),
  preferEnclosureAllocated_(0),
  preferEnclosureRules_(NULL),
  preferOverhang1_(NULL),
  preferOverhang2_(NULL),
  preferMinWidth_(NULL),
  resPerCut_(0.0),
  diagMinEdgeLength_(0.0),
  numMinSize_(0),
  minSizeWidth_(NULL),
  minSizeLength_(NULL),
  eolWidth_(NULL),
  eolWithin_(NULL),
  parSpace_(NULL),
  parWithin_(NULL),
  maxArea_(0.0),
  hasLongArray_(0),
  viaWidth_(0.0),
  cutSpacing_(0.0),
  numArrayCuts_(0),
  arrayCutsAllocated_(0),
  arrayCuts_(NULL),
  arraySpacings_(NULL),
  hasSpacingTableOrtho_(0),
  spacingTableOrtho_(NULL),
  notchLength_(NULL),
  endOfNotchWidth_(NULL),
  minNotchSpacing_(NULL),
  eonotchLength_(NULL)
{
    Init();
}

void
lefiLayer::Init()
{
    name_ = (char*) lefMalloc(16);
    nameSize_ = 16;
    type_ = (char*) lefMalloc(16);
    typeSize_ = 16;
    layerType_ = 0;
    numSpacings_ = 0;
    spacingsAllocated_ = 0;
    numMinimumcut_ = 0;
    minimumcutAllocated_ = 0;
    numMinenclosedarea_ = 0;
    minenclosedareaAllocated_ = 0;
    numCurrentPoints_ = 0;
    currentPointsAllocated_ = 2;
    currentWidths_ = (double*) lefMalloc(sizeof(double) * 2);
    current_ = (double*) lefMalloc(sizeof(double) * 2);
    numResistancePoints_ = 0;
    resistancePointsAllocated_ = 2;
    resistanceWidths_ = (double*) lefMalloc(sizeof(double) * 2);
    resistances_ = (double*) lefMalloc(sizeof(double) * 2);
    numCapacitancePoints_ = 0;
    capacitancePointsAllocated_ = 2;
    capacitanceWidths_ = (double*) lefMalloc(sizeof(double) * 2);
    capacitances_ = (double*) lefMalloc(sizeof(double) * 2);
    numProps_ = 0;
    propsAllocated_ = 1;
    names_ = (char**) lefMalloc(sizeof(char*));
    values_ = (char**) lefMalloc(sizeof(char*));
    dvalues_ = (double*) lefMalloc(sizeof(double));
    types_ = (char*) lefMalloc(sizeof(char));
    numAccurrents_ = 0;
    accurrentAllocated_ = 0;
    numDccurrents_ = 0;
    dccurrentAllocated_ = 0;
    numNums_ = 0;
    numAllocated_ = 0;
    hasTwoWidthPRL_ = 0;
    numSpacingTable_ = 0;
    spacingTableAllocated_ = 0;
    numEnclosure_ = 0;
    enclosureAllocated_ = 0;
    numPreferEnclosure_ = 0;
    preferEnclosureAllocated_ = 0;
    numMinSize_ = 0;
    numMinstepAlloc_ = 0;
    numArrayCuts_ = 0;
    arrayCutsAllocated_ = 0;
    cutSpacing_ = 0;        // Initialize ARRAYSPACING
    currentAntennaModel_ = 0;
    numAntennaModel_ = 0;
    antennaModelAllocated_ = 0;
    antennaModel_ = 0;
    hasSpacingTableOrtho_ = 0;
    spacing_ = 0;
    spacingName_ = 0;
    spacingAdjacentCuts_ = 0;
    spacingAdjacentWithin_ = 0;
    hasSpacingName_ = 0;
    hasSpacingLayerStack_ = 0;
    hasSpacingAdjacent_ = 0;
    hasSpacingCenterToCenter_ = 0;
    hasSpacingParallelOverlap_ = 0;
    hasSpacingEndOfLine_ = 0;
    eolWidth_ = 0;
    eolWithin_ = 0;
    hasSpacingParellelEdge_ = 0;
    parSpace_ = 0;
    parWithin_ = 0;
    hasSpacingTwoEdges_ = 0;
    hasSpacingAdjacentExcept_ = 0;
    hasSpacingSamenet_ = 0;
    hasSpacingSamenetPGonly_ = 0;
    hasSpacingCutArea_ = 0;
    spacingCutArea_ = 0;
    notchLength_ = 0;
    endOfNotchWidth_ = 0;
    minNotchSpacing_ = 0;
    eonotchLength_ = 0;
    rangeMin_ = 0;
    rangeMax_ = 0;
    rangeInfluence_ = 0;
    rangeInfluenceRangeMin_ = 0;
    rangeInfluenceRangeMax_ = 0;
    rangeRangeMin_ = 0;
    rangeRangeMax_ = 0;
    lengthThreshold_ = 0;
    lengthThresholdRangeMin_ = 0;
    lengthThresholdRangeMax_ = 0;
    hasSpacingRange_ = 0;
    hasSpacingUseLengthThreshold_ = 0;
    hasSpacingLengthThreshold_ = 0;
    clear();
}


lefiLayer::lefiLayer(const lefiLayer& prev) 
: name_(NULL),
  nameSize_(0),
  type_(NULL),
  typeSize_(0),
  layerType_(NULL),
  hasPitch_(0),
  hasMask_(0),
  hasOffset_(0),
  hasWidth_(0),
  hasArea_(0),
  hasSpacing_(0),
  hasDiagPitch_(0),
  hasDiagWidth_(0),
  hasDiagSpacing_(0),
  hasSpacingName_(NULL),
  hasSpacingLayerStack_(NULL),
  hasSpacingAdjacent_(NULL),
  hasSpacingRange_(NULL),
  hasSpacingUseLengthThreshold_(NULL),
  hasSpacingLengthThreshold_(NULL),
  hasSpacingCenterToCenter_(NULL),
  hasSpacingParallelOverlap_(NULL),
  hasSpacingCutArea_(NULL),
  hasSpacingEndOfLine_(NULL),
  hasSpacingParellelEdge_(NULL),
  hasSpacingTwoEdges_(NULL),
  hasSpacingAdjacentExcept_(NULL),
  hasSpacingSamenet_(NULL),
  hasSpacingSamenetPGonly_(NULL),
  hasArraySpacing_(0),
  hasDirection_(0),
  hasResistance_(0),
  hasCapacitance_(0),
  hasHeight_(0),
  hasWireExtension_(0),
  hasThickness_(0),
  hasShrinkage_(0),
  hasCapMultiplier_(0),
  hasEdgeCap_(0),
  hasAntennaArea_(0),
  hasAntennaLength_(0),
  hasCurrentDensityPoint_(0),
  currentDensity_(0.0),
  pitchX_(0.0),
  pitchY_(0.0),
  offsetX_(0.0),
  offsetY_(0.0),
  diagPitchX_(0.0),
  diagPitchY_(0.0),
  diagWidth_(0.0),
  diagSpacing_(0.0),
  width_(0.0),
  area_(0.0),
  wireExtension_(0.0),
  numSpacings_(0),
  spacingsAllocated_(0),
  maskNumber_(0),
  spacing_(NULL),
  spacingAdjacentCuts_(NULL),
  spacingAdjacentWithin_(NULL),
  spacingCutArea_(NULL),
  rangeMin_(NULL),
  rangeMax_(NULL),
  rangeInfluence_(NULL),
  rangeInfluenceRangeMin_(NULL),
  rangeInfluenceRangeMax_(NULL),
  rangeRangeMin_(NULL),
  rangeRangeMax_(NULL),
  lengthThreshold_(NULL),
  lengthThresholdRangeMin_(NULL),
  lengthThresholdRangeMax_(NULL),
  numMinimumcut_(0),
  minimumcutAllocated_(0),
  minimumcut_(NULL),
  minimumcutWidth_(NULL),
  hasMinimumcutWithin_(NULL),
  minimumcutWithin_(NULL),
  hasMinimumcutConnection_(NULL),
  minimumcutConnection_(NULL),
  hasMinimumcutNumCuts_(NULL),
  minimumcutLength_(NULL),
  minimumcutDistance_(NULL),
  maxwidth_(0.0),
  minwidth_(0.0),
  numMinenclosedarea_(0),
  minenclosedareaAllocated_(0),
  minenclosedarea_(NULL),
  minenclosedareaWidth_(NULL),
  protrusionWidth1_(0.0),
  protrusionLength_(0.0),
  protrusionWidth2_(0.0),
  numMinstep_(0),
  numMinstepAlloc_(0),
  minstep_(NULL),
  minstepType_(NULL),
  minstepLengthsum_(NULL),
  minstepMaxEdges_(NULL),
  minstepMinAdjLength_(NULL),
  minstepMinBetLength_(NULL),
  minstepXSameCorners_(NULL),
  direction_(NULL),
  resistance_(0.0),
  capacitance_(0.0),
  height_(0.0),
  thickness_(0.0),
  shrinkage_(0.0),
  capMultiplier_(0.0),
  edgeCap_(0.0),
  antennaArea_(0.0),
  antennaLength_(0.0),
  numCurrentPoints_(0),
  currentPointsAllocated_(0),
  currentWidths_(NULL),
  current_(NULL),
  numCapacitancePoints_(0),
  capacitancePointsAllocated_(0),
  capacitanceWidths_(NULL),
  capacitances_(NULL),
  numResistancePoints_(0),
  resistancePointsAllocated_(0),
  resistanceWidths_(NULL),
  resistances_(NULL),
  numProps_(0),
  propsAllocated_(0),
  dvalues_(NULL),
  types_(NULL),
  numAccurrents_(0),
  accurrentAllocated_(0),
  accurrents_(NULL),
  numDccurrents_(0),
  dccurrentAllocated_(0),
  dccurrents_(NULL),
  numNums_(0),
  numAllocated_(0),
  nums_(NULL),
  hasAntennaAreaRatio_(0),
  hasAntennaDiffAreaRatio_(0),
  hasAntennaDiffAreaRatioPWL_(0),
  hasAntennaCumAreaRatio_(0),
  hasAntennaCumDiffAreaRatio_(0),
  hasAntennaCumDiffAreaRatioPWL_(0),
  hasAntennaAreaFactor_(0),
  hasAntennaAreaFactorDUO_(0),
  hasAntennaSideAreaRatio_(0),
  hasAntennaDiffSideAreaRatio_(0),
  hasAntennaDiffSideAreaRatioPWL_(0),
  hasAntennaCumSideAreaRatio_(0),
  hasAntennaCumDiffSideAreaRatio_(0),
  hasAntennaCumDiffSideAreaRatioPWL_(0),
  hasAntennaSideAreaFactor_(0),
  hasAntennaSideAreaFactorDUO_(0),
  currentAntennaModel_(0),
  numAntennaModel_(0),
  antennaModelAllocated_(0),
  antennaModel_(NULL),
  hasSlotWireWidth_(0),
  hasSlotWireLength_(0),
  hasSlotWidth_(0),
  hasSlotLength_(0),
  hasMaxAdjacentSlotSpacing_(0),
  hasMaxCoaxialSlotSpacing_(0),
  hasMaxEdgeSlotSpacing_(0),
  hasSplitWireWidth_(0),
  hasMinimumDensity_(0),
  hasMaximumDensity_(0),
  hasDensityCheckWindow_(0),
  hasDensityCheckStep_(0),
  hasFillActiveSpacing_(0),
  hasTwoWidthPRL_(0),
  slotWireWidth_(0.0),
  slotWireLength_(0.0),
  slotWidth_(0.0),
  slotLength_(0.0),
  maxAdjacentSlotSpacing_(0.0),
  maxCoaxialSlotSpacing_(0.0),
  maxEdgeSlotSpacing_(0.0),
  splitWireWidth_(0.0),
  minimumDensity_(0.0),
  maximumDensity_(0.0),
  densityCheckWindowLength_(0.0),
  densityCheckWindowWidth_(0.0),
  densityCheckStep_(0.0),
  fillActiveSpacing_(0.0),
  numSpacingTable_(0),
  spacingTableAllocated_(0),
  spacingTable_(NULL),
  numEnclosure_(0),
  enclosureAllocated_(0),
  enclosureRules_(NULL), 
  overhang1_(NULL),
  overhang2_(NULL),
  encminWidth_(NULL),
  cutWithin_(NULL),
  minLength_(NULL),
  numPreferEnclosure_(0),
  preferEnclosureAllocated_(0),
  preferEnclosureRules_(NULL),
  preferOverhang1_(NULL),
  preferOverhang2_(NULL),
  preferMinWidth_(NULL),
  resPerCut_(0.0),
  diagMinEdgeLength_(0.0),
  numMinSize_(0),
  minSizeWidth_(NULL),
  minSizeLength_(NULL),
  eolWidth_(NULL),
  eolWithin_(NULL),
  parSpace_(NULL),
  parWithin_(NULL),
  maxArea_(0.0),
  hasLongArray_(0),
  viaWidth_(0.0),
  cutSpacing_(0.0),
  numArrayCuts_(0),
  arrayCutsAllocated_(0),
  arrayCuts_(NULL),
  arraySpacings_(NULL),
  hasSpacingTableOrtho_(0),
  spacingTableOrtho_(NULL),
  notchLength_(NULL),
  endOfNotchWidth_(NULL),
  minNotchSpacing_(NULL),
  eonotchLength_(NULL)
{
    this->Init();

    LEF_COPY_FUNC( nameSize_ );
    LEF_MALLOC_FUNC( name_, char, sizeof(char) * nameSize_ );
//    printf("currentPrev : %s\n", prev.name_);
//    fflush(stdout);

    LEF_COPY_FUNC( typeSize_ );
    LEF_MALLOC_FUNC( type_, char, sizeof(char) * typeSize_ );

//    std::cout << prev.layerType_ << std:: endl;
    LEF_MALLOC_FUNC( layerType_, char, sizeof(char) * (strlen(prev.layerType_)+1) );

    LEF_COPY_FUNC( hasPitch_ );
    LEF_COPY_FUNC( hasMask_ );
    LEF_COPY_FUNC( hasOffset_ );
    LEF_COPY_FUNC( hasWidth_ );
    LEF_COPY_FUNC( hasArea_ );
    LEF_COPY_FUNC( hasSpacing_ );
    LEF_COPY_FUNC( hasDiagPitch_ );
    LEF_COPY_FUNC( hasDiagWidth_ );
    LEF_COPY_FUNC( hasDiagSpacing_ );

    LEF_COPY_FUNC( spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingName_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingLayerStack_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingAdjacent_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingRange_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingUseLengthThreshold_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingLengthThreshold_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingCenterToCenter_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingParallelOverlap_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingCutArea_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingEndOfLine_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingParellelEdge_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingTwoEdges_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingAdjacentExcept_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingSamenet_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( hasSpacingSamenetPGonly_, int, sizeof(int) * spacingsAllocated_ );

    LEF_COPY_FUNC( hasArraySpacing_ );
    LEF_COPY_FUNC( hasDirection_ );
    LEF_COPY_FUNC( hasResistance_ );
    LEF_COPY_FUNC( hasCapacitance_ );
    LEF_COPY_FUNC( hasHeight_ );
    LEF_COPY_FUNC( hasWireExtension_ );
    LEF_COPY_FUNC( hasThickness_ );
    LEF_COPY_FUNC( hasShrinkage_ );
    LEF_COPY_FUNC( hasCapMultiplier_ );
    LEF_COPY_FUNC( hasEdgeCap_ );
    LEF_COPY_FUNC( hasAntennaArea_ );
    LEF_COPY_FUNC( hasAntennaLength_ );
    LEF_COPY_FUNC( hasCurrentDensityPoint_ );
    LEF_COPY_FUNC( currentDensity_ );
    LEF_COPY_FUNC( pitchX_ );
    LEF_COPY_FUNC( pitchY_ );
    LEF_COPY_FUNC( offsetX_ );
    LEF_COPY_FUNC( offsetY_ );
    LEF_COPY_FUNC( diagPitchX_ );
    LEF_COPY_FUNC( diagPitchY_ );
    LEF_COPY_FUNC( diagWidth_ );
    LEF_COPY_FUNC( diagSpacing_ );
    LEF_COPY_FUNC( width_ );
    LEF_COPY_FUNC( area_ );
    LEF_COPY_FUNC( wireExtension_ );
    LEF_COPY_FUNC( numSpacings_ );

    LEF_COPY_FUNC( maskNumber_ );
    LEF_MALLOC_FUNC( spacing_, double, sizeof(double) * spacingsAllocated_ );

    //!!
    LEF_MALLOC_FUNC_FOR_2D_STR(spacingName_, numSpacings_);
    /*
    if(prev.spacingName_) {                                                  
        spacingName_ = (char**) lefMalloc( sizeof(char*) * spacingsAllocated_ );         
                                                                        
        for(int i=0; i<prev.spacingsAllocated_; i++) {                              
            if( prev.spacingName_[i] ) {                                     
                int len = sizeof(char) * strlen( prev.spacingName_[i] );     
                spacingName_[i] = (char*) lefMalloc( len );                  
                strcpy( spacingName_[i], prev.spacingName_[i] );                  
            }                                                           
            else {                                                      
                spacingName_[i] = 0;                                         
            }                                                           
        }                                                               
    }                                                                   
    else {                                                              
        spacingName_ = 0;                                                    
    } */                                                                  


    LEF_MALLOC_FUNC( spacingAdjacentCuts_, int, sizeof(int) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( spacingAdjacentWithin_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( spacingCutArea_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( rangeMin_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( rangeMax_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( rangeInfluence_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( rangeInfluenceRangeMin_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( rangeInfluenceRangeMax_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( rangeRangeMin_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( rangeRangeMax_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( lengthThreshold_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( lengthThresholdRangeMin_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( lengthThresholdRangeMax_, double, sizeof(double) * spacingsAllocated_ );

    LEF_COPY_FUNC( numMinimumcut_ );
    LEF_COPY_FUNC( minimumcutAllocated_ );
    LEF_MALLOC_FUNC( minimumcut_, int, sizeof(int) * minimumcutAllocated_ );
    LEF_MALLOC_FUNC( minimumcutWidth_, double, sizeof(double) * minimumcutAllocated_ );
    LEF_MALLOC_FUNC( hasMinimumcutWithin_, int, sizeof(int) * minimumcutAllocated_ );
    LEF_MALLOC_FUNC( minimumcutWithin_, double, sizeof(double) * minimumcutAllocated_ );
    LEF_MALLOC_FUNC( hasMinimumcutConnection_, int, sizeof(int) * minimumcutAllocated_ );

    //!!
    LEF_MALLOC_FUNC_FOR_2D_STR(minimumcutConnection_, numMinimumcut_);
   
   /* 
    if(prev.minimumcutConnection_) {                                                  
        minimumcutConnection_ = (char**) lefMalloc( sizeof(char*) * numMinimumcut_ );         
                                                                        
        for(int i=0; i<prev.numMinimumcut_; i++) {                              
            if( prev.minimumcutConnection_[i] ) {                                     
                int len = sizeof(char) * strlen( prev.minimumcutConnection_[i] );     
                minimumcutConnection_[i] = (char*) lefMalloc( len );                  
                strcpy( minimumcutConnection_[i], prev.minimumcutConnection_[i] );                  
            }                                                           
            else {                                                      
                minimumcutConnection_[i] = 0;                                         
            }                                                           
        }                                                               
    }                                                                   
    else {                                                              
        minimumcutConnection_ = 0;                                                    
    }
    */

    LEF_MALLOC_FUNC( hasMinimumcutNumCuts_, int, sizeof(int)* minimumcutAllocated_ );
    LEF_MALLOC_FUNC( minimumcutLength_, double, sizeof(double)* minimumcutAllocated_ );
    LEF_MALLOC_FUNC( minimumcutDistance_, double, sizeof(double)* minimumcutAllocated_ );
    LEF_COPY_FUNC( maxwidth_ );
    LEF_COPY_FUNC( minwidth_ );
    LEF_COPY_FUNC( numMinenclosedarea_ );
    LEF_COPY_FUNC( minenclosedareaAllocated_ );
    LEF_MALLOC_FUNC( minenclosedarea_, double, sizeof(double) *minenclosedareaAllocated_ );
    LEF_MALLOC_FUNC( minenclosedareaWidth_, double, sizeof(double) *minenclosedareaAllocated_ );
    LEF_COPY_FUNC( protrusionWidth1_ );
    LEF_COPY_FUNC( protrusionLength_ );
    LEF_COPY_FUNC( protrusionWidth2_ );
    LEF_COPY_FUNC( numMinstep_ );
    LEF_COPY_FUNC( numMinstepAlloc_ );
    LEF_MALLOC_FUNC( minstep_, double, sizeof(double) * numMinstepAlloc_ );

    //!!
    LEF_MALLOC_FUNC_FOR_2D_STR(minstepType_, numMinstep_);

    LEF_MALLOC_FUNC( minstepLengthsum_, double, sizeof(double)* numMinstepAlloc_ );
    LEF_MALLOC_FUNC( minstepMaxEdges_, int, sizeof(int)* numMinstepAlloc_ );
    LEF_MALLOC_FUNC( minstepMinAdjLength_, double, sizeof(double)* numMinstepAlloc_ );
    LEF_MALLOC_FUNC( minstepMinBetLength_, double, sizeof(double)* numMinstepAlloc_ );
    LEF_MALLOC_FUNC( minstepXSameCorners_, int, sizeof(int)* numMinstepAlloc_ );
    LEF_MALLOC_FUNC( direction_, char, sizeof(char) * (strlen(prev.direction_)+1));
    LEF_COPY_FUNC( resistance_ );
    LEF_COPY_FUNC( capacitance_ );
    LEF_COPY_FUNC( height_ );
    LEF_COPY_FUNC( thickness_ );
    LEF_COPY_FUNC( shrinkage_ );
    LEF_COPY_FUNC( capMultiplier_ );
    LEF_COPY_FUNC( edgeCap_ );
    LEF_COPY_FUNC( antennaArea_ );
    LEF_COPY_FUNC( antennaLength_ );
    LEF_COPY_FUNC( numCurrentPoints_ );
    LEF_COPY_FUNC( currentPointsAllocated_ );
    LEF_MALLOC_FUNC( currentWidths_, double, sizeof(double) * currentPointsAllocated_ );
    LEF_MALLOC_FUNC( current_, double, sizeof(double) * currentPointsAllocated_ );
    LEF_COPY_FUNC( numCapacitancePoints_ );
    LEF_COPY_FUNC( capacitancePointsAllocated_ );
    LEF_MALLOC_FUNC( capacitanceWidths_, double, sizeof(double) * capacitancePointsAllocated_ );
    LEF_MALLOC_FUNC( capacitances_, double, sizeof(double) * capacitancePointsAllocated_);
    LEF_COPY_FUNC( numResistancePoints_ );
    LEF_COPY_FUNC( resistancePointsAllocated_ );
    LEF_MALLOC_FUNC( resistanceWidths_, double, sizeof(double) * resistancePointsAllocated_ );
    LEF_MALLOC_FUNC( resistances_, double, sizeof(double) * resistancePointsAllocated_);
    LEF_COPY_FUNC( numProps_ );
    LEF_COPY_FUNC( propsAllocated_ );

    //!!
    LEF_MALLOC_FUNC_FOR_2D_STR(names_, numProps_);
    //!!
    LEF_MALLOC_FUNC_FOR_2D_STR(values_, numProps_);

    LEF_MALLOC_FUNC( dvalues_, double, sizeof(double) * propsAllocated_ );
    LEF_MALLOC_FUNC( types_, char, sizeof(char));
//    printf("TYPES: %s\n", types_);
//    fflush(stdout);
    LEF_COPY_FUNC( numAccurrents_ );
    LEF_COPY_FUNC( accurrentAllocated_ );

    // !!
    LEF_MALLOC_FUNC( accurrents_, lefiLayerDensity*, sizeof(lefiLayerDensity*) * accurrentAllocated_ );
    LEF_COPY_FUNC( numDccurrents_ );
    LEF_COPY_FUNC( dccurrentAllocated_ );

    // !!
    LEF_MALLOC_FUNC( dccurrents_, lefiLayerDensity*, sizeof(lefiLayerDensity*) * dccurrentAllocated_ );
    LEF_COPY_FUNC( numNums_ );
    LEF_COPY_FUNC( numAllocated_ );
    LEF_MALLOC_FUNC( nums_, double, sizeof(double)* numAllocated_ );

    LEF_COPY_FUNC( hasAntennaAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaDiffAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaDiffAreaRatioPWL_ );
    LEF_COPY_FUNC( hasAntennaCumAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaCumDiffAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaCumDiffAreaRatioPWL_ );
    LEF_COPY_FUNC( hasAntennaAreaFactor_ );
    LEF_COPY_FUNC( hasAntennaAreaFactorDUO_ );
    LEF_COPY_FUNC( hasAntennaSideAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaDiffSideAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaDiffSideAreaRatioPWL_ );
    LEF_COPY_FUNC( hasAntennaCumSideAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaCumDiffSideAreaRatio_ );
    LEF_COPY_FUNC( hasAntennaCumDiffSideAreaRatioPWL_ );
    LEF_COPY_FUNC( hasAntennaSideAreaFactor_ );
    LEF_COPY_FUNC( hasAntennaSideAreaFactorDUO_ );

    LEF_MALLOC_FUNC( currentAntennaModel_, lefiAntennaModel, sizeof(lefiAntennaModel) * 1 );
    LEF_COPY_FUNC( numAntennaModel_ );
    LEF_COPY_FUNC( antennaModelAllocated_ );

    // !!
    LEF_MALLOC_FUNC( antennaModel_, lefiAntennaModel*, sizeof(lefiAntennaModel*) * antennaModelAllocated_ );
    LEF_COPY_FUNC( hasSlotWireWidth_ );
    LEF_COPY_FUNC( hasSlotWireLength_ );
    LEF_COPY_FUNC( hasSlotWidth_ );
    LEF_COPY_FUNC( hasSlotLength_ );
    LEF_COPY_FUNC( hasMaxAdjacentSlotSpacing_ );
    LEF_COPY_FUNC( hasMaxCoaxialSlotSpacing_ );
    LEF_COPY_FUNC( hasMaxEdgeSlotSpacing_ );
    LEF_COPY_FUNC( hasSplitWireWidth_ );
    LEF_COPY_FUNC( hasMinimumDensity_ );
    LEF_COPY_FUNC( hasMaximumDensity_ );
    LEF_COPY_FUNC( hasDensityCheckWindow_ );
    LEF_COPY_FUNC( hasDensityCheckStep_ );
    LEF_COPY_FUNC( hasFillActiveSpacing_ );
    LEF_COPY_FUNC( hasTwoWidthPRL_ );
    LEF_COPY_FUNC( slotWireWidth_ );
    LEF_COPY_FUNC( slotWireLength_ );
    LEF_COPY_FUNC( slotWidth_ );
    LEF_COPY_FUNC( slotLength_ );
    LEF_COPY_FUNC( maxAdjacentSlotSpacing_ );
    LEF_COPY_FUNC( maxCoaxialSlotSpacing_ );
    LEF_COPY_FUNC( maxEdgeSlotSpacing_ );
    LEF_COPY_FUNC( splitWireWidth_ );
    LEF_COPY_FUNC( minimumDensity_ );
    LEF_COPY_FUNC( maximumDensity_ );
    LEF_COPY_FUNC( densityCheckWindowLength_ );
    LEF_COPY_FUNC( densityCheckWindowWidth_ );
    LEF_COPY_FUNC( densityCheckStep_ );
    LEF_COPY_FUNC( fillActiveSpacing_ );
    LEF_COPY_FUNC( numSpacingTable_ );
    LEF_COPY_FUNC( spacingTableAllocated_ );

    // !!
    LEF_MALLOC_FUNC_FOR_2D( spacingTable_, lefiSpacingTable, numSpacingTable_, 1 );
    LEF_COPY_FUNC( numEnclosure_ );
    LEF_COPY_FUNC( enclosureAllocated_ );

    // !!
    LEF_MALLOC_FUNC_FOR_2D_STR( enclosureRules_ , numEnclosure_);

    LEF_MALLOC_FUNC( overhang1_, double, sizeof(double) * enclosureAllocated_ );
    LEF_MALLOC_FUNC( overhang2_, double, sizeof(double) * enclosureAllocated_ );
    LEF_MALLOC_FUNC( encminWidth_, double, sizeof(double) * enclosureAllocated_);
    LEF_MALLOC_FUNC( cutWithin_, double, sizeof(double) * enclosureAllocated_ );
    LEF_MALLOC_FUNC( minLength_, double, sizeof(double) * enclosureAllocated_ );
    LEF_COPY_FUNC( numPreferEnclosure_ );
    LEF_COPY_FUNC( preferEnclosureAllocated_ );

    //!!
    LEF_MALLOC_FUNC_FOR_2D_STR( preferEnclosureRules_, numPreferEnclosure_);

    LEF_MALLOC_FUNC( preferOverhang1_, double, sizeof(double) * preferEnclosureAllocated_ );
    LEF_MALLOC_FUNC( preferOverhang2_, double, sizeof(double) * preferEnclosureAllocated_ );
    LEF_MALLOC_FUNC( preferMinWidth_, double, sizeof(double) * preferEnclosureAllocated_ );
    LEF_COPY_FUNC( resPerCut_ );
    LEF_COPY_FUNC( diagMinEdgeLength_ );
    LEF_COPY_FUNC( numMinSize_ );
    LEF_MALLOC_FUNC( minSizeWidth_, double, sizeof(double) * numMinSize_ );
    LEF_MALLOC_FUNC( minSizeLength_, double, sizeof(double) * numMinSize_ );
    LEF_MALLOC_FUNC( eolWidth_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( eolWithin_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( parSpace_, double, sizeof(double)  * spacingsAllocated_ );
    LEF_MALLOC_FUNC( parWithin_, double, sizeof(double) * spacingsAllocated_ );
    LEF_COPY_FUNC( maxArea_ );
    LEF_COPY_FUNC( hasLongArray_ );
    LEF_COPY_FUNC( viaWidth_ );
    LEF_COPY_FUNC( cutSpacing_ );
    LEF_COPY_FUNC( numArrayCuts_ );
    LEF_COPY_FUNC( arrayCutsAllocated_ );
    LEF_MALLOC_FUNC( arrayCuts_, int, sizeof(int) * arrayCutsAllocated_ );
    LEF_MALLOC_FUNC( arraySpacings_, double, sizeof(double) * arrayCutsAllocated_ );
    LEF_COPY_FUNC( hasSpacingTableOrtho_ );

    LEF_MALLOC_FUNC_WITH_OPERATOR( spacingTableOrtho_, lefiOrthogonal, sizeof(lefiOrthogonal) );

    LEF_MALLOC_FUNC( notchLength_, double, sizeof(double) * spacingsAllocated_);
    LEF_MALLOC_FUNC( endOfNotchWidth_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( minNotchSpacing_, double, sizeof(double) * spacingsAllocated_ );
    LEF_MALLOC_FUNC( eonotchLength_, double, sizeof(double)  * spacingsAllocated_ ); 
    LEF_COPY_FUNC( lef58WidthTableOrthoValues_ );
    LEF_COPY_FUNC( lef58WidthTableWrongDirValues_ );
//    printf("constructor finish\n");
//    fflush(stdout);
//    LEF_MALLOC_FUNC( lef58WidthTableOrtho_, double, sizeof(double) );
//    LEF_MALLOC_FUNC( lef58WidthTableWrongDir_, double, sizeof(double) );


}

void
lefiLayer::Destroy()
{

    clear();
    lefFree(name_);
    nameSize_ = 0;
    lefFree(type_);
    typeSize_ = 0;
    if (spacing_)
        lefFree((char*) (spacing_));
    spacing_ = 0;
    if (spacingTable_)
        lefFree((char*) spacingTable_);
    spacingTable_ = 0;
    if (spacingName_)
        lefFree((char*) (spacingName_));
    spacingName_ = 0;
    if (spacingAdjacentCuts_)
        lefFree((char*) (spacingAdjacentCuts_));
    spacingAdjacentCuts_ = 0;
    if (spacingAdjacentWithin_)
        lefFree((char*) (spacingAdjacentWithin_));
    spacingAdjacentWithin_ = 0;
    if (hasSpacingName_)
        lefFree((char*) (hasSpacingName_));
    hasSpacingName_ = 0;
    if (hasSpacingLayerStack_)
        lefFree((char*) (hasSpacingLayerStack_));
    hasSpacingLayerStack_ = 0;
    if (hasSpacingAdjacent_)
        lefFree((char*) (hasSpacingAdjacent_));
    hasSpacingAdjacent_ = 0;
    if (hasSpacingCenterToCenter_)
        lefFree((char*) (hasSpacingCenterToCenter_));
    hasSpacingCenterToCenter_ = 0;
    if (hasSpacingParallelOverlap_)
        lefFree((char*) (hasSpacingParallelOverlap_));
    hasSpacingParallelOverlap_ = 0;
    if (hasSpacingEndOfLine_)
        lefFree((char*) (hasSpacingEndOfLine_));
    hasSpacingEndOfLine_ = 0;
    if (eolWidth_)
        lefFree((char*) (eolWidth_));
    eolWidth_ = 0;
    if (eolWithin_)
        lefFree((char*) (eolWithin_));
    eolWithin_ = 0;
    if (hasSpacingParellelEdge_)
        lefFree((char*) (hasSpacingParellelEdge_));
    hasSpacingParellelEdge_ = 0;
    if (parSpace_)
        lefFree((char*) (parSpace_));
    parSpace_ = 0;
    if (parWithin_)
        lefFree((char*) (parWithin_));
    parWithin_ = 0;
    if (hasSpacingTwoEdges_)
        lefFree((char*) (hasSpacingTwoEdges_));
    hasSpacingTwoEdges_ = 0;
    if (hasSpacingAdjacentExcept_)
        lefFree((char*) (hasSpacingAdjacentExcept_));
    hasSpacingAdjacentExcept_ = 0;
    if (hasSpacingSamenet_)
        lefFree((char*) (hasSpacingSamenet_));
    hasSpacingSamenet_ = 0;
    if (hasSpacingSamenetPGonly_)
        lefFree((char*) (hasSpacingSamenetPGonly_));
    hasSpacingSamenetPGonly_ = 0;
    if (hasSpacingCutArea_)
        lefFree((char*) (hasSpacingCutArea_));
    hasSpacingCutArea_ = 0;
    if (spacingCutArea_)
        lefFree((char*) (spacingCutArea_));
    spacingCutArea_ = 0;
    if (notchLength_)
        lefFree((char*) (notchLength_));
    notchLength_ = 0;
    if (endOfNotchWidth_)
        lefFree((char*) (endOfNotchWidth_));
    endOfNotchWidth_ = 0;
    if (minNotchSpacing_)
        lefFree((char*) (minNotchSpacing_));
    minNotchSpacing_ = 0;
    if (eonotchLength_)
        lefFree((char*) (eonotchLength_));
    eonotchLength_ = 0;
    if (rangeMin_)
        lefFree((char*) (rangeMin_));
    rangeMin_ = 0;
    if (rangeMax_)
        lefFree((char*) (rangeMax_));
    rangeMax_ = 0;
    if (rangeInfluence_)
        lefFree((char*) (rangeInfluence_));
    rangeInfluence_ = 0;
    if (rangeInfluenceRangeMin_)
        lefFree((char*) (rangeInfluenceRangeMin_));
    rangeInfluenceRangeMin_ = 0;
    if (rangeInfluenceRangeMax_)
        lefFree((char*) (rangeInfluenceRangeMax_));
    rangeInfluenceRangeMax_ = 0;
    if (rangeRangeMin_)
        lefFree((char*) (rangeRangeMin_));
    rangeRangeMin_ = 0;
    if (rangeRangeMax_)
        lefFree((char*) (rangeRangeMax_));
    rangeRangeMax_ = 0;
    if (lengthThreshold_)
        lefFree((char*) (lengthThreshold_));
    lengthThreshold_ = 0;
    if (lengthThresholdRangeMin_)
        lefFree((char*) (lengthThresholdRangeMin_));
    lengthThresholdRangeMin_ = 0;
    if (lengthThresholdRangeMax_)
        lefFree((char*) (lengthThresholdRangeMax_));
    lengthThresholdRangeMax_ = 0;
    if (hasSpacingRange_)
        lefFree((char*) (hasSpacingRange_));
    hasSpacingRange_ = 0;
    if (hasSpacingUseLengthThreshold_)
        lefFree((char*) (hasSpacingUseLengthThreshold_));
    hasSpacingUseLengthThreshold_ = 0;
    if (hasSpacingLengthThreshold_)
        lefFree((char*) (hasSpacingLengthThreshold_));
    hasSpacingLengthThreshold_ = 0;
    lefFree((char*) (currentWidths_));
    lefFree((char*) (current_));
    lefFree((char*) (resistanceWidths_));
    lefFree((char*) (resistances_));
    lefFree((char*) (capacitanceWidths_));
    lefFree((char*) (capacitances_));
    lefFree((char*) (names_));
    lefFree((char*) (values_));
    lefFree((char*) (dvalues_));
    lefFree((char*) (types_));
    currentPointsAllocated_ = 0;
    resistancePointsAllocated_ = 0;
    capacitancePointsAllocated_ = 0;
    propsAllocated_ = 0;
}

lefiLayer::~lefiLayer()
{
//    Destroy();
}

void
lefiLayer::clear()
{
    int                 i;
    lefiLayerDensity    *p;
    lefiSpacingTable    *sp;
    lefiAntennaModel    *aO;         // 5.5

    if (name_)
        *(name_) = 0;
    if (type_)
        *(type_) = 0;
    if (layerType_) {
        lefFree(layerType_);
        layerType_ = 0;
    }
    hasMask_ = 0;
    hasPitch_ = 0;
    hasOffset_ = 0;
    hasWidth_ = 0;
    hasArea_ = 0;
    hasDiagPitch_ = 0;
    hasDiagWidth_ = 0;
    hasDiagSpacing_ = 0;
    hasWireExtension_ = 0;
    hasSpacing_ = 0;
    hasDirection_ = 0;
    hasResistance_ = 0;
    hasCapacitance_ = 0;
    hasHeight_ = 0;
    hasThickness_ = 0;
    hasShrinkage_ = 0;
    hasCapMultiplier_ = 0;
    hasEdgeCap_ = 0;
    hasAntennaArea_ = 0;
    hasAntennaLength_ = 0;
    hasCurrentDensityPoint_ = 0;
    for (i = 0; i < numSpacings_; i++)
        if (spacingName_[i])
            lefFree(spacingName_[i]);
    for (i = 0; i < numProps_; i++) {
        if (names_[i])
            lefFree(names_[i]);
        if (values_[i])
            lefFree(values_[i]);
        dvalues_[i] = 0;
    }
    numProps_ = 0;
    numSpacings_ = 0;
    numCurrentPoints_ = 0;
    if (numAccurrents_) {
        for (i = 0; i < numAccurrents_; i++) {
            p = accurrents_[i];
            p->Destroy();
            lefFree((char*) p);
        }
        numAccurrents_ = 0;
        accurrentAllocated_ = 0;
        lefFree((char*) (accurrents_));
        accurrents_ = 0;
    }
    if (numDccurrents_) {
        for (i = 0; i < numDccurrents_; i++) {
            p = dccurrents_[i];
            p->Destroy();
            lefFree((char*) p);
        }
        numDccurrents_ = 0;
        dccurrentAllocated_ = 0;
        lefFree((char*) (dccurrents_));
        dccurrents_ = 0;
    }
    // 8/29/2001 - Wanda da Rosa.  The following are 5.4 enhancements
    hasSlotWireWidth_ = 0;
    hasSlotWireLength_ = 0;
    hasSlotWidth_ = 0;
    hasSlotLength_ = 0;
    hasMaxAdjacentSlotSpacing_ = 0;
    hasMaxCoaxialSlotSpacing_ = 0;
    hasMaxEdgeSlotSpacing_ = 0;
    hasSplitWireWidth_ = 0;
    hasMinimumDensity_ = 0;
    hasMaximumDensity_ = 0;
    hasDensityCheckWindow_ = 0;
    hasDensityCheckStep_ = 0;
    hasTwoWidthPRL_ = 0;
    hasFillActiveSpacing_ = 0;
    // 5.5
    if (numMinimumcut_ > 0) {
        // Has allocated memories
        lefFree((char*) (minimumcut_));
        lefFree((char*) (minimumcutWidth_));
        lefFree((char*) (hasMinimumcutWithin_));
        lefFree((char*) (minimumcutWithin_));
        lefFree((char*) (hasMinimumcutConnection_));
        lefFree((char*) (hasMinimumcutNumCuts_));
        lefFree((char*) (minimumcutLength_));
        lefFree((char*) (minimumcutDistance_));
        for (i = 0; i < numMinimumcut_; i++) {
            if (minimumcutConnection_[i])
                lefFree((char*) (minimumcutConnection_[i]));
        }
        lefFree((char*) (minimumcutConnection_));
        numMinimumcut_ = 0;
        minimumcutAllocated_ = 0;
    }
    maxwidth_ = -1;
    minwidth_ = -1;
    if (numMinenclosedarea_ > 0) {
        lefFree((char*) (minenclosedarea_));
        lefFree((char*) (minenclosedareaWidth_));
        numMinenclosedarea_ = 0;
        minenclosedareaAllocated_ = 0;
    }
    if (numMinstepAlloc_ > 0) {
        for (i = 0; i < numMinstep_; i++) { // 5.6
            lefFree((char*) (minstepType_[i]));
        }
        lefFree((char*) (minstep_));
        lefFree((char*) (minstepType_));
        lefFree((char*) (minstepLengthsum_));
        lefFree((char*) (minstepMaxEdges_));
        lefFree((char*) (minstepMinAdjLength_));
        lefFree((char*) (minstepMinBetLength_));
        lefFree((char*) (minstepXSameCorners_));
    }
    numMinstepAlloc_ = 0;
    numMinstep_ = 0;
    protrusionWidth1_ = -1;
    protrusionLength_ = -1;
    protrusionWidth2_ = -1;
    if (numSpacingTable_) {
        for (i = 0; i < numSpacingTable_; i++) {
            sp = spacingTable_[i];
            sp->Destroy();
            lefFree((char*) sp);
        }
    }
    numSpacingTable_ = 0;
    spacingTableAllocated_ = 0;

    for (i = 0; i < numAntennaModel_; i++) {  // 5.5
        aO = antennaModel_[i];
        aO->Destroy();
    }
    for (i = 0; i < antennaModelAllocated_; i++) { // 5.5
        lefFree((char*) antennaModel_[i]);
    }

    if (antennaModel_)                        // 5.5
        lefFree((char*) (antennaModel_));
    antennaModel_ = 0;
    currentAntennaModel_ = 0;
    numAntennaModel_ = 0;
    antennaModelAllocated_ = 0;

    if (nums_)
        lefFree((char*) (nums_));

    // 5.6
    if (numEnclosure_) {
        for (i = 0; i < numEnclosure_; i++) {
            if (enclosureRules_[i])
                lefFree((char*) (enclosureRules_[i]));
        }
        lefFree((char*) (enclosureRules_));
        lefFree((char*) (overhang1_));
        lefFree((char*) (overhang2_));
        lefFree((char*) (encminWidth_));
        lefFree((char*) (cutWithin_));
        lefFree((char*) (minLength_));
        numEnclosure_ = 0;
        enclosureAllocated_ = 0;
    }
    if (numPreferEnclosure_) {
        for (i = 0; i < numPreferEnclosure_; i++) {
            if (preferEnclosureRules_[i])
                lefFree((char*) (preferEnclosureRules_[i]));
        }
        lefFree((char*) (preferEnclosureRules_));
        lefFree((char*) (preferOverhang1_));
        lefFree((char*) (preferOverhang2_));
        lefFree((char*) (preferMinWidth_));
        numPreferEnclosure_ = 0;
        preferEnclosureAllocated_ = 0;
    }
    resPerCut_ = 0;
    diagMinEdgeLength_ = 0;
    if (numMinSize_) {
        lefFree((char*) (minSizeWidth_));
        lefFree((char*) (minSizeLength_));
        numMinSize_ = 0;
    }
    maxArea_ = 0;
    hasLongArray_ = 0;
    viaWidth_ = 0;
    cutSpacing_ = 0;
    if (numArrayCuts_) {
        lefFree((char*) (arrayCuts_));
        lefFree((char*) (arraySpacings_));
    }
    arrayCuts_ = 0;
    arraySpacings_ = 0;
    arrayCutsAllocated_ = 0;
    numArrayCuts_ = 0;

    // 5.7
    if (hasSpacingTableOrtho_) {
        spacingTableOrtho_->Destroy();
        lefFree((char*) (spacingTableOrtho_));
        spacingTableOrtho_ = 0;
    }
    hasSpacingTableOrtho_ = 0;
}


void
lefiLayer::setName(const char *name)
{
    int len = strlen(name) + 1;
    clear();
    if (len > nameSize_) {
        lefFree(name_);
        name_ = (char*) lefMalloc(len);
        nameSize_ = len;
    }
    strcpy(name_, CASE(name));
}

void
lefiLayer::setType(const char *typ)
{
    int len = strlen(typ) + 1;
    if (len > typeSize_) {
        lefFree(type_);
        type_ = (char*) lefMalloc(len);
        typeSize_ = len;
    }
    strcpy(type_, CASE(typ));
}

// 5.8
void
lefiLayer::setLayerType(const char *layerType)
{
    if (layerType_)
        lefFree(layerType_);
    layerType_ = strdup(layerType);
}

void
lefiLayer::setPitch(double num)
{
    hasPitch_ = 1;
    pitchX_ = num;
    pitchY_ = -1;
}

// 5.6
void
lefiLayer::setPitchXY(double    xdist,
                      double    ydist)
{
    hasPitch_ = 2;             // 2 means it has X & Y values
    pitchX_ = xdist;
    pitchY_ = ydist;
}

void
lefiLayer::setMask(int num)
{
    hasMask_ = 1;
    maskNumber_ = num;
}

void
lefiLayer::setOffset(double num)
{
    hasOffset_ = 1;
    offsetX_ = num;
    offsetY_ = -1;
}

// 5.6
void
lefiLayer::setOffsetXY(double   xdist,
                       double   ydist)
{
    hasOffset_ = 2;             // 2 means it has X & Y values
    offsetX_ = xdist;
    offsetY_ = ydist;
}

void
lefiLayer::setWidth(double num)
{
    hasWidth_ = 1;
    width_ = num;
}

void
lefiLayer::setArea(double num)
{
    hasArea_ = 1;
    area_ = num;
}

// 5.6
void
lefiLayer::setDiagPitch(double dist)
{
    hasDiagPitch_ = 1;
    diagPitchX_ = dist;
    diagPitchY_ = -1;
}

// 5.6
void
lefiLayer::setDiagPitchXY(double    xdist,
                          double    ydist)
{
    hasDiagPitch_ = 2;
    diagPitchX_ = xdist;
    diagPitchY_ = ydist;
}

// 5.6
void
lefiLayer::setDiagWidth(double width)
{
    hasDiagWidth_ = 1;
    diagWidth_ = width;
}

// 5.6
void
lefiLayer::setDiagSpacing(double spacing)
{
    hasDiagSpacing_ = 1;
    diagSpacing_ = spacing;
}

void
lefiLayer::setWireExtension(double num)
{
    hasWireExtension_ = 1;
    wireExtension_ = num;
}

// 5.5
void
lefiLayer::setMaxwidth(double width)
{
    maxwidth_ = width;
}

// 5.5
void
lefiLayer::setMinwidth(double width)
{
    minwidth_ = width;
}

// 5.5
void
lefiLayer::addMinenclosedarea(double area)
{
    if (numMinenclosedarea_ == minenclosedareaAllocated_) {
        double  *na;
        double  *nw;
        int     i, lim;

        if (minenclosedareaAllocated_ == 0) {
            lim = minenclosedareaAllocated_ = 2;
            na = (double*) lefMalloc(sizeof(double) * lim);
            nw = (double*) lefMalloc(sizeof(double) * lim);
        } else {
            lim = minenclosedareaAllocated_ * 2;
            minenclosedareaAllocated_ = lim;
            na = (double*) lefMalloc(sizeof(double) * lim);
            nw = (double*) lefMalloc(sizeof(double) * lim);
            lim /= 2;
            for (i = 0; i < lim; i++) {
                na[i] = minenclosedarea_[i];
                nw[i] = minenclosedareaWidth_[i];
            }
            lefFree((char*) (minenclosedarea_));
            lefFree((char*) (minenclosedareaWidth_));
        }
        minenclosedarea_ = na;
        minenclosedareaWidth_ = nw;
    }
    minenclosedarea_[numMinenclosedarea_] = area;
    minenclosedareaWidth_[numMinenclosedarea_] = -1;
    numMinenclosedarea_ += 1;
}

// 5.5
void
lefiLayer::addMinenclosedareaWidth(double width)
{
    minenclosedareaWidth_[numMinenclosedarea_ - 1] = width;
}

// 5.5
void
lefiLayer::addMinimumcut(int    mincut,
                         double width)
{
    if (numMinimumcut_ == minimumcutAllocated_) {
        int     *nc;
        double  *nw;
        int     *hcd;
        double  *ncd;
        int     *hm;
        char    **nud;
        int     *hc;
        double  *nl;
        double  *nd;
        int     i, lim;

        if (minimumcutAllocated_ == 0) {
            lim = minimumcutAllocated_ = 2;
            nc = (int*) lefMalloc(sizeof(int) * lim);
            nw = (double*) lefMalloc(sizeof(double) * lim);
            hcd = (int*) lefMalloc(sizeof(int) * lim);
            ncd = (double*) lefMalloc(sizeof(double) * lim);
            hm = (int*) lefMalloc(sizeof(int) * lim);
            nud = (char**) lefMalloc(sizeof(char*) * lim);
            hc = (int*) lefMalloc(sizeof(int) * lim);
            nl = (double*) lefMalloc(sizeof(double) * lim);
            nd = (double*) lefMalloc(sizeof(double) * lim);
        } else {
            lim = minimumcutAllocated_ * 2;
            minimumcutAllocated_ = lim;
            nc = (int*) lefMalloc(sizeof(int) * lim);
            nw = (double*) lefMalloc(sizeof(double) * lim);
            hcd = (int*) lefMalloc(sizeof(int) * lim);
            ncd = (double*) lefMalloc(sizeof(double) * lim);
            hm = (int*) lefMalloc(sizeof(int) * lim);
            nud = (char**) lefMalloc(sizeof(char*) * lim);
            hc = (int*) lefMalloc(sizeof(int) * lim);
            nl = (double*) lefMalloc(sizeof(double) * lim);
            nd = (double*) lefMalloc(sizeof(double) * lim);
            lim /= 2;
            for (i = 0; i < lim; i++) {
                nc[i] = minimumcut_[i];
                nw[i] = minimumcutWidth_[i];
                hcd[i] = hasMinimumcutWithin_[i];
                ncd[i] = minimumcutWithin_[i];
                hm[i] = hasMinimumcutConnection_[i];
                nud[i] = minimumcutConnection_[i];
                hc[i] = hasMinimumcutNumCuts_[i];
                nl[i] = minimumcutLength_[i];
                nd[i] = minimumcutDistance_[i];
            }
            lefFree((char*) (minimumcut_));
            lefFree((char*) (minimumcutWidth_));
            lefFree((char*) (hasMinimumcutWithin_));
            lefFree((char*) (minimumcutWithin_));
            lefFree((char*) (hasMinimumcutConnection_));
            lefFree((char*) (minimumcutConnection_));
            lefFree((char*) (hasMinimumcutNumCuts_));
            lefFree((char*) (minimumcutLength_));
            lefFree((char*) (minimumcutDistance_));
        }
        minimumcut_ = nc;
        minimumcutWidth_ = nw;
        hasMinimumcutWithin_ = hcd;
        minimumcutWithin_ = ncd;
        hasMinimumcutConnection_ = hm;
        minimumcutConnection_ = nud;
        hasMinimumcutNumCuts_ = hc;
        minimumcutLength_ = nl;
        minimumcutDistance_ = nd;
    }
    minimumcut_[numMinimumcut_] = mincut;
    minimumcutWidth_[numMinimumcut_] = width;
    hasMinimumcutWithin_[numMinimumcut_] = 0;
    minimumcutWithin_[numMinimumcut_] = 0;
    hasMinimumcutConnection_[numMinimumcut_] = 0;
    minimumcutConnection_[numMinimumcut_] = 0;
    hasMinimumcutNumCuts_[numMinimumcut_] = 0;
    minimumcutLength_[numMinimumcut_] = 0;
    minimumcutDistance_[numMinimumcut_] = 0;
    numMinimumcut_ += 1;
}

// 5.7
void
lefiLayer::addMinimumcutWithin(double cutDistance)
{
    hasMinimumcutWithin_[numMinimumcut_ - 1] = 1;
    minimumcutWithin_[numMinimumcut_ - 1] = cutDistance;
}

// 5.5
void
lefiLayer::addMinimumcutConnect(const char *connectType)
{
    if (connectType && (strcmp(connectType, "") != 0)) {
        hasMinimumcutConnection_[numMinimumcut_ - 1] = 1;
        minimumcutConnection_[numMinimumcut_ - 1] =
            strdup(connectType);
    }
}

// 5.5
void
lefiLayer::addMinimumcutLengDis(double  length,
                                double  width)
{
    hasMinimumcutNumCuts_[numMinimumcut_ - 1] = 1;
    minimumcutLength_[numMinimumcut_ - 1] = length;
    minimumcutDistance_[numMinimumcut_ - 1] = width;
}

// 5.5, 5.6 switched to multiple
void
lefiLayer::addMinstep(double distance)
{
    double  *ms;
    char    **mt;
    double  *ml;
    int     *me;
    double  *ma;
    double  *mb;
    int     *mx;
    int     i;

    if (numMinstep_ == numMinstepAlloc_) {
        int len;
        if (numMinstepAlloc_ == 0) {
            len = numMinstepAlloc_ = 2;
            ms = (double*) lefMalloc(sizeof(double) * len);
            mt = (char**) lefMalloc(sizeof(char*) * len);
            ml = (double*) lefMalloc(sizeof(double) * len);
            me = (int*) lefMalloc(sizeof(int) * len);
            ma = (double*) lefMalloc(sizeof(double) * len);
            mb = (double*) lefMalloc(sizeof(double) * len);
            mx = (int*) lefMalloc(sizeof(int) * len);
        } else {
            len = numMinstepAlloc_ *= 2;
            ms = (double*) lefMalloc(sizeof(double) * len);
            mt = (char**) lefMalloc(sizeof(char*) * len);
            ml = (double*) lefMalloc(sizeof(double) * len);
            me = (int*) lefMalloc(sizeof(int) * len);
            ma = (double*) lefMalloc(sizeof(double) * len);
            mb = (double*) lefMalloc(sizeof(double) * len);
            mx = (int*) lefMalloc(sizeof(int) * len);

            for (i = 0; i < numMinstep_; i++) {
                ms[i] = minstep_[i];
                mt[i] = minstepType_[i];
                ml[i] = minstepLengthsum_[i];
                me[i] = minstepMaxEdges_[i];
                ma[i] = minstepMinAdjLength_[i];
                mb[i] = minstepMinBetLength_[i];
                mx[i] = minstepXSameCorners_[i];
            }
            lefFree((char*) (minstep_));
            lefFree((char*) (minstepType_));
            lefFree((char*) (minstepLengthsum_));
            lefFree((char*) (minstepMaxEdges_));
            lefFree((char*) (minstepMinAdjLength_));
            lefFree((char*) (minstepMinBetLength_));
            lefFree((char*) (minstepXSameCorners_));
        }
        minstep_ = ms;
        minstepType_ = mt;
        minstepLengthsum_ = ml;
        minstepMaxEdges_ = me;
        minstepMinAdjLength_ = ma;
        minstepMinBetLength_ = mb;
        minstepXSameCorners_ = mx;
    }
    minstep_[numMinstep_] = distance;
    minstepType_[numMinstep_] = 0;
    minstepLengthsum_[numMinstep_] = -1;
    minstepMaxEdges_[numMinstep_] = -1;
    minstepMinAdjLength_[numMinstep_] = -1;
    minstepMinBetLength_[numMinstep_] = -1;
    minstepXSameCorners_[numMinstep_] = -1;
    numMinstep_ += 1;
}

// 5.6
void
lefiLayer::addMinstepType(char *type)
{
    minstepType_[numMinstep_ - 1] = strdup(type);
}

// 5.6
void
lefiLayer::addMinstepLengthsum(double maxLength)
{
    minstepLengthsum_[numMinstep_ - 1] = maxLength;
}

// 5.7
void
lefiLayer::addMinstepMaxedges(int maxEdges)
{
    minstepMaxEdges_[numMinstep_ - 1] = maxEdges;
}

// 5.7
void
lefiLayer::addMinstepMinAdjLength(double adjLength)
{
    minstepMinAdjLength_[numMinstep_ - 1] = adjLength;
}

// 5.7
void
lefiLayer::addMinstepMinBetLength(double betLength)
{
    minstepMinBetLength_[numMinstep_ - 1] = betLength;
}

// 5.7
void
lefiLayer::addMinstepXSameCorners()
{
    minstepXSameCorners_[numMinstep_ - 1] = 1;
}

// 5.5
void
lefiLayer::setProtrusion(double width1,
                         double length,
                         double width2)
{
    protrusionWidth1_ = width1;
    protrusionLength_ = length;
    protrusionWidth2_ = width2;
}

// wmd -- pcr 282799, need to make left_, right_ as arrays
// when bumping to new list, need to copy also hasUseLengthThreshold_
// and lengthThreshold_
void
lefiLayer::setSpacingMin(double dist)
{
    if (numSpacings_ == spacingsAllocated_) {
        double  *nd;
        char    **nn;             // Also set up the spacing name
        int     *nsn;           // hasSpacingName_
        int     *nss;           // hasSpacingLayerStack_
        int     *nsa;           // hasSpacingAdjacent_
        int     *nr;            // hasSpacingRange_
        int     *nac;           // adjacentCuts_
        int     *ncc;           // hasCenterToCenter_
        int     *hpo;           // hasSpacingParallelOverlap_
        int     *heol;          // hasSpacingEndOfLine_
        double  *nwd, *nwn;     // eolWidth_, eolWithin_
        double  *ntl;           // notchLength_
        double  *eon;           // endOfNotchWidth_
        double  *nts;           // minNotchSpacing_
        double  *eonl;          // eonotchLength_
        int     *hpe;           // hasSpacingParellelEdge_
        double  *nps, *npw;     // parSpace_, parWithin_
        int     *hte;           // hasSpacingTwoEdges_
        int     *hae;           // hasSpacingAdjacentExcept_
        int     *hsn;           // hasSpacingSamenet_
        int     *hsno;          // hasSpacingSamenetPGonly_
        int     *hca;           // hasSpacingCutArea_
        double  *nca;           // spacingCutArea_
        double  *naw;           // adjacentWithin_
        double  *nrmin, *nrmax; // rangeMin_, rangeMax_
        double  *nri, *nrimin, *nrimax; // rangeInfluence_, rangeInfluenceRangeMin_,
        // rangeInfluenceRangeMax_
        double  *nrrmin, *nrrmax;       // rangeRangeMin_, rangeRangeMax_
        int     *ht;            // hasSpacingUseLengthThreshold_
        int     *nl;            // hasSpacingLengthThreshold_
        double  *nt, *ntmin, *ntmax; // lengthThreshold_, lengthThresholdMin_
        // lengthThresholdMax_

        int     i, lim;
        if (spacingsAllocated_ == 0) {
            lim = spacingsAllocated_ = 2;
            nd = (double*) lefMalloc(sizeof(double) * lim);
            nn = (char**) lefMalloc(sizeof(char*) * lim);
            nac = (int*) lefMalloc(sizeof(int) * lim);
            naw = (double*) lefMalloc(sizeof(double) * lim);
            nsn = (int*) lefMalloc(sizeof(int) * lim);
            nss = (int*) lefMalloc(sizeof(int) * lim);
            nsa = (int*) lefMalloc(sizeof(int) * lim);
            ncc = (int*) lefMalloc(sizeof(int) * lim);
            hpo = (int*) lefMalloc(sizeof(int) * lim);
            heol = (int*) lefMalloc(sizeof(int) * lim);
            nwd = (double*) lefMalloc(sizeof(double) * lim);
            nwn = (double*) lefMalloc(sizeof(double) * lim);
            ntl = (double*) lefMalloc(sizeof(double) * lim);
            eon = (double*) lefMalloc(sizeof(double) * lim);
            nts = (double*) lefMalloc(sizeof(double) * lim);
            eonl = (double*) lefMalloc(sizeof(double) * lim);
            hpe = (int*) lefMalloc(sizeof(int) * lim);
            nps = (double*) lefMalloc(sizeof(double) * lim);
            npw = (double*) lefMalloc(sizeof(double) * lim);
            hte = (int*) lefMalloc(sizeof(int) * lim);
            hae = (int*) lefMalloc(sizeof(int) * lim);
            hsn = (int*) lefMalloc(sizeof(int) * lim);
            hsno = (int*) lefMalloc(sizeof(int) * lim);
            hca = (int*) lefMalloc(sizeof(int) * lim);
            nca = (double*) lefMalloc(sizeof(double) * lim);
            nr = (int*) lefMalloc(sizeof(int) * lim);
            nrmin = (double*) lefMalloc(sizeof(double) * lim);
            nrmax = (double*) lefMalloc(sizeof(double) * lim);
            nri = (double*) lefMalloc(sizeof(double) * lim);
            nrimin = (double*) lefMalloc(sizeof(double) * lim);
            nrimax = (double*) lefMalloc(sizeof(double) * lim);
            nrrmin = (double*) lefMalloc(sizeof(double) * lim);
            nrrmax = (double*) lefMalloc(sizeof(double) * lim);
            ht = (int*) lefMalloc(sizeof(int) * lim);
            nl = (int*) lefMalloc(sizeof(int) * lim);
            nt = (double*) lefMalloc(sizeof(double) * lim);
            ntmin = (double*) lefMalloc(sizeof(double) * lim);
            ntmax = (double*) lefMalloc(sizeof(double) * lim);
        } else {
            lim = spacingsAllocated_ * 2;
            spacingsAllocated_ = lim;
            nd = (double*) lefMalloc(sizeof(double) * lim);
            nn = (char**) lefMalloc(sizeof(char*) * lim);
            nac = (int*) lefMalloc(sizeof(int) * lim);
            naw = (double*) lefMalloc(sizeof(double) * lim);
            nsn = (int*) lefMalloc(sizeof(int) * lim);
            nss = (int*) lefMalloc(sizeof(int) * lim);
            nsa = (int*) lefMalloc(sizeof(int) * lim);
            ncc = (int*) lefMalloc(sizeof(int) * lim);
            hpo = (int*) lefMalloc(sizeof(int) * lim);
            heol = (int*) lefMalloc(sizeof(int) * lim);
            nwd = (double*) lefMalloc(sizeof(double) * lim);
            nwn = (double*) lefMalloc(sizeof(double) * lim);
            ntl = (double*) lefMalloc(sizeof(double) * lim);
            eon = (double*) lefMalloc(sizeof(double) * lim);
            nts = (double*) lefMalloc(sizeof(double) * lim);
            eonl = (double*) lefMalloc(sizeof(double) * lim);
            hpe = (int*) lefMalloc(sizeof(int) * lim);
            nps = (double*) lefMalloc(sizeof(double) * lim);
            npw = (double*) lefMalloc(sizeof(double) * lim);
            hte = (int*) lefMalloc(sizeof(int) * lim);
            hae = (int*) lefMalloc(sizeof(int) * lim);
            hsn = (int*) lefMalloc(sizeof(int) * lim);
            hsno = (int*) lefMalloc(sizeof(int) * lim);
            hca = (int*) lefMalloc(sizeof(int) * lim);
            nca = (double*) lefMalloc(sizeof(double) * lim);
            nr = (int*) lefMalloc(sizeof(int) * lim);
            nrmin = (double*) lefMalloc(sizeof(double) * lim);
            nrmax = (double*) lefMalloc(sizeof(double) * lim);
            nri = (double*) lefMalloc(sizeof(double) * lim);
            nrimin = (double*) lefMalloc(sizeof(double) * lim);
            nrimax = (double*) lefMalloc(sizeof(double) * lim);
            nrrmin = (double*) lefMalloc(sizeof(double) * lim);
            nrrmax = (double*) lefMalloc(sizeof(double) * lim);
            ht = (int*) lefMalloc(sizeof(int) * lim);
            nl = (int*) lefMalloc(sizeof(int) * lim);
            nt = (double*) lefMalloc(sizeof(double) * lim);
            ntmin = (double*) lefMalloc(sizeof(double) * lim);
            ntmax = (double*) lefMalloc(sizeof(double) * lim);
            lim /= 2;
            for (i = 0; i < lim; i++) {
                nd[i] = spacing_[i];
                if (spacingName_[i])  // is null if is not CUT layer
                    nn[i] = spacingName_[i];
                else
                    nn[i] = 0;
                nac[i] = spacingAdjacentCuts_[i];
                naw[i] = spacingAdjacentWithin_[i];
                nsn[i] = hasSpacingName_[i];
                nss[i] = hasSpacingLayerStack_[i];
                nsa[i] = hasSpacingAdjacent_[i];
                ncc[i] = hasSpacingCenterToCenter_[i];
                hpo[i] = hasSpacingParallelOverlap_[i];
                nwd[i] = eolWidth_[i];
                nwn[i] = eolWithin_[i];
                ntl[i] = notchLength_[i];
                eon[i] = endOfNotchWidth_[i];
                nts[i] = minNotchSpacing_[i];
                eonl[i] = eonotchLength_[i];
                heol[i] = hasSpacingEndOfLine_[i];
                hpe[i] = hasSpacingParellelEdge_[i];
                nps[i] = parSpace_[i];
                npw[i] = parWithin_[i];
                hte[i] = hasSpacingTwoEdges_[i];
                hae[i] = hasSpacingAdjacentExcept_[i];
                hsn[i] = hasSpacingSamenet_[i];
                hsno[i] = hasSpacingSamenetPGonly_[i];
                hca[i] = hasSpacingCutArea_[i];
                nca[i] = spacingCutArea_[i];
                nr[i] = hasSpacingRange_[i];
                nrmin[i] = rangeMin_[i];
                nrmax[i] = rangeMax_[i];
                nri[i] = rangeInfluence_[i];
                nrimin[i] = rangeInfluenceRangeMin_[i];
                nrimax[i] = rangeInfluenceRangeMax_[i];
                nrrmin[i] = rangeRangeMin_[i];
                nrrmax[i] = rangeRangeMax_[i];
                ht[i] = hasSpacingUseLengthThreshold_[i];
                nl[i] = hasSpacingLengthThreshold_[i];
                nt[i] = lengthThreshold_[i];
                ntmin[i] = lengthThresholdRangeMin_[i];
                ntmax[i] = lengthThresholdRangeMax_[i];
            }
            lefFree((char*) (spacing_));
            lefFree((char*) (spacingName_));
            lefFree((char*) (spacingAdjacentCuts_));
            lefFree((char*) (spacingAdjacentWithin_));
            lefFree((char*) (hasSpacingName_));
            lefFree((char*) (hasSpacingLayerStack_));
            lefFree((char*) (hasSpacingAdjacent_));
            lefFree((char*) (hasSpacingRange_));
            lefFree((char*) (hasSpacingCenterToCenter_));
            lefFree((char*) (hasSpacingParallelOverlap_));
            lefFree((char*) (hasSpacingEndOfLine_));
            lefFree((char*) (eolWidth_));
            lefFree((char*) (eolWithin_));
            lefFree((char*) (notchLength_));
            lefFree((char*) (endOfNotchWidth_));
            lefFree((char*) (minNotchSpacing_));
            lefFree((char*) (eonotchLength_));
            lefFree((char*) (hasSpacingParellelEdge_));
            lefFree((char*) (hasSpacingAdjacentExcept_));
            lefFree((char*) (parSpace_));
            lefFree((char*) (parWithin_));
            lefFree((char*) (hasSpacingTwoEdges_));
            lefFree((char*) (hasSpacingSamenet_));
            lefFree((char*) (hasSpacingSamenetPGonly_));
            lefFree((char*) (hasSpacingCutArea_));
            lefFree((char*) (spacingCutArea_));
            lefFree((char*) (rangeMin_));
            lefFree((char*) (rangeMax_));
            lefFree((char*) (rangeInfluence_));
            lefFree((char*) (rangeInfluenceRangeMin_));
            lefFree((char*) (rangeInfluenceRangeMax_));
            lefFree((char*) (rangeRangeMin_));
            lefFree((char*) (rangeRangeMax_));
            lefFree((char*) (hasSpacingUseLengthThreshold_));
            lefFree((char*) (hasSpacingLengthThreshold_));
            lefFree((char*) (lengthThreshold_));
            lefFree((char*) (lengthThresholdRangeMin_));
            lefFree((char*) (lengthThresholdRangeMax_));
        }
        spacing_ = nd;
        spacingName_ = nn;
        spacingAdjacentCuts_ = nac;
        spacingAdjacentWithin_ = naw;
        hasSpacingName_ = nsn;
        hasSpacingLayerStack_ = nss;
        hasSpacingAdjacent_ = nsa;
        hasSpacingRange_ = nr;
        hasSpacingCenterToCenter_ = ncc;
        hasSpacingParallelOverlap_ = hpo;
        hasSpacingEndOfLine_ = heol;
        eolWidth_ = nwd;
        eolWithin_ = nwn;
        notchLength_ = ntl;
        endOfNotchWidth_ = eon;
        minNotchSpacing_ = nts;
        eonotchLength_ = eonl;
        hasSpacingParellelEdge_ = hpe;
        parSpace_ = nps;
        parWithin_ = npw;
        hasSpacingTwoEdges_ = hte;
        hasSpacingAdjacentExcept_ = hae;
        hasSpacingSamenet_ = hsn;
        hasSpacingSamenetPGonly_ = hsno;
        hasSpacingCutArea_ = hca;
        spacingCutArea_ = nca;
        rangeMin_ = nrmin;
        rangeMax_ = nrmax;
        rangeInfluence_ = nri;
        rangeInfluenceRangeMin_ = nrimin;
        rangeInfluenceRangeMax_ = nrimax;
        rangeRangeMin_ = nrrmin;
        rangeRangeMax_ = nrrmax;
        hasSpacingUseLengthThreshold_ = ht;
        hasSpacingLengthThreshold_ = nl;
        lengthThreshold_ = nt;
        lengthThresholdRangeMin_ = ntmin;
        lengthThresholdRangeMax_ = ntmax;
    }
    hasSpacing_ = 1;
    spacing_[numSpacings_] = dist;
    spacingName_[numSpacings_] = 0;
    hasSpacingName_[numSpacings_] = 0;
    hasSpacingLayerStack_[numSpacings_] = 0;
    spacingAdjacentCuts_[numSpacings_] = 0;
    spacingAdjacentWithin_[numSpacings_] = 0;
    hasSpacingAdjacent_[numSpacings_] = 0;
    hasSpacingRange_[numSpacings_] = 0;
    hasSpacingCenterToCenter_[numSpacings_] = 0;
    hasSpacingParallelOverlap_[numSpacings_] = 0;
    hasSpacingEndOfLine_[numSpacings_] = 0;
    hasSpacingAdjacentExcept_[numSpacings_] = 0;
    eolWidth_[numSpacings_] = 0;
    eolWithin_[numSpacings_] = 0;
    notchLength_[numSpacings_] = -1;
    endOfNotchWidth_[numSpacings_] = 0;
    minNotchSpacing_[numSpacings_] = 0;
    eonotchLength_[numSpacings_] = 0;
    hasSpacingParellelEdge_[numSpacings_] = 0;
    parSpace_[numSpacings_] = 0;
    parWithin_[numSpacings_] = 0;
    hasSpacingTwoEdges_[numSpacings_] = 0;
    hasSpacingSamenet_[numSpacings_] = 0;
    hasSpacingSamenetPGonly_[numSpacings_] = 0;
    hasSpacingCutArea_[numSpacings_] = 0;
    spacingCutArea_[numSpacings_] = 0;
    rangeMin_[numSpacings_] = -1;
    rangeMax_[numSpacings_] = -1;
    rangeInfluence_[numSpacings_] = 0;
    rangeInfluenceRangeMin_[numSpacings_] = -1;
    rangeInfluenceRangeMax_[numSpacings_] = -1;
    rangeRangeMin_[numSpacings_] = -1;
    rangeRangeMax_[numSpacings_] = -1;
    hasSpacingUseLengthThreshold_[numSpacings_] = 0;
    hasSpacingLengthThreshold_[numSpacings_] = 0;
    lengthThreshold_[numSpacings_] = 0;
    lengthThresholdRangeMin_[numSpacings_] = -1;
    lengthThresholdRangeMax_[numSpacings_] = -1;
    numSpacings_ += 1;
}

void
lefiLayer::setSpacingRange(double   left,
                           double   right)
{
    rangeMin_[numSpacings_ - 1] = left;
    rangeMax_[numSpacings_ - 1] = right;
    hasSpacingRange_[numSpacings_ - 1] = 1;
    rangeInfluence_[numSpacings_ - 1] = -1;
}

void
lefiLayer::setSpacingName(const char *spacingName)
{
    if (spacingName) {
        int len = strlen(spacingName) + 1;
        spacingName_[numSpacings_ - 1] = (char*) lefMalloc(len);
        strcpy(spacingName_[numSpacings_ - 1], CASE(spacingName));
        hasSpacingName_[numSpacings_ - 1] = 1;
    }
}

void
lefiLayer::setSpacingLayerStack()
{
    hasSpacingLayerStack_[numSpacings_ - 1] = 1;
}

void
lefiLayer::setSpacingAdjacent(int       numCuts,
                              double    distance)
{
    spacingAdjacentCuts_[numSpacings_ - 1] = numCuts;
    spacingAdjacentWithin_[numSpacings_ - 1] = distance;
    hasSpacingAdjacent_[numSpacings_ - 1] = 1;
}

void
lefiLayer::setSpacingRangeUseLength()
{
    hasSpacingUseLengthThreshold_[numSpacings_ - 1] = 1;
}

void
lefiLayer::setSpacingRangeInfluence(double infLength)
{
    rangeInfluence_[numSpacings_ - 1] = infLength;
}

void
lefiLayer::setSpacingRangeInfluenceRange(double min,
                                         double max)
{
    rangeInfluenceRangeMin_[numSpacings_ - 1] = min;
    rangeInfluenceRangeMax_[numSpacings_ - 1] = max;
}

void
lefiLayer::setSpacingRangeRange(double  min,
                                double  max)
{
    rangeRangeMin_[numSpacings_ - 1] = min;
    rangeRangeMax_[numSpacings_ - 1] = max;
}

void
lefiLayer::setSpacingLength(double num)
{
    lengthThreshold_[numSpacings_ - 1] = num;
    hasSpacingLengthThreshold_[numSpacings_ - 1] = 1;
}

void
lefiLayer::setSpacingLengthRange(double min,
                                 double max)
{
    lengthThresholdRangeMin_[numSpacings_ - 1] = min;
    lengthThresholdRangeMax_[numSpacings_ - 1] = max;
}

void
lefiLayer::setSpacingCenterToCenter()
{
    hasSpacingCenterToCenter_[numSpacings_ - 1] = 1;
}

// 5.7
void
lefiLayer::setSpacingParallelOverlap()
{
    hasSpacingParallelOverlap_[numSpacings_ - 1] = 1;
}

// 5.7
void
lefiLayer::setSpacingArea(double cutArea)
{
    spacingCutArea_[numSpacings_ - 1] = cutArea;
    hasSpacingCutArea_[numSpacings_ - 1] = 1;
}

// 5.7
void
lefiLayer::setSpacingEol(double width,
                         double within)
{
    hasSpacingEndOfLine_[numSpacings_ - 1] = 1;
    eolWidth_[numSpacings_ - 1] = width;
    eolWithin_[numSpacings_ - 1] = within;
}

// 5.7
void
lefiLayer::setSpacingParSW(double   space,
                           double   within)
{
    hasSpacingParellelEdge_[numSpacings_ - 1] = 1;
    parSpace_[numSpacings_ - 1] = space;
    parWithin_[numSpacings_ - 1] = within;
}

// 5.7
void
lefiLayer::setSpacingParTwoEdges()
{
    hasSpacingTwoEdges_[numSpacings_ - 1] = 1;
}

// 5.7
void
lefiLayer::setSpacingAdjacentExcept()
{
    hasSpacingAdjacentExcept_[numSpacings_ - 1] = 1;
}

// 5.7
void
lefiLayer::setSpacingSamenet()
{
    hasSpacingSamenet_[numSpacings_ - 1] = 1;
}

// 5.7
void
lefiLayer::setSpacingSamenetPGonly()
{
    hasSpacingSamenetPGonly_[numSpacings_ - 1] = 1;
}

// 5.7
void
lefiLayer::setSpacingNotchLength(double notchLength)
{
    notchLength_[numSpacings_ - 1] = notchLength;
}

// 5.7
void
lefiLayer::setSpacingEndOfNotchWidth(double eonWidth,
                                     double mnSpacing,
                                     double eonLength)
{
    endOfNotchWidth_[numSpacings_ - 1] = eonWidth;
    minNotchSpacing_[numSpacings_ - 1] = mnSpacing;
    eonotchLength_[numSpacings_ - 1] = eonLength;
}

// 5.7
void
lefiLayer::setSpacingTableOrtho()
{

    spacingTableOrtho_ = (lefiOrthogonal*) lefMalloc(sizeof(lefiOrthogonal));
    spacingTableOrtho_->Init();
}

// 5.7
void
lefiLayer::addSpacingTableOrthoWithin(double    cutWithin,
                                      double    orthoSp)
{
    spacingTableOrtho_->addOrthogonal(cutWithin, orthoSp);
    hasSpacingTableOrtho_ = 1;
}

// 5.7
void
lefiLayer::setMaxFloatingArea(double num)
{
    maxArea_ = num;
}

// 5.7
void
lefiLayer::setArraySpacingLongArray()
{
    hasLongArray_ = 1;
}

// 5.7
void
lefiLayer::setArraySpacingWidth(double viaWidth)
{
    viaWidth_ = viaWidth;
}

// 5.7
void
lefiLayer::setArraySpacingCut(double cutSpacing)
{
    cutSpacing_ = cutSpacing;
}

// 5.7
void
lefiLayer::addArraySpacingArray(int     arrayCut,
                                double  arraySpacing)
{
    int     i, len;
    int     *ac;
    double  *as;

    if (numArrayCuts_ == arrayCutsAllocated_) {
        if (arrayCutsAllocated_ == 0)
            len = arrayCutsAllocated_ = 2;
        else
            len = arrayCutsAllocated_ *= 2;
        ac = (int*) lefMalloc(sizeof(int) * len);
        as = (double*) lefMalloc(sizeof(double) * len);

        if (numArrayCuts_ > 0) {
            for (i = 0; i < numArrayCuts_; i++) {
                ac[i] = arrayCuts_[i];
                as[i] = arraySpacings_[i];
            }
            lefFree((char*) (arrayCuts_));
            lefFree((char*) (arraySpacings_));
        }
        arrayCuts_ = ac;
        arraySpacings_ = as;
    }
    arrayCuts_[numArrayCuts_] = arrayCut;
    arraySpacings_[numArrayCuts_] = arraySpacing;
    numArrayCuts_ += 1;
}

void
lefiLayer::setDirection(const char *dir)
{
    direction_ = (char*) dir;
    hasDirection_ = 1;
}

void
lefiLayer::setResistance(double num)
{
    hasResistance_ = 1;
    resistance_ = num;
}

void
lefiLayer::setCapacitance(double num)
{
    hasCapacitance_ = 1;
    capacitance_ = num;
}

void
lefiLayer::setHeight(double num)
{
    hasHeight_ = 1;
    height_ = num;
}

void
lefiLayer::setThickness(double num)
{
    hasThickness_ = 1;
    thickness_ = num;
}

void
lefiLayer::setShrinkage(double num)
{
    hasShrinkage_ = 1;
    shrinkage_ = num;
}

void
lefiLayer::setCapMultiplier(double num)
{
    hasCapMultiplier_ = 1;
    capMultiplier_ = num;
}

void
lefiLayer::setEdgeCap(double num)
{
    hasEdgeCap_ = 1;
    edgeCap_ = num;
}

void
lefiLayer::setAntennaLength(double num)
{
    hasAntennaLength_ = 1;
    antennaLength_ = num;
}

void
lefiLayer::setAntennaArea(double num)
{
    hasAntennaArea_ = 1;
    antennaArea_ = num;
}

void
lefiLayer::setCurrentDensity(double num)
{
    hasCurrentDensityPoint_ = 1;
    currentDensity_ = num;
}

void
lefiLayer::setCurrentPoint(double   width,
                           double   current)
{
    if (numCurrentPoints_ == currentPointsAllocated_) {
        int     max = numCurrentPoints_;
        int     len;
        int     i;
        double  *nc;
        double  *nw;

        if (currentPointsAllocated_ == 0)
            len = currentPointsAllocated_ = 2;
        else
            len = currentPointsAllocated_ *= 2;
        nc = (double*) lefMalloc(sizeof(double) * len);
        nw = (double*) lefMalloc(sizeof(double) * len);

        for (i = 0; i < max; i++) {
            nc[i] = current_[i];
            nw[i] = currentWidths_[i];
        }
        lefFree((char*) (current_));
        lefFree((char*) (currentWidths_));
        current_ = nc;
        currentWidths_ = nw;
    }
    current_[numCurrentPoints_] = current;
    currentWidths_[numCurrentPoints_] = width;
    numCurrentPoints_ += 1;
}

void
lefiLayer::setResistancePoint(double    width,
                              double    resistance)
{
    if (numResistancePoints_ == resistancePointsAllocated_) {
        int     max = numResistancePoints_;
        int     len;
        int     i;
        double  *nc;
        double  *nw;

        if (resistancePointsAllocated_ == 0)
            len = resistancePointsAllocated_ = 2;
        else
            len = resistancePointsAllocated_ *= 2;
        nc = (double*) lefMalloc(sizeof(double) * len);
        nw = (double*) lefMalloc(sizeof(double) * len);
        for (i = 0; i < max; i++) {
            nc[i] = resistances_[i];
            nw[i] = resistanceWidths_[i];
        }
        lefFree((char*) (resistances_));
        lefFree((char*) (resistanceWidths_));
        resistances_ = nc;
        resistanceWidths_ = nw;
    }
    resistances_[numResistancePoints_] = resistance;
    resistanceWidths_[numResistancePoints_] = width;
    numResistancePoints_ += 1;
}

void
lefiLayer::setCapacitancePoint(double   width,
                               double   capacitance)
{
    if (numCapacitancePoints_ == capacitancePointsAllocated_) {
        int     max = numCapacitancePoints_;
        int     len;
        int     i;
        double  *nc;
        double  *nw;

        if (capacitancePointsAllocated_ == 0)
            len = capacitancePointsAllocated_ = 2;
        else
            len = capacitancePointsAllocated_ *= 2;
        nc = (double*) lefMalloc(sizeof(double) * len);
        nw = (double*) lefMalloc(sizeof(double) * len);
        for (i = 0; i < max; i++) {
            nc[i] = capacitances_[i];
            nw[i] = capacitanceWidths_[i];
        }
        lefFree((char*) (capacitances_));
        lefFree((char*) (capacitanceWidths_));
        capacitances_ = nc;
        capacitanceWidths_ = nw;
    }
    capacitances_[numCapacitancePoints_] = capacitance;
    capacitanceWidths_[numCapacitancePoints_] = width;
    numCapacitancePoints_ += 1;
}

int
lefiLayer::hasType() const
{
    return (type_[0] != '\0') ? 1 : 0;
}

// 5.8
int
lefiLayer::hasLayerType() const
{
    if (layerType_)
        return 1;
    return 0;
}

int
lefiLayer::hasPitch() const
{
    if (hasPitch_ == 1)
        return 1;
    else
        return 0;
}

int
lefiLayer::hasMask() const
{
    if (hasMask_) {
        return 1;
    }

    return 0;
}
// 5.6
int
lefiLayer::hasXYPitch() const
{
    if (hasPitch_ == 2)
        return 1;
    else
        return 0;
}

int
lefiLayer::hasOffset() const
{
    if (hasOffset_ == 1)
        return 1;
    else
        return 0;
}

// 5.6
int
lefiLayer::hasXYOffset() const
{
    if (hasOffset_ == 2)
        return 1;
    else
        return 0;
}

int
lefiLayer::hasWidth() const
{
    return hasWidth_;
}

int
lefiLayer::hasArea() const
{
    return hasArea_;
}

// 5.6
int
lefiLayer::hasDiagPitch() const
{
    if (hasDiagPitch_ == 1)
        return 1;
    else
        return 0;
}

// 5.6
int
lefiLayer::hasXYDiagPitch() const
{
    if (hasDiagPitch_ == 2)
        return 1;
    else
        return 0;
}

// 5.6
int
lefiLayer::hasDiagWidth() const
{
    return hasDiagWidth_;
}

// 5.6
int
lefiLayer::hasDiagSpacing() const
{
    return hasDiagSpacing_;
}

int
lefiLayer::hasWireExtension() const
{
    return hasWireExtension_;
}

int
lefiLayer::hasSpacingNumber() const
{
    return ((hasSpacing_ != 0) && (numSpacings_ > 0)) ? 1 : 0;
}

int
lefiLayer::hasSpacingName(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingName_[index] != 0)) ? 1 : 0;
}

int
lefiLayer::hasSpacingLayerStack(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingLayerStack_[index] != 0)) ? 1 : 0;
}

int
lefiLayer::hasSpacingAdjacent(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingAdjacent_[index] != 0)) ? 1 : 0;
}

int
lefiLayer::hasSpacingRange(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingRange_[index] != 0)) ? 1 : 0;
}

int
lefiLayer::hasSpacingRangeUseLengthThreshold(int index) const
{
    return (hasSpacingUseLengthThreshold_[index]);
}

int
lefiLayer::hasSpacingRangeInfluence(int index) const
{
    return ((hasSpacing_ != 0) && (hasSpacingRange_[index] != 0) &&
            (rangeInfluence_[index]) != -1) ? 1 : 0;
}

int
lefiLayer::hasSpacingRangeInfluenceRange(int index) const
{
    return ((hasSpacing_ != 0) && (hasSpacingRange_[index] != 0) &&
            (rangeInfluenceRangeMin_[index] != -1) &&
            (rangeInfluenceRangeMax_[index] != -1)) ? 1 : 0;
}

int
lefiLayer::hasSpacingRangeRange(int index) const
{
    return ((hasSpacing_ != 0) && (hasSpacingRange_[index] != 0) &&
            (rangeRangeMin_[index] != -1) &&
            (rangeRangeMax_[index] != -1)) ? 1 : 0;
}

int
lefiLayer::hasSpacingLengthThreshold(int index) const
{
    return (hasSpacingLengthThreshold_[index]) ? 1 : 0;
}

int
lefiLayer::hasSpacingLengthThresholdRange(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingLengthThreshold_[index] != 0) &&
            (lengthThresholdRangeMin_[index] != -1) &&
            (lengthThresholdRangeMax_[index] != -1)) ? 1 : 0;
}

int
lefiLayer::hasSpacingCenterToCenter(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingCenterToCenter_[index] != 0)) ? 1 : 0;
}

// 5.7
int
lefiLayer::hasSpacingParallelOverlap(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingParallelOverlap_[index] != 0)) ? 1 : 0;
}

// 5.7
int
lefiLayer::hasSpacingArea(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingCutArea_[index] != 0)) ? 1 : 0;
}

// 5.7
int
lefiLayer::hasSpacingEndOfLine(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingEndOfLine_[index] != 0)) ? 1 : 0;
}

// 5.7
int
lefiLayer::hasSpacingParellelEdge(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingParellelEdge_[index] != 0)) ? 1 : 0;
}

// 5.7
int
lefiLayer::hasSpacingTwoEdges(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingTwoEdges_[index] != 0)) ? 1 : 0;
}

// 5.7
int
lefiLayer::hasSpacingAdjacentExcept(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingAdjacentExcept_[index] != 0)) ? 1 : 0;
}

// 5.7
int
lefiLayer::hasSpacingSamenet(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingSamenet_[index] != 0)) ? 1 : 0;
}

// 5.7
int
lefiLayer::hasSpacingSamenetPGonly(int index) const
{
    return ((hasSpacing_ != 0) &&
            (hasSpacingSamenetPGonly_[index] != 0)) ? 1 : 0;
}

// 5.7
int
lefiLayer::hasSpacingNotchLength(int index) const
{
    return (notchLength_[index] >= 0) ? 1 : 0;
}

// 5.7
int
lefiLayer::hasSpacingEndOfNotchWidth(int index) const
{
    return (endOfNotchWidth_[index] != 0) ? 1 : 0;
}

// 5.7
int
lefiLayer::hasSpacingTableOrtho() const
{
    return hasSpacingTableOrtho_ ? 1 : 0;
}

// 5.7
int
lefiLayer::hasMaxFloatingArea() const
{
    return maxArea_ ? 1 : 0;
}

// 5.7
int
lefiLayer::hasArraySpacing() const
{
    return cutSpacing_ ? 1 : 0;
}

// 5.7
int
lefiLayer::hasLongArray() const
{
    return hasLongArray_ ? 1 : 0;
}

// 5.7
int
lefiLayer::hasViaWidth() const
{
    return viaWidth_ ? 1 : 0;
}

int
lefiLayer::hasDirection() const
{
    return hasDirection_;
}

int
lefiLayer::hasResistance() const
{
    return hasResistance_;
}

int
lefiLayer::hasCapacitance() const
{
    return hasCapacitance_;
}

int
lefiLayer::hasHeight() const
{
    return hasHeight_;
}

int
lefiLayer::hasThickness() const
{
    return hasThickness_;
}

int
lefiLayer::hasShrinkage() const
{
    return hasShrinkage_;
}

int
lefiLayer::hasCapMultiplier() const
{
    return hasCapMultiplier_;
}

int
lefiLayer::hasEdgeCap() const
{
    return hasEdgeCap_;
}

int
lefiLayer::hasAntennaArea() const
{
    return hasAntennaArea_;
}

int
lefiLayer::hasAntennaLength() const
{
    return hasAntennaLength_;
}

int
lefiLayer::hasCurrentDensityPoint() const
{
    return hasCurrentDensityPoint_;
}

int
lefiLayer::hasCurrentDensityArray() const
{
    return numCurrentPoints_ ? 1 : 0;
}

int
lefiLayer::hasResistanceArray() const
{
    return numResistancePoints_ ? 1 : 0;
}

int
lefiLayer::hasCapacitanceArray() const
{
    return numCapacitancePoints_ ? 1 : 0;
}

void
lefiLayer::currentDensityArray(int      *numPoints,
                               double   **widths,
                               double   **current) const
{
    *numPoints = numCurrentPoints_;
    *widths = currentWidths_;
    *current = current_;
}

void
lefiLayer::resistanceArray(int      *numPoints,
                           double   **widths,
                           double   **res) const
{
    *numPoints = numResistancePoints_;
    *widths = resistanceWidths_;
    *res = resistances_;
}

void
lefiLayer::capacitanceArray(int     *numPoints,
                            double  **widths,
                            double  **cap) const
{
    *numPoints = numCapacitancePoints_;
    *widths = capacitanceWidths_;
    *cap = capacitances_;
}

int
lefiLayer::numSpacing() const
{
    return numSpacings_;
}

char *
lefiLayer::name() const
{
    return name_;
}

const char *
lefiLayer::type() const
{
    return type_;
}

// 5.8
const char *
lefiLayer::layerType() const
{
    return layerType_;
}

double
lefiLayer::pitch() const
{
    return pitchX_;
}

// 5.6
double
lefiLayer::pitchX() const
{
    return pitchX_;
}

// 5.6
double
lefiLayer::pitchY() const
{
    return pitchY_;
}

double
lefiLayer::offset() const
{
    return offsetX_;
}

// 5.6
double
lefiLayer::offsetX() const
{
    return offsetX_;
}

// 5.6
double
lefiLayer::offsetY() const
{
    return offsetY_;
}

double
lefiLayer::width() const
{
    return width_;
}

double
lefiLayer::area() const
{
    return area_;
}

// 5.6
double
lefiLayer::diagPitch() const
{
    return diagPitchX_;
}

// 5.6
double
lefiLayer::diagPitchX() const
{
    return diagPitchX_;
}

// 5.6
double
lefiLayer::diagPitchY() const
{
    return diagPitchY_;
}

// 5.6
double
lefiLayer::diagWidth() const
{
    return diagWidth_;
}

// 5.6
double
lefiLayer::diagSpacing() const
{
    return diagSpacing_;
}

double
lefiLayer::wireExtension() const
{
    return wireExtension_;
}

double
lefiLayer::spacing(int index) const
{
    return spacing_[index];
}

char *
lefiLayer::spacingName(int index) const
{
    return spacingName_[index];
}

int
lefiLayer::spacingAdjacentCuts(int index) const
{
    return spacingAdjacentCuts_[index];
}

double
lefiLayer::spacingAdjacentWithin(int index) const
{
    return spacingAdjacentWithin_[index];
}

double
lefiLayer::spacingArea(int index) const
{
    return spacingCutArea_[index];
}

double
lefiLayer::spacingRangeMin(int index) const
{
    return rangeMin_[index];
}

double
lefiLayer::spacingRangeMax(int index) const
{
    return rangeMax_[index];
}

double
lefiLayer::spacingRangeInfluence(int index) const
{
    return rangeInfluence_[index];
}

double
lefiLayer::spacingRangeInfluenceMin(int index) const
{
    return rangeInfluenceRangeMin_[index];
}

double
lefiLayer::spacingRangeInfluenceMax(int index) const
{
    return rangeInfluenceRangeMax_[index];
}

double
lefiLayer::spacingRangeRangeMin(int index) const
{
    return rangeRangeMin_[index];
}

double
lefiLayer::spacingRangeRangeMax(int index) const
{
    return rangeRangeMax_[index];
}

double
lefiLayer::spacingLengthThreshold(int index) const
{
    return lengthThreshold_[index];
}

double
lefiLayer::spacingLengthThresholdRangeMin(int index) const
{
    return lengthThresholdRangeMin_[index];
}

double
lefiLayer::spacingLengthThresholdRangeMax(int index) const
{
    return lengthThresholdRangeMax_[index];
}

// 5.7
double
lefiLayer::spacingEolWidth(int index) const
{
    return eolWidth_[index];
}

// 5.7
double
lefiLayer::spacingEolWithin(int index) const
{
    return eolWithin_[index];
}

// 5.7
double
lefiLayer::spacingParSpace(int index) const
{
    return parSpace_[index];
}

// 5.7
double
lefiLayer::spacingParWithin(int index) const
{
    return parWithin_[index];
}

// 5.7
double
lefiLayer::spacingNotchLength(int index) const
{
    return notchLength_[index];
}

// 5.7
double
lefiLayer::spacingEndOfNotchWidth(int index) const
{
    return endOfNotchWidth_[index];
}

// 5.7
double
lefiLayer::spacingEndOfNotchSpacing(int index) const
{
    return minNotchSpacing_[index];
}

// 5.7
double
lefiLayer::spacingEndOfNotchLength(int index) const
{
    return eonotchLength_[index];
}

const char *
lefiLayer::direction() const
{
    return direction_;
}

double
lefiLayer::currentDensityPoint() const
{
    return currentDensity_;
}

double
lefiLayer::resistance() const
{
    return resistance_;
}

double
lefiLayer::capacitance() const
{
    return capacitance_;
}

double
lefiLayer::height() const
{
    return height_;
}

double
lefiLayer::thickness() const
{
    return thickness_;
}

double
lefiLayer::shrinkage() const
{
    return shrinkage_;
}

double
lefiLayer::capMultiplier() const
{
    return capMultiplier_;
}

double
lefiLayer::edgeCap() const
{
    return edgeCap_;
}

double
lefiLayer::antennaLength() const
{
    return antennaLength_;
}

double
lefiLayer::antennaArea() const
{
    return antennaArea_;
}

// 5.5
int
lefiLayer::numMinimumcut() const
{
    return numMinimumcut_;
}

// 5.5
int
lefiLayer::minimumcut(int index) const
{
    return minimumcut_[index];
}

// 5.5
double
lefiLayer::minimumcutWidth(int index) const
{
    return minimumcutWidth_[index];
}

// 5.7
int
lefiLayer::hasMinimumcutWithin(int index) const
{
    return hasMinimumcutWithin_[index];
}

// 5.7
double
lefiLayer::minimumcutWithin(int index) const
{
    return minimumcutWithin_[index];
}

// 5.5
int
lefiLayer::hasMinimumcutConnection(int index) const
{
    return hasMinimumcutConnection_[index];
}

// 5.5
const char *
lefiLayer::minimumcutConnection(int index) const
{
    return minimumcutConnection_[index];
}

// 5.5
int
lefiLayer::hasMinimumcutNumCuts(int index) const
{
    return hasMinimumcutNumCuts_[index];
}

// 5.5
double
lefiLayer::minimumcutLength(int index) const
{
    return minimumcutLength_[index];
}

// 5.5
double
lefiLayer::minimumcutDistance(int index) const
{
    return minimumcutDistance_[index];
}

// 5.5
int
lefiLayer::hasMaxwidth() const
{
    return maxwidth_ == -1 ? 0 : 1;
}

// 5.5
double
lefiLayer::maxwidth() const
{
    return maxwidth_;
}

// 5.5
int
lefiLayer::hasMinwidth() const
{
    return minwidth_ == -1 ? 0 : 1;
}

// 5.5
double
lefiLayer::minwidth() const
{
    return minwidth_;
}

// 5.8
int
lefiLayer::mask() const
{
    return maskNumber_;
}

// 5.5
int
lefiLayer::numMinenclosedarea() const
{
    return numMinenclosedarea_;
}

// 5.5
int
lefiLayer::hasMinenclosedareaWidth(int index) const
{
    return minenclosedareaWidth_[index] == -1 ? 0 : 1;
}

// 5.5
double
lefiLayer::minenclosedarea(int index) const
{
    return minenclosedarea_[index];
}

// 5.5
double
lefiLayer::minenclosedareaWidth(int index) const
{
    return minenclosedareaWidth_[index];
}

// 5.5 & 5.6
int
lefiLayer::hasMinstep() const
{
    return numMinstep_? 1 : 0;
}

// 5.5
int
lefiLayer::hasProtrusion() const
{
    return protrusionWidth1_ == -1 ? 0 : 1;
}

// 5.5
double
lefiLayer::protrusionWidth1() const
{
    return protrusionWidth1_;
}

// 5.5
double
lefiLayer::protrusionLength() const
{
    return protrusionLength_;
}

// 5.5
double
lefiLayer::protrusionWidth2() const
{
    return protrusionWidth2_;
}

void
lefiLayer::print(FILE *f) const
{
    int     i, max;
    double  *j;
    double  *k;
    fprintf(f, "Layer %s:\n", name());
    if (hasType())
        fprintf(f, "  type %s\n", type());
    if (hasMask())
        fprintf(f, "  mask %d\n", mask());
    if (hasPitch())
        fprintf(f, "  pitch %g\n", pitch());
    if (hasWireExtension())
        fprintf(f, "  wireextension %g\n", wireExtension());
    if (hasWidth())
        fprintf(f, "  width %g\n", width());
    if (hasArea())
        fprintf(f, "  area %g\n", area());
    if (hasSpacingNumber()) {
        for (i = 0; i < numSpacing(); i++) {
            fprintf(f, "  spacing %g\n", spacing(i));
            if (hasSpacingRange(i)) {
                fprintf(f, "  range %g %g\n",
                        spacingRangeMin(i),
                        spacingRangeMax(i));
                if (hasSpacingRangeUseLengthThreshold(i))
                    fprintf(f, "    uselengththreshold\n");
                else if (hasSpacingRangeInfluence(i)) {
                    fprintf(f, "    influence %g\n",
                            spacingRangeInfluence(i));
                    if (hasSpacingRangeInfluenceRange(i))
                        fprintf(f, "      Range %g %g\n",
                                spacingRangeInfluenceMin(i),
                                spacingRangeInfluenceMax(i));
                } else if (hasSpacingRangeRange(i))
                    fprintf(f, "    range %g %g\n",
                            spacingRangeRangeMin(i),
                            spacingRangeRangeMax(i));
            } else if (hasSpacingLengthThreshold(i)) {
                fprintf(f, "  lengththreshold %g\n",
                        spacingLengthThreshold(i));
                if (hasSpacingLengthThresholdRange(i))
                    fprintf(f, "  range %g %g\n",
                            spacingLengthThresholdRangeMin(i),
                            spacingLengthThresholdRangeMax(i));
            }
        }
    }
    if (hasDirection())
        fprintf(f, "  direction %s\n", direction());

    if (hasResistance())
        fprintf(f, "  resistance %g\n", resistance());
    if (hasResistanceArray()) {
        resistanceArray(&max, &j, &k);
        fprintf(f, "  resistance PWL");
        for (i = 0; i < max; i++)
            fprintf(f, " %g %g", j[i], k[i]);
        fprintf(f, "\n");
    }
    if (hasCapacitance())
        fprintf(f, "  capacitance %g\n", capacitance());
    if (hasCapacitanceArray()) {
        capacitanceArray(&max, &j, &k);
        fprintf(f, "  capacitance PWL");
        for (i = 0; i < max; i++)
            fprintf(f, " %g %g", j[i], k[i]);
        fprintf(f, "\n");
    }

    if (hasHeight())
        fprintf(f, "  height %g\n", height());
    if (hasThickness())
        fprintf(f, "  thickness %g\n", thickness());
    if (hasShrinkage())
        fprintf(f, "  shrinkage %g\n", shrinkage());
    if (hasCapMultiplier())
        fprintf(f, "  cap muptiplier %g\n", capMultiplier());
    if (hasEdgeCap())
        fprintf(f, "  edge cap %g\n", edgeCap());

    if (hasCurrentDensityPoint())
        fprintf(f, "  currentden %g\n", currentDensityPoint());
    if (hasCurrentDensityArray()) {
        currentDensityArray(&max, &j, &k);
        fprintf(f, "  currentden PWL");
        for (i = 0; i < max; i++)
            fprintf(f, " %g %g", j[i], k[i]);
        fprintf(f, "\n");
    }
}

void
lefiLayer::addProp(const char   *name,
                   const char   *value,
                   const char   type)
{
    int len = strlen(name) + 1;
    // char*  tvalue;
    // int    vlen, i;
    if (numProps_ == propsAllocated_) {
        int     i;
        int     max;
        int     lim = numProps_;
        char    **nn;
        char    **nv;
        double  *nd;
        char    *nt;

        if (propsAllocated_ == 0)
            max = propsAllocated_ = 2;
        else
            max = propsAllocated_ *= 2;
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
    //if (type == 'N') {
    // it is a number, don't know if it is an integer or real
    // Look for . in the value
    //tvalue = (char*)value;
    //vlen = strlen(value);
    //for (i = 0; i < vlen; i++) { 
    //if (*tvalue == '.') {
    //types_[numProps_] = 'R';
    //break;
    //}
    //++tvalue;
    //types_[numProps_] = type;
    //}
    //} else
    types_[numProps_] = type;
    numProps_ += 1;
}

void
lefiLayer::addNumProp(const char    *name,
                      const double  d,
                      const char    *value,
                      const char    type)
{
    int len = strlen(name) + 1;
    if (numProps_ == propsAllocated_) {
        int     i;
        int     max;
        int     lim = numProps_;
        char    **nn;
        char    **nv;
        double  *nd;
        char    *nt;

        if (propsAllocated_ == 0)
            max = propsAllocated_ = 2;
        else
            max = propsAllocated_ *= 2;
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

int
lefiLayer::numProps() const
{
    return numProps_;
}

const char *
lefiLayer::propName(int i) const
{
    char msg[160];
    if (i < 0 || i >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1300): The index number %d given for the layer property is invalid.\nValid index is from 0 to %d", i, numProps_);
        lefiError(0, 1300, msg);
        return 0;
    }
    return names_[i];
}

const char *
lefiLayer::propValue(int i) const
{
    char msg[160];
    if (i < 0 || i >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1300): The index number %d given for the layer property is invalid.\nValid index is from 0 to %d", i, numProps_);
        lefiError(0, 1300, msg);
        return 0;
    }
    return values_[i];
}

double
lefiLayer::propNumber(int i) const
{
    char msg[160];
    if (i < 0 || i >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1300): The index number %d given for the layer property is invalid.\nValid index is from 0 to %d", i, numProps_);
        lefiError(0, 1300, msg);
        return 0;
    }
    return dvalues_[i];
}

char
lefiLayer::propType(int i) const
{
    char msg[160];
    if (i < 0 || i >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1300): The index number %d given for the layer property is invalid.\nValid index is from 0 to %d", i, numProps_);
        lefiError(0, 1300, msg);
        return 0;
    }
    return types_[i];
}

int
lefiLayer::propIsNumber(int i) const
{
    char msg[160];
    if (i < 0 || i >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1300): The index number %d given for the layer property is invalid.\nValid index is from 0 to %d", i, numProps_);
        lefiError(0, 1300, msg);
        return 0;
    }
    return dvalues_[i] ? 1 : 0;
}

int
lefiLayer::propIsString(int i) const
{
    char msg[160];
    if (i < 0 || i >= numProps_) {
        sprintf(msg, "ERROR (LEFPARS-1300): The index number %d given for the layer property is invalid.\nValid index is from 0 to %d", i, numProps_);
        lefiError(0, 1300, msg);
        return 0;
    }
    return dvalues_[i] ? 0 : 1;
}

void
lefiLayer::addAccurrentDensity(const char *type)
{
    lefiLayerDensity *density;
    if (numAccurrents_ == accurrentAllocated_) {
        lefiLayerDensity    **array;
        int                 i;
        accurrentAllocated_ = accurrentAllocated_ ?
            accurrentAllocated_ * 2 : 2;
        array = (lefiLayerDensity**) lefMalloc(sizeof(lefiLayerDensity*)
                                               * accurrentAllocated_);
        for (i = 0; i < numAccurrents_; i++)
            array[i] = accurrents_[i];
        if (accurrents_)
            lefFree((char*) (accurrents_));
        accurrents_ = array;
    }
    density = accurrents_[numAccurrents_] =
        (lefiLayerDensity*) lefMalloc(sizeof(lefiLayerDensity));
    numAccurrents_ += 1;
    density->Init(type);
}

void
lefiLayer::setAcOneEntry(double num)
{
    lefiLayerDensity *density;
    density = accurrents_[numAccurrents_ - 1];
    density->setOneEntry(num);
}

void
lefiLayer::addAcFrequency()
{
    lefiLayerDensity *density;
    density = accurrents_[numAccurrents_ - 1];
    density->addFrequency(numNums_, nums_);
    numNums_ = 0;
    numAllocated_ = 0;
    nums_ = 0;
}

void
lefiLayer::addAcCutarea()
{
    lefiLayerDensity *density;
    density = accurrents_[numAccurrents_ - 1];
    density->addCutarea(numNums_, nums_);
    numNums_ = 0;
    numAllocated_ = 0;
    nums_ = 0;
}

void
lefiLayer::addAcTableEntry()
{
    lefiLayerDensity *density;
    density = accurrents_[numAccurrents_ - 1];
    density->addTableEntry(numNums_, nums_);
    numNums_ = 0;
    numAllocated_ = 0;
    nums_ = 0;
}

void
lefiLayer::addAcWidth()
{
    lefiLayerDensity *density;
    density = accurrents_[numAccurrents_ - 1];
    density->addWidth(numNums_, nums_);
    numNums_ = 0;
    numAllocated_ = 0;
    nums_ = 0;
}

void
lefiLayer::setDcOneEntry(double num)
{
    lefiLayerDensity *density;
    density = dccurrents_[numDccurrents_ - 1];
    density->setOneEntry(num);
}

void
lefiLayer::addDccurrentDensity(const char *type)
{
    lefiLayerDensity *density;
    if (numDccurrents_ == dccurrentAllocated_) {
        lefiLayerDensity    **array;
        int                 i;
        dccurrentAllocated_ = dccurrentAllocated_ ?
            dccurrentAllocated_ * 2 : 2;
        array = (lefiLayerDensity**) lefMalloc(sizeof(lefiLayerDensity*)
                                               * dccurrentAllocated_);
        for (i = 0; i < numDccurrents_; i++)
            array[i] = dccurrents_[i];
        if (dccurrents_)
            lefFree((char*) (dccurrents_));
        dccurrents_ = array;
    }
    density = dccurrents_[numDccurrents_] =
        (lefiLayerDensity*) lefMalloc(sizeof(lefiLayerDensity));
    numDccurrents_ += 1;
    density->Init(type);
}

void
lefiLayer::addDcCutarea()
{
    lefiLayerDensity *density;
    density = dccurrents_[numDccurrents_ - 1];
    density->addCutarea(numNums_, nums_);
    numNums_ = 0;
    numAllocated_ = 0;
    nums_ = 0;
}

void
lefiLayer::addDcTableEntry()
{
    lefiLayerDensity *density;
    density = dccurrents_[numDccurrents_ - 1];
    density->addTableEntry(numNums_, nums_);
    numNums_ = 0;
    numAllocated_ = 0;
    nums_ = 0;
}

void
lefiLayer::addDcWidth()
{
    lefiLayerDensity *density;
    density = dccurrents_[numDccurrents_ - 1];
    density->addWidth(numNums_, nums_);
    numNums_ = 0;
    numAllocated_ = 0;
    nums_ = 0;
}

void
lefiLayer::addNumber(double num)
{
    if (numNums_ == numAllocated_) {
        double  *array;
        int     i;
        numAllocated_ = numAllocated_ ?
            numAllocated_ * 2 : 2;
        array = (double*) lefMalloc(sizeof(double) * numAllocated_);
        for (i = 0; i < numNums_; i++)
            array[i] = nums_[i];
        if (nums_)
            lefFree((char*) (nums_));
        nums_ = array;
    }
    nums_[numNums_++] = num;
}

int
lefiLayer::getNumber()
{
    return numNums_ - 1;
}

int
lefiLayer::hasAccurrentDensity() const
{
    return numAccurrents_ ? 1 : 0;
}

int
lefiLayer::hasDccurrentDensity() const
{
    return numDccurrents_ ? 1 : 0;
}

int
lefiLayer::numAccurrentDensity() const
{
    return (numAccurrents_);
}

int
lefiLayer::numDccurrentDensity() const
{
    return (numDccurrents_);
}

lefiLayerDensity *
lefiLayer::accurrent(int index) const
{
    if (index >= numAccurrents_)
        return 0;
    return (accurrents_[index]);
}

lefiLayerDensity *
lefiLayer::dccurrent(int index) const
{
    if (index >= numDccurrents_)
        return 0;
    return (dccurrents_[index]);
}

// 5.5
void
lefiLayer::addAntennaModel(int aOxide)
{
     // For version 5.5 only OXIDE1, OXIDE2, OXIDE3, & OXIDE4
    // are defined within a macro pin
    lefiAntennaModel *amo;
    int                 i;

    if (numAntennaModel_ == 0) {   // does not have antennaModel
        antennaModel_ = (lefiAntennaModel**)
            lefMalloc(sizeof(lefiAntennaModel*) * 4);
        antennaModelAllocated_ = 4;
        for (i = 0; i < 4; i++) {
            antennaModel_[i] = (lefiAntennaModel*)
                lefMalloc(sizeof(lefiAntennaModel));
            antennaModel_[i]->Init();
            // just initialize it first
        }
        antennaModelAllocated_ = 4;
        amo = antennaModel_[0];
                }

    // First can go any oxide, so fill pref oxides models.
    for (int idx = 0; idx < aOxide - 1; idx++) {
        amo = antennaModel_[idx];
        if (!amo->antennaOxide()) {
            amo->setAntennaModel(idx + 1);   
                }
                }

    amo = antennaModel_[aOxide - 1];
    // Oxide has not defined yet
    if (amo->antennaOxide()) {
        amo->Destroy();
                }

    if (aOxide > numAntennaModel_) {
        numAntennaModel_ = aOxide;
            }

            amo->Init();
    amo->setAntennaModel(aOxide);

    currentAntennaModel_ = amo;
    
    return;
}

// 5.5
int
lefiLayer::numAntennaModel() const
{
    return numAntennaModel_;
}

// 5.5
lefiAntennaModel *
lefiLayer::antennaModel(int index) const
{
    return antennaModel_[index];
}

// 3/23/2000 -- Wanda da Rosa.  The following are for 5.4 syntax
void
lefiLayer::setAntennaAreaRatio(double value)
{
    if (numAntennaModel_ == 0)   // haven't created any antannaModel yet
        addAntennaModel(1);
    currentAntennaModel_->setAntennaAreaRatio(value);
}

void
lefiLayer::setAntennaCumAreaRatio(double value)
{
    if (numAntennaModel_ == 0)   // haven't created any antannaModel yet
        addAntennaModel(1);
    currentAntennaModel_->setAntennaCumAreaRatio(value);
}

void
lefiLayer::setAntennaAreaFactor(double value)
{
    if (numAntennaModel_ == 0)   // haven't created any antannaModel yet
        addAntennaModel(1);
    currentAntennaModel_->setAntennaAreaFactor(value);
}

void
lefiLayer::setAntennaSideAreaRatio(double value)
{
    if (numAntennaModel_ == 0)   // haven't created any antannaModel yet
        addAntennaModel(1);
    currentAntennaModel_->setAntennaSideAreaRatio(value);
}

void
lefiLayer::setAntennaCumSideAreaRatio(double value)
{
    if (numAntennaModel_ == 0)   // haven't created any antannaModel yet
        addAntennaModel(1);
    currentAntennaModel_->setAntennaCumSideAreaRatio(value);
}

void
lefiLayer::setAntennaSideAreaFactor(double value)
{
    if (numAntennaModel_ == 0)   // haven't created any antannaModel yet
        addAntennaModel(1);
   currentAntennaModel_->setAntennaSideAreaFactor(value);
}

void
lefiLayer::setAntennaValue(lefiAntennaEnum  antennaType,
                           double           value)
{
    if (numAntennaModel_ == 0)   // haven't created any antannaModel yet
        addAntennaModel(1);
    currentAntennaModel_->setAntennaValue(antennaType, value);
}

void
lefiLayer::setAntennaDUO(lefiAntennaEnum antennaType)
{
    if (numAntennaModel_ == 0)   // haven't created any antannaModel yet
        addAntennaModel(1);
    currentAntennaModel_->setAntennaDUO(antennaType);
}

void
lefiLayer::setAntennaPWL(lefiAntennaEnum    antennaType,
                         lefiAntennaPWL     *pwl)
{
    if (numAntennaModel_ == 0)   // haven't created any antannaModel yet
        addAntennaModel(1);
    currentAntennaModel_->setAntennaPWL(antennaType, pwl);
}

// 5.7
void
lefiLayer::setAntennaCumRoutingPlusCut()
{
    if (numAntennaModel_ == 0)   // haven't created any antannaModel yet
        addAntennaModel(1);
    currentAntennaModel_->setAntennaCumRoutingPlusCut();
}

// 5.7
void
lefiLayer::setAntennaGatePlusDiff(double value)
{
    if (numAntennaModel_ == 0)   // haven't created any antannaModel yet
        addAntennaModel(1);
    currentAntennaModel_->setAntennaGatePlusDiff(value);
}

// 5.7
void
lefiLayer::setAntennaAreaMinusDiff(double value)
{
    if (numAntennaModel_ == 0)   // haven't created any antannaModel yet
        addAntennaModel(1);
    currentAntennaModel_->setAntennaAreaMinusDiff(value);
}

// 8/29/2001 -- Wanda da Rosa.  The following are for 5.4 enhancements

void
lefiLayer::setSlotWireWidth(double num)
{
    hasSlotWireWidth_ = 1;
    slotWireWidth_ = num;
}

void
lefiLayer::setSlotWireLength(double num)
{
    hasSlotWireLength_ = 1;
    slotWireLength_ = num;
}

void
lefiLayer::setSlotWidth(double num)
{
    hasSlotWidth_ = 1;
    slotWidth_ = num;
}

void
lefiLayer::setSlotLength(double num)
{
    hasSlotLength_ = 1;
    slotLength_ = num;
}

void
lefiLayer::setMaxAdjacentSlotSpacing(double num)
{
    hasMaxAdjacentSlotSpacing_ = 1;
    maxAdjacentSlotSpacing_ = num;
}

void
lefiLayer::setMaxCoaxialSlotSpacing(double num)
{
    hasMaxCoaxialSlotSpacing_ = 1;
    maxCoaxialSlotSpacing_ = num;
}

void
lefiLayer::setMaxEdgeSlotSpacing(double num)
{
    hasMaxEdgeSlotSpacing_ = 1;
    maxEdgeSlotSpacing_ = num;
}

void
lefiLayer::setSplitWireWidth(double num)
{
    hasSplitWireWidth_ = 1;
    splitWireWidth_ = num;
}

void
lefiLayer::setMinimumDensity(double num)
{
    hasMinimumDensity_ = 1;
    minimumDensity_ = num;
}

void
lefiLayer::setMaximumDensity(double num)
{
    hasMaximumDensity_ = 1;
    maximumDensity_ = num;
}

void
lefiLayer::setDensityCheckWindow(double length,
                                 double width)
{
    hasDensityCheckWindow_ = 1;
    densityCheckWindowLength_ = length;
    densityCheckWindowWidth_ = width;
}

void
lefiLayer::setDensityCheckStep(double num)
{
    hasDensityCheckStep_ = 1;
    densityCheckStep_ = num;
}

void
lefiLayer::setFillActiveSpacing(double num)
{
    hasFillActiveSpacing_ = 1;
    fillActiveSpacing_ = num;
}

int
lefiLayer::hasSlotWireWidth() const
{
    return hasSlotWireWidth_;
}

int
lefiLayer::hasSlotWireLength() const
{
    return hasSlotWireLength_;
}

int
lefiLayer::hasSlotWidth() const
{
    return hasSlotWidth_;
}

int
lefiLayer::hasSlotLength() const
{
    return hasSlotLength_;
}

int
lefiLayer::hasMaxAdjacentSlotSpacing() const
{
    return hasMaxAdjacentSlotSpacing_;
}

int
lefiLayer::hasMaxCoaxialSlotSpacing() const
{
    return hasMaxCoaxialSlotSpacing_;
}

int
lefiLayer::hasMaxEdgeSlotSpacing() const
{
    return hasMaxEdgeSlotSpacing_;
}

int
lefiLayer::hasSplitWireWidth() const
{
    return hasSplitWireWidth_;
}

int
lefiLayer::hasMinimumDensity() const
{
    return hasMinimumDensity_;
}

int
lefiLayer::hasMaximumDensity() const
{
    return hasMaximumDensity_;
}

int
lefiLayer::hasDensityCheckWindow() const
{
    return hasDensityCheckWindow_;
}

int
lefiLayer::hasDensityCheckStep() const
{
    return hasDensityCheckStep_;
}

int
lefiLayer::hasFillActiveSpacing() const
{
    return hasFillActiveSpacing_;
}

double
lefiLayer::slotWireWidth() const
{
    return slotWireWidth_;
}

double
lefiLayer::slotWireLength() const
{
    return slotWireLength_;
}

double
lefiLayer::slotWidth() const
{
    return slotWidth_;
}

double
lefiLayer::slotLength() const
{
    return slotLength_;
}

double
lefiLayer::maxAdjacentSlotSpacing() const
{
    return maxAdjacentSlotSpacing_;
}

double
lefiLayer::maxCoaxialSlotSpacing() const
{
    return maxCoaxialSlotSpacing_;
}

double
lefiLayer::maxEdgeSlotSpacing() const
{
    return maxEdgeSlotSpacing_;
}

double
lefiLayer::splitWireWidth() const
{
    return splitWireWidth_;
}

double
lefiLayer::minimumDensity() const
{
    return minimumDensity_;
}

double
lefiLayer::maximumDensity() const
{
    return maximumDensity_;
}

double
lefiLayer::densityCheckWindowLength() const
{
    return densityCheckWindowLength_;
}

double
lefiLayer::densityCheckWindowWidth() const
{
    return densityCheckWindowWidth_;
}

double
lefiLayer::densityCheckStep() const
{
    return densityCheckStep_;
}

double
lefiLayer::fillActiveSpacing() const
{
    return fillActiveSpacing_;
}

// 5.5 SPACINGTABLE

void
lefiLayer::addSpacingTable()
{
    lefiSpacingTable *sp;
    if (numSpacingTable_ == spacingTableAllocated_) {
        lefiSpacingTable    **array;
        int                 i;
        spacingTableAllocated_ = spacingTableAllocated_ ?
            spacingTableAllocated_ * 2 : 2;
        array = (lefiSpacingTable**) lefMalloc(sizeof(lefiSpacingTable*)
                                               * spacingTableAllocated_);
        for (i = 0; i < numSpacingTable_; i++)
            array[i] = spacingTable_[i];
        if (spacingTable_)
            lefFree((char*) (spacingTable_));
        spacingTable_ = array;
    }
    sp = spacingTable_[numSpacingTable_] =
        (lefiSpacingTable*) lefMalloc(sizeof(lefiSpacingTable));
    numSpacingTable_ += 1;
    sp->Init();
}

void
lefiLayer::addSpParallelLength()
{
    lefiSpacingTable *sp;
    sp = spacingTable_[numSpacingTable_ - 1];
    sp->addParallelLength(numNums_, nums_);
    numNums_ = 0;
    numAllocated_ = 0;
    nums_ = 0;
}

void
lefiLayer::addSpParallelWidth(double width)
{
    lefiSpacingTable *sp;
    sp = spacingTable_[numSpacingTable_ - 1];
    sp->addParallelWidth(width);
}

void
lefiLayer::addSpParallelWidthSpacing()
{
    lefiSpacingTable *sp;
    sp = spacingTable_[numSpacingTable_ - 1];
    sp->addParallelWidthSpacing(numNums_, nums_);
    // Since inside addParallelWidthSpacing copy the nums_, we can free it
    // here
    lefFree((char*) (nums_));
    numNums_ = 0;
    numAllocated_ = 0;
    nums_ = 0;
}

void
lefiLayer::addSpTwoWidths(double    width,
                          double    runLength)
{
    lefiSpacingTable *sp;
    sp = spacingTable_[numSpacingTable_ - 1];
    /* This will never happen since in lef.y, the grammer requires a number for
       spacing
      if (numNums_ == 0) {
         * spacing is required in TWOWIDTHS *
         lefiError("ERROR (LEFPARS-1324): Incorrect syntax defined for the statement TWOWIDTHS.\nspacing, which is required is not defined.");
         return;
      }
    */
    sp->addTwoWidths(width, runLength, numNums_,
                     nums_, hasTwoWidthPRL_);
    // Since inside addTwoWidthsSpacing copy the nums_, we can free it
    // here
    lefFree((char*) (nums_));
    numNums_ = 0;
    numAllocated_ = 0;
    nums_ = 0;
    hasTwoWidthPRL_ = 0;
}

void
lefiLayer::setInfluence()
{
    lefiSpacingTable *sp;
    sp = spacingTable_[numSpacingTable_ - 1];
    sp->setInfluence();
}

void
lefiLayer::addSpInfluence(double    width,
                          double    distance,
                          double    spacing)
{
    lefiSpacingTable *sp;
    sp = spacingTable_[numSpacingTable_ - 1];
    sp->addInfluence(width, distance, spacing);
}

int
lefiLayer::numSpacingTable()
{
    return numSpacingTable_;
}

void
lefiLayer::setSpTwoWidthsHasPRL(int hasPRL)
{
    hasTwoWidthPRL_ = hasPRL;
}

lefiSpacingTable *
lefiLayer::spacingTable(int index)
{
    return spacingTable_[index];
}

// 5.6 ENCLOSURE PREFERENCLOSURE & RESISTANCEPERCUT

void
lefiLayer::addEnclosure(char    *enclRule,
                        double  overhang1,
                        double  overhang2)
{
    if (numEnclosure_ == enclosureAllocated_) {
        int     i, len;
        char    **er;
        double  *o1;
        double  *o2;
        double  *mw;
        double  *ct;
        double  *ml;

        if (enclosureAllocated_ == 0)
            len = enclosureAllocated_ = 2;
        else
            len = enclosureAllocated_ *= 2;
        er = (char**) lefMalloc(sizeof(char*) * len);
        o1 = (double*) lefMalloc(sizeof(double) * len);
        o2 = (double*) lefMalloc(sizeof(double) * len);
        mw = (double*) lefMalloc(sizeof(double) * len);
        ct = (double*) lefMalloc(sizeof(double) * len);
        ml = (double*) lefMalloc(sizeof(double) * len);

        if (numEnclosure_ > 0) {
            for (i = 0; i < numEnclosure_; i++) {
                er[i] = enclosureRules_[i];
                o1[i] = overhang1_[i];
                o2[i] = overhang2_[i];
                mw[i] = encminWidth_[i];
                ct[i] = cutWithin_[i];
                ml[i] = minLength_[i];
            }
            lefFree((char*) (enclosureRules_));
            lefFree((char*) (overhang1_));
            lefFree((char*) (overhang2_));
            lefFree((char*) (encminWidth_));
            lefFree((char*) (cutWithin_));
            lefFree((char*) (minLength_));
        }
        enclosureRules_ = er;
        overhang1_ = o1;
        overhang2_ = o2;
        encminWidth_ = mw;
        cutWithin_ = ct;
        minLength_ = ml;
    }
    if (enclRule) {
        if (strcmp(enclRule, "NULL") == 0)
            enclosureRules_[numEnclosure_] = NULL;
        else
            enclosureRules_[numEnclosure_] = strdup(enclRule);
    } else
        enclosureRules_[numEnclosure_] = NULL;
    overhang1_[numEnclosure_] = overhang1;
    overhang2_[numEnclosure_] = overhang2;
    encminWidth_[numEnclosure_] = 0;
    cutWithin_[numEnclosure_] = 0;
    minLength_[numEnclosure_] = 0;
    numEnclosure_ += 1;
}

void
lefiLayer::addEnclosureWidth(double minWidth)
{
    encminWidth_[numEnclosure_ - 1] = minWidth;
}

void
lefiLayer::addEnclosureExceptEC(double cutWithin)
{
    cutWithin_[numEnclosure_ - 1] = cutWithin;
}

void
lefiLayer::addEnclosureLength(double minLength)
{
    minLength_[numEnclosure_ - 1] = minLength;
}

int
lefiLayer::numEnclosure() const
{
    return numEnclosure_;
}

int
lefiLayer::hasEnclosureRule(int index) const
{
    return enclosureRules_[index] ? 1 : 0;
}

char *
lefiLayer::enclosureRule(int index)
{
    return enclosureRules_[index];
}

double
lefiLayer::enclosureOverhang1(int index) const
{
    return overhang1_[index];
}

double
lefiLayer::enclosureOverhang2(int index) const
{
    return overhang2_[index];
}

int
lefiLayer::hasEnclosureWidth(int index) const
{
    return encminWidth_[index] ? 1 : 0;
}

double
lefiLayer::enclosureMinWidth(int index) const
{
    return encminWidth_[index];
}

int
lefiLayer::hasEnclosureExceptExtraCut(int index) const
{
    return cutWithin_[index] ? 1 : 0;
}

double
lefiLayer::enclosureExceptExtraCut(int index) const
{
    return cutWithin_[index];
}

int
lefiLayer::hasEnclosureMinLength(int index) const
{
    return minLength_[index] ? 1 : 0;
}

double
lefiLayer::enclosureMinLength(int index) const
{
    return minLength_[index];
}

void
lefiLayer::addPreferEnclosure(char      *enclRule,
                              double    overhang1,
                              double    overhang2)
{
    if (numPreferEnclosure_ == preferEnclosureAllocated_) {
        int     i, len;
        char    **er;
        double  *o1;
        double  *o2;
        double  *mw;

        if (preferEnclosureAllocated_ == 0)
            len = preferEnclosureAllocated_ = 2;
        else
            len = preferEnclosureAllocated_ *= 2;
        er = (char**) lefMalloc(sizeof(char*) * len);
        o1 = (double*) lefMalloc(sizeof(double) * len);
        o2 = (double*) lefMalloc(sizeof(double) * len);
        mw = (double*) lefMalloc(sizeof(double) * len);

        if (numPreferEnclosure_ > 0) {
            for (i = 0; i < numPreferEnclosure_; i++) {
                er[i] = preferEnclosureRules_[i];
                o1[i] = preferOverhang1_[i];
                o2[i] = preferOverhang1_[i];
                mw[i] = preferMinWidth_[i];
            }
            lefFree((char*) (preferEnclosureRules_));
            lefFree((char*) (preferOverhang1_));
            lefFree((char*) (preferOverhang2_));
            lefFree((char*) (preferMinWidth_));
        }
        preferEnclosureRules_ = er;
        preferOverhang1_ = o1;
        preferOverhang2_ = o2;
        preferMinWidth_ = mw;
    }
    if (strcmp(enclRule, "NULL") == 0)
        preferEnclosureRules_[numPreferEnclosure_] = NULL;
    else
        preferEnclosureRules_[numPreferEnclosure_] = strdup(enclRule);
    preferOverhang1_[numPreferEnclosure_] = overhang1;
    preferOverhang2_[numPreferEnclosure_] = overhang2;
    preferMinWidth_[numPreferEnclosure_] = 0;
    numPreferEnclosure_ += 1;
}

void
lefiLayer::addPreferEnclosureWidth(double minWidth)
{
    preferMinWidth_[numPreferEnclosure_ - 1] = minWidth;
}

int
lefiLayer::numPreferEnclosure() const
{
    return numPreferEnclosure_;
}

int
lefiLayer::hasPreferEnclosureRule(int index) const
{
    return preferEnclosureRules_[index] ? 1 : 0;
}

char *
lefiLayer::preferEnclosureRule(int index)
{
    return preferEnclosureRules_[index];
}

double
lefiLayer::preferEnclosureOverhang1(int index) const
{
    return preferOverhang1_[index];
}

double
lefiLayer::preferEnclosureOverhang2(int index) const
{
    return preferOverhang2_[index];
}

int
lefiLayer::hasPreferEnclosureWidth(int index) const
{
    return preferMinWidth_[index] ? 1 : 0;
}

double
lefiLayer::preferEnclosureMinWidth(int index) const
{
    return preferMinWidth_[index];
}

void
lefiLayer::setResPerCut(double value)
{
    resPerCut_ = value;
}

int
lefiLayer::hasResistancePerCut() const
{
    return resPerCut_ ? 1 : 0;
}

double
lefiLayer::resistancePerCut() const
{
    return resPerCut_;
}

void
lefiLayer::setDiagMinEdgeLength(double value)
{
    diagMinEdgeLength_ = value;
}

int
lefiLayer::hasDiagMinEdgeLength() const
{
    return diagMinEdgeLength_ ? 1 : 0;
}

double
lefiLayer::diagMinEdgeLength() const
{
    return diagMinEdgeLength_;
}

void
lefiLayer::setMinSize(lefiGeometries *geom)
{
    struct lefiGeomPolygon  tempPoly;
    int                     i;

    tempPoly = *(geom->getPolygon(0));
    numMinSize_ = tempPoly.numPoints;
    if (numMinSize_ > 0) {
        minSizeWidth_ = (double*) lefMalloc(sizeof(double) * numMinSize_);
        minSizeLength_ = (double*) lefMalloc(sizeof(double) * numMinSize_);
        for (i = 0; i < numMinSize_; i++) {
            minSizeWidth_[i] = tempPoly.x[i];
            minSizeLength_[i] = tempPoly.y[i];
        }
    } else {
        minSizeWidth_ = 0;
        minSizeLength_ = 0;
    }
}

int
lefiLayer::numMinSize() const
{
    return numMinSize_;
}

double
lefiLayer::minSizeWidth(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinSize_) {
        sprintf(msg, "ERROR (LEFPARS-1301): The index number %d given for the layer MINSIZE is invalid.\nValid index is from 0 to %d\n", index, numMinSize_);
        lefiError(0, 1301, msg);
        return 0;
    }
    return minSizeWidth_[index];
}

double
lefiLayer::minSizeLength(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinSize_) {
        sprintf(msg, "ERROR (LEFPARS-1301): The index number %d given for the layer MINSIZE is invalid.\nValid index is from 0 to %d\n", index, numMinSize_);
        lefiError(0, 1301, msg);
        return 0;
    }
    return minSizeLength_[index];
}

// 5.6 CHANGES ON MINSTEP
int
lefiLayer::numMinstep() const
{
    return numMinstep_;
}

double
lefiLayer::minstep(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinstep_) {
        sprintf(msg, "ERROR (LEFPARS-1302): The index number %d given for the layer MINSTEP is invalid.\nValid index is from 0 to %d", index, numMinstep_);
        lefiError(0, 1302, msg);
        return 0;
    }
    return minstep_[index];
}

int
lefiLayer::hasMinstepType(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinstep_) {
        sprintf(msg, "ERROR (LEFPARS-1302): The index number %d given for the layer MINSTEP is invalid.\nValid index is from 0 to %d", index, numMinstep_);
        lefiError(0, 1302, msg);
        return 0;
    }
    return minstepType_[index] ? 1 : 0;
}

char *
lefiLayer::minstepType(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinstep_) {
        sprintf(msg, "ERROR (LEFPARS-1302): The index number %d given for the layer MINSTEP is invalid.\nValid index is from 0 to %d", index, numMinstep_);
        lefiError(0, 1302, msg);
        return 0;
    }
    return minstepType_[index];
}

int
lefiLayer::hasMinstepLengthsum(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinstep_) {
        sprintf(msg, "ERROR (LEFPARS-1302): The index number %d given for the layer MINSTEP is invalid.\nValid index is from 0 to %d", index, numMinstep_);
        lefiError(0, 1302, msg);
        return 0;
    }
    return minstepLengthsum_[index] == -1 ? 0 : 1;
}

double
lefiLayer::minstepLengthsum(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinstep_) {
        sprintf(msg, "ERROR (LEFPARS-1302): The index number %d given for the layer MINSTEP is invalid.\nValid index is from 0 to %d", index, numMinstep_);
        lefiError(0, 1302, msg);
        return 0;
    }
    return minstepLengthsum_[index];
}

// 5.7
int
lefiLayer::hasMinstepMaxedges(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinstep_) {
        sprintf(msg, "ERROR (LEFPARS-1302): The index number %d given for the layer MINSTEP is invalid.\nValid index is from 0 to %d", index, numMinstep_);
        lefiError(0, 1302, msg);
        return 0;
    }
    return minstepMaxEdges_[index] == -1 ? 0 : 1;
}

// 5.7
int
lefiLayer::minstepMaxedges(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinstep_) {
        sprintf(msg, "ERROR (LEFPARS-1302): The index number %d given for the layer MINSTEP is invalid.\nValid index is from 0 to %d", index, numMinstep_);
        lefiError(0, 1302, msg);
        return 0;
    }
    return minstepMaxEdges_[index];
}

// 5.7
int
lefiLayer::hasMinstepMinAdjLength(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinstep_) {
        sprintf(msg, "ERROR (LEFPARS-1302): The index number %d given for the layer MINSTEP is invalid.\nValid index is from 0 to %d", index, numMinstep_);
        lefiError(0, 1302, msg);
        return 0;
    }
    return minstepMinAdjLength_[index] == -1 ? 0 : 1;
}

// 5.7
double
lefiLayer::minstepMinAdjLength(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinstep_) {
        sprintf(msg, "ERROR (LEFPARS-1302): The index number %d given for the layer MINSTEP is invalid.\nValid index is from 0 to %d", index, numMinstep_);
        lefiError(0, 1302, msg);
        return 0;
    }
    return minstepMinAdjLength_[index];
}

// 5.7
int
lefiLayer::hasMinstepMinBetLength(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinstep_) {
        sprintf(msg, "ERROR (LEFPARS-1302): The index number %d given for the layer MINSTEP is invalid.\nValid index is from 0 to %d", index, numMinstep_);
        lefiError(0, 1302, msg);
        return 0;
    }
    return minstepMinBetLength_[index] == -1 ? 0 : 1;
}

// 5.7
double
lefiLayer::minstepMinBetLength(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinstep_) {
        sprintf(msg, "ERROR (LEFPARS-1302): The index number %d given for the layer MINSTEP is invalid.\nValid index is from 0 to %d", index, numMinstep_);
        lefiError(0, 1302, msg);
        return 0;
    }
    return minstepMinBetLength_[index];
}

// 5.7
int
lefiLayer::hasMinstepXSameCorners(int index) const
{
    char msg[160];
    if (index < 0 || index > numMinstep_) {
        sprintf(msg, "ERROR (LEFPARS-1302): The index number %d given for the layer MINSTEP is invalid.\nValid index is from 0 to %d", index, numMinstep_);
        lefiError(0, 1302, msg);
        return 0;
    }
    return minstepXSameCorners_[index] == -1 ? 0 : 1;
}

// 5.7
lefiOrthogonal *
lefiLayer::orthogonal() const
{
    return spacingTableOrtho_;
}

// 5.7
double
lefiLayer::maxFloatingArea() const
{
    return maxArea_;
}

// 5.7
double
lefiLayer::viaWidth() const
{
    return viaWidth_;
}

// 5.7
double
lefiLayer::cutSpacing() const
{
    return cutSpacing_;
}

// 5.7
int
lefiLayer::numArrayCuts() const
{
    return numArrayCuts_;
}

// 5.7
int
lefiLayer::arrayCuts(int index) const
{
    char msg[160];
    if (index < 0 || index > numArrayCuts_) {
        sprintf(msg, "ERROR (LEFPARS-1303): The index number %d given for the layer ARRAYCUTS is invalid.\nValid index is from 0 to %d", index, numArrayCuts_);
        lefiError(0, 1303, msg);
        return 0;
    }
    return arrayCuts_[index];
}

// 5.7
double
lefiLayer::arraySpacing(int index) const
{
    char msg[160];
    if (index < 0 || index > numArrayCuts_) {
        sprintf(msg, "ERROR (LEFPARS-1304): The index number %d given for the layer SPACING is invalid.\nValid index is from 0 to %d", index, numArrayCuts_);
        lefiError(0, 1304, msg);
        return 0;
    }
    return arraySpacings_[index];
}

// PRIVATE 5.7
// SPACING cutSpacing
//     [CENTERTOCENTER]
//     [SAMENET]
//     [ LAYER secondLayerName [STACK]
//     | ADJACENTCUTS {2|3|4} WITHIN cutWithin [EXCEPTSAMEPGNET]
//     | PARALLELOVERLAP
//     | AREA cutArea ] ;
// SPACING routing ENDOFLINE eolWidth WITHIN eolWithin
//     [PARALLELEDGE parSpace WITHIN parWithin [TWOEDGES]];
void
lefiLayer::parseSpacing(int index)
{
    char    *wrkingStr = strdup(values_[index]);
    char    *value;
    double  spValue = 0, width = 0, within = 0, pValue = 0, pWithin = 0;
    double  cutArea = 0;
    char    msg[1024];
    int     numCuts = 0, twoEdges = 0;

    // Pre-parse the string before breaking it up and store it in
    // the layer class.
    // If the string is
    // SPACING eolSpace ENDOFLINE eolWidth WITHIN eolWithin
    //    [PARALLELEDGE parSpace WITHIN parWithin [TWOEDGES]]
    //    [ENCLOSECUT [BELOW | ABOVE] encloseDist CUTSPACING cutToMetalSpace]
    // Keep the property as it, and don't break it and save in layer class

    value = strtok(wrkingStr, " ");
    while (value) {
        if (strcmp(value, "SPACING") != 0) {
            free(wrkingStr);
            return;
        }

        value = strtok(NULL, " ");
        spValue = atof(value);

        value = strtok(NULL, " ");
        if ((strcmp(value, "CENTERTOCENTER") == 0) ||
            (strcmp(value, "SAMENET") == 0) ||
            (strcmp(value, "LAYER") == 0) ||
            (strcmp(value, "ADJACENTCUTS") == 0) ||
            (strcmp(value, "PARALLELOVERLAP") == 0) ||
            (strcmp(value, "AREA") == 0)) {

            if (strcmp(type(), "CUT") != 0) {
                /*
                            sprintf(msg, "ERROR (LEFPARS-1321): The property LEF57_SPACING with value %s is for TYPE CUT only.\nThe current layer has the TYPE %s.\nUpdate the property of your lef file with the correct syntax or remove this property from your lef file.\n",
                            values_[index], type());
                            lefiError(msg);
                */
                sprintf(msg, "The property LEF57_SPACING with value %s is for TYPE CUT only.\nThe current layer has the TYPE %s.\nUpdate the property of your lef file with the correct syntax or remove this property from your lef file.\n",
                        values_[index], type());
                lefError(1321, msg);
                free(wrkingStr);
                return;
            }
            setSpacingMin(spValue);
            if (strcmp(value, "CENTERTOCENTER") == 0) {
                // SPACING minSpacing CENTERTOCENTER ;
                setSpacingCenterToCenter();
                value = strtok(NULL, " ");
                if (*value == ';') {
                    value = strtok(NULL, " ");
                    continue;  // Look for a new statement
                }
            }
            if (strcmp(value, "SAMENET") == 0) {
                // SPACING minSpacing SAMENET ;
                setSpacingSamenet();
                value = strtok(NULL, " ");
                if (*value == ';') {
                    value = strtok(NULL, " ");
                    continue;  // Look for a new statement
                }
            }
            if (strcmp(value, "LAYER") == 0) {
                value = strtok(NULL, " ");
                if (value && *value != '\n') {
                    setSpacingName(value);
                    value = strtok(NULL, " ");
                    if (strcmp(value, "STACK") == 0) {
                        setSpacingLayerStack();
                        value = strtok(NULL, " ");
                        if (*value != ';') {
                            /*
                                                 sprintf(msg, "ERROR (LEFPARS-1320): Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                                                 values_[index]);
                                                 lefiError(msg);
                            */
                            sprintf(msg, "Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                                    values_[index]);
                            lefError(1320, msg);
                            free(wrkingStr);
                            return;
                        }
                    } else if (*value != ';') {
                        /*
                                          sprintf(msg, "ERROR (LEFPARS-1320): Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                                          values_[index]);
                                          lefiError(msg);
                        */
                        sprintf(msg, "Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                                values_[index]);
                        lefError(1320, msg);
                        free(wrkingStr);
                        return;
                    } else {
                        value = strtok(NULL, " ");
                        continue;
                    }
                }
            } else if (strcmp(value, "ADJACENTCUTS") == 0) {
                value = strtok(NULL, " ");
                numCuts = atoi(value);
                if ((numCuts < 2) || (numCuts > 4)) {
                    /*
                                   sprintf(msg, "ERROR (LEFPARS-1320): Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                                   values_[index]);
                                   lefiError(msg);
                    */
                    sprintf(msg, "Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                            values_[index]);
                    lefError(1320, msg);
                    free(wrkingStr);
                    return;
                }
                value = strtok(NULL, " ");
                if (strcmp(value, "WITHIN") == 0) {
                    value = strtok(NULL, " ");
                    within = atof(value);
                    setSpacingAdjacent(numCuts, within);
                    value = strtok(NULL, " ");
                    if (strcmp(value, "EXCEPTSAMEPGNET") == 0) {
                        setSpacingAdjacentExcept();
                        value = strtok(NULL, " ");
                        if (*value != ';') {
                            /*
                                                 sprintf(msg, "ERROR (LEFPARS-1320): Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                                                 values_[index]);
                                                 lefiError(msg);
                            */
                            sprintf(msg, "Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                                    values_[index]);
                            lefError(1320, msg);
                            free(wrkingStr);
                            return;
                        }
                    } else if (*value != ';') {
                        /*
                                          sprintf(msg, "ERROR (LEFPARS-1320): Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                                          values_[index]);
                                          lefiError(msg);
                        */
                        sprintf(msg, "Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                                values_[index]);
                        lefError(1320, msg);
                        free(wrkingStr);
                        return;
                    } else {
                        value = strtok(NULL, " ");
                    }
                } else {
                    /*
                                   sprintf(msg, "ERROR (LEFPARS-1320): Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                                   values_[index]);
                                   lefiError(msg);
                    */
                    sprintf(msg, "Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                            values_[index]);
                    lefError(1320, msg);
                    free(wrkingStr);
                    return;
                }
            } else if (strcmp(value, "PARALLELOVERLAP") == 0) {
                // SPACING minSpacing PARALLELOVERLAP ;
                setSpacingParallelOverlap();
                value = strtok(NULL, " ");
                if (*value != ';') {
                    /*
                                   sprintf(msg, "ERROR (LEFPARS-1320): Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                                   values_[index]);
                                   lefiError(msg);
                    */
                    sprintf(msg, "Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                            values_[index]);
                    lefError(1320, msg);
                    free(wrkingStr);
                    return;
                } else {
                    value = strtok(NULL, " ");
                }
            } else if (strcmp(value, "AREA") == 0) {
                value = strtok(NULL, " ");
                cutArea = atof(value);
                setSpacingArea(cutArea);
                value = strtok(NULL, " ");
                if (*value != ';') {
                    /*
                                   sprintf(msg, "ERROR (LEFPARS-1320): Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                                   values_[index]);
                                   lefiError(msg);
                    */
                    sprintf(msg, "Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING cutSpacing [CENTERTOCENTER][SAMENET]\n\t[LAYER secondLayerName[STACK]\n\t| ADJACENTCUTS {2 | 3 | 4} WITHIN cutWithin [EXCEPTSAMEPGNET]\n\t| PARALLELOVERLAP\n\t| AREA cutArea ;\"",
                            values_[index]);
                    lefError(1320, msg);
                    free(wrkingStr);
                    return;
                } else {
                    value = strtok(NULL, " ");
                }
            }
        } else if (strcmp(value, "SAMEMETAL") == 0) {
            // SPACING cutSpacing SAMEMETAL just exit
            free(wrkingStr);
            return;

        } else if (strcmp(value, "ENDOFLINE") == 0) {
            // SPACING eolSpace ENDOFLINE eolWidth WITHIN eolWithin
            //    [PARALLELEGE parSpace WITHIN parWithin [TWOEDGES]]
            // Parse the string lefData->first, if it has the syntax of
            // SPACING eolSpace ENDOFLINE eolWidth WITHIN eolWithin
            // [PARALLELEDGE parSpace WITHIN parWithin [TWOEDGES]]
            // than store the data, otherwise, skip to ;

            if (strcmp(type(), "ROUTING") != 0) {
                /*
                            sprintf(msg, "ERROR (LEFPARS-1322): The property LEF57_SPACING with value %s is for TYPE ROUTING only.\nThe current layer has the TYPE %s.\nUpdate the property of your lef file with the correct syntax or remove this property from your lef file.\n",
                            values_[index], type());
                            lefiError(msg);
                */
                sprintf(msg, "The property LEF57_SPACING with value %s is for TYPE ROUTING only.\nThe current layer has the TYPE %s.\nUpdate the property of your lef file with the correct syntax or remove this property from your lef file.\n",
                        values_[index], type());
                lefError(1322, msg);
                free(wrkingStr);
                return;
            }

            twoEdges = 0;

            value = strtok(NULL, " ");
            width = atof(value);
            value = strtok(NULL, " ");
            if (strcmp(value, "WITHIN") == 0) {
                value = strtok(NULL, " ");
                within = atof(value);
                // * setSpacingMin(spValue);
                // * setSpacingEol(width, within);
                // Check if option [PARALLELEDGE parSpace WITHIN parWithin] is set
                value = strtok(NULL, " ");
                if (value && *value != '\n') {
                    if (strcmp(value, "PARALLELEDGE") == 0) {
                        value = strtok(NULL, " ");
                        pValue = atof(value);
                        value = strtok(NULL, " ");
                        if (strcmp(value, "WITHIN") == 0) {
                            value = strtok(NULL, " ");
                            pWithin = atof(value);
                            // * setSpacingParSW(pValue, pWithin);
                            // Check if TWOEDGES is set
                            value = strtok(NULL, " ");
                            if (value && *value != '\n') {
                                if (strcmp(value, "TWOEDGES") == 0) {
                                    // * setSpacingParTwoEdges();
                                    twoEdges = 1;
                                    value = strtok(NULL, " ");
                                    if (*value == ';') {
                                        // Save the value to lefiLayer class
                                        setSpacingMin(spValue);
                                        setSpacingEol(width, within);
                                        setSpacingParSW(pValue, pWithin);
                                        if (twoEdges)
                                            setSpacingParTwoEdges();
                                        value = strtok(NULL, " ");
                                        continue;                // with the while loop 
                                    } else {
                                        // More rules, skip to ;
                                        while ((value) && (*value != ';') &&
                                               (*value != '\n'))
                                            value = strtok(NULL, " ");
                                        if ((value) && (*value == ';')) {
                                            value = strtok(NULL, " ");
                                            continue;
                                        }
                                    }
                                } else if (*value == ';') {
                                    setSpacingMin(spValue);
                                    setSpacingEol(width, within);
                                    setSpacingParSW(pValue, pWithin);
                                    value = strtok(NULL, " ");      // done with this 
                                    continue;       // statement with the while loop 
                                } else {
                                    // More rules, skip to ;
                                    while ((value) && (*value != ';') &&
                                           (*value != '\n'))
                                        value = strtok(NULL, " ");
                                    if ((value) && (*value == ';')) {
                                        value = strtok(NULL, " ");
                                        continue;
                                    }
                                }
                            } else {
                                /*
                                                        sprintf(msg, "ERROR (LEFPARS-1305): Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING minSpacing [CENTERTOCENTER]\"\n\"[LAYER secondLayerName | ADJACENTCUTS {2|3|4} WITHIN cutWithin | PARALLELOVERLAP | AREA cutArea]\" or\n\"SPACING eolSpace ENDOFLINE eolWidth WITHIN eolWITHIN [PARALLELEDGE parSpace WITHIN parWithin [TOWEDGES]]\"\n", values_[index]);
                                                        lefiError(msg);
                                */
                                sprintf(msg, "Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING minSpacing [CENTERTOCENTER]\"\n\"[LAYER secondLayerName | ADJACENTCUTS {2|3|4} WITHIN cutWithin | PARALLELOVERLAP | AREA cutArea]\" or\n\"SPACING eolSpace ENDOFLINE eolWidth WITHIN eolWITHIN [PARALLELEDGE parSpace WITHIN parWithin [TOWEDGES]]\"\n", values_[index]);
                                lefError(1305, msg);
                                free(wrkingStr);
                                return;
                            }
                        } else {
                            /*
                                                 sprintf(msg, "ERROR (LEFPARS-1305): Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING minSpacing [CENTERTOCENTER]\"\n\"[LAYER secondLayerName | ADJACENTCUTS {2|3|4} WITHIN cutWithin | PARALLELOVERLAP | AREA cutArea]\" or\n\"SPACING eolSpace ENDOFLINE eolWidth WITHIN eolWITHIN [PARALLELEDGE parSpace WITHIN parWithin [TOWEDGES]]\"\n", values_[index]);
                                                 lefiError(msg);
                            */
                            sprintf(msg, "Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is \"SPACING minSpacing [CENTERTOCENTER]\"\n\"[LAYER secondLayerName | ADJACENTCUTS {2|3|4} WITHIN cutWithin | PARALLELOVERLAP | AREA cutArea]\" or\n\"SPACING eolSpace ENDOFLINE eolWidth WITHIN eolWITHIN [PARALLELEDGE parSpace WITHIN parWithin [TOWEDGES]]\"\n", values_[index]);
                            lefError(1305, msg);
                            free(wrkingStr);
                            return;
                        }
                    } else if (*value == ';') {
                        // Save the data in lefiLayer lefData->first
                        setSpacingMin(spValue);
                        setSpacingEol(width, within);
                        value = strtok(NULL, " ");      // done with this 
                        continue;
                    } else {
                        while ((value) && (*value != ';') && (*value != '\n'))
                            value = strtok(NULL, " ");
                        if ((value) && (*value == ';')) {
                            value = strtok(NULL, " ");
                            continue;
                        }
                    }
                }
            } else {
                /*
                            sprintf(msg, "ERROR (LEFPARS-1305): Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is either \"SPACING minSpacing [CENTERTOCENTER]\"\n\"[LAYER secondLayerName | ADJACENTCUTS {2|3|4} WITHIN cutWithin | PARALLELOVERLAP | AREA cutArea]\" or\n\"SPACING eolSpace ENDOFLINE eolWidth WITHIN eolWITHIN [PARALLELEDGE parSpace WITHIN parWithin [TOWEDGES]]\"\n", values_[index]);
                            lefiError(msg);
                */
                sprintf(msg, "Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is either \"SPACING minSpacing [CENTERTOCENTER]\"\n\"[LAYER secondLayerName | ADJACENTCUTS {2|3|4} WITHIN cutWithin | PARALLELOVERLAP | AREA cutArea]\" or\n\"SPACING eolSpace ENDOFLINE eolWidth WITHIN eolWITHIN [PARALLELEDGE parSpace WITHIN parWithin [TOWEDGES]]\"\n", values_[index]);
                lefError(1305, msg);
                free(wrkingStr);
                return;
            }
        } else {
            /*
                     sprintf(msg, "ERROR (LEFPARS-1305): Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is either \"SPACING minSpacing [CENTERTOCENTER]\"\n\"[LAYER secondLayerName | ADJACENTCUTS {2|3|4} WITHIN cutWithin | PARALLELOVERLAP | AREA cutArea]\" or\n\"SPACING eolSpace ENDOFLINE eolWidth WITHIN eolWITHIN [PARALLELEDGE parSpace WITHIN parWithin [TOWEDGES]]\"\n", values_[index]);
                     lefiError(msg);
            */
            sprintf(msg, "Incorrect syntax defined for property LEF57_SPACING: %s.\nCorrect syntax is either \"SPACING minSpacing [CENTERTOCENTER]\"\n\"[LAYER secondLayerName | ADJACENTCUTS {2|3|4} WITHIN cutWithin | PARALLELOVERLAP | AREA cutArea]\" or\n\"SPACING eolSpace ENDOFLINE eolWidth WITHIN eolWITHIN [PARALLELEDGE parSpace WITHIN parWithin [TOWEDGES]]\"\n", values_[index]);
            lefError(1305, msg);
            free(wrkingStr);
            return;
        }
    }

    // None of the above statement
    free(wrkingStr);
    return;
}

// PRIVATE 5.7
/* NOT an OA data model
void lefiLayer::parseMaxFloating(int index) {
   char   *wrkingStr = strdup(values_[index]);
   char   *value;
   double maxArea;
   char   msg[1024];

   value = strtok(wrkingStr, " ");
   if (strcmp(value, "MAXFLOATINGAREA") != 0) {
      sprintf(msg, "ERROR (LEFPARS-1306): Incorrect syntax defined for property LEF57_MAXFLOATINGAREA: %s.\nCorrect syntax is \"MAXFLOATINGAREA maxArea\"\n", values_[index]);
      lefiError(0, 1306, msg);
      free(wrkingStr);
      return;
   }

   value = strtok(NULL, " ");
   maxArea = atof(value);
   setMaxFloatingArea(maxArea);

   free(wrkingStr);
   return;
}
*/

// PRIVATE 5.7
void
lefiLayer::parseArraySpacing(int index)
{
    char    *wrkingStr = strdup(values_[index]);
    char    *value;
    double  viaWidth = 0, cutSpacing = 0, arraySpacing;
    int     arrayCuts;
    int     hasLongArray = 0, hasArrayCut = 0;
    char    msg[1024];

    value = strtok(wrkingStr, " ");
    if (strcmp(value, "ARRAYSPACING") != 0) {
        /*
              sprintf(msg, "ERROR (LEFPARS-1307): Incorrect syntax defined for property LEF57_ARRAYSPACING: %s.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
              lefiError(msg);
        */
        sprintf(msg, "Incorrect syntax defined for property LEF57_ARRAYSPACING: %s.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
        lefError(1307, msg);
        free(wrkingStr);
        return;
    }
    value = strtok(NULL, " ");

    while (strcmp(value, ";") != 0) {
        if (strcmp(value, "LONGARRAY") == 0) {
            if (cutSpacing != 0) { // make sure syntax has correct order 
                /*
                sprintf(msg, "ERROR (LEFPARS-1308): Incorrect syntax defined for property LEF57_ARRAYSPACING: %s.\nLONGARRAY is defined after CUTSPACING.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
                lefiError(msg);
                */
                sprintf(msg, "Incorrect syntax defined for property LEF57_ARRAYSPACING: %s.\nLONGARRAY is defined after CUTSPACING.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
                lefError(1308, msg);
                free(wrkingStr);
                return;
            }
            hasLongArray = 1;
            value = strtok(NULL, " ");
        } else if (strcmp(value, "WIDTH") == 0) {
            if (cutSpacing != 0) { // make sure syntax has correct order 
                /*
                sprintf(msg, "ERROR (LEFPARS-1309): Incorrect syntax defined for property LEF57_ARRAYSPACING: %s.\nWIDTH is defined after CUTSPACING.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
                lefiError(msg);
                */
                sprintf(msg, "Incorrect syntax defined for property LEF57_ARRAYSPACING: %s.\nWIDTH is defined after CUTSPACING.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
                lefError(1309, msg);
                free(wrkingStr);
                return;
            }
            value = strtok(NULL, " ");
            viaWidth = atof(value);
            value = strtok(NULL, " ");
        } else if (strcmp(value, "CUTSPACING") == 0) {
            if (cutSpacing != 0) { // make sure syntax has correct order 
                /*
                sprintf(msg, "ERROR (LEFPARS-1310): Incorrect syntax defined for property LEF57_ARRAYSPACING: %s.\nCUTSPACING has defined more than once.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
                lefiError(msg);
                */
                sprintf(msg, "Incorrect syntax defined for property LEF57_ARRAYSPACING: %s.\nCUTSPACING has defined more than once.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
                lefError(1310, msg);
                free(wrkingStr);
                return;
            }
            value = strtok(NULL, " ");
            cutSpacing = atof(value);
            /*
                     setArraySpacing(hasLongArray, viaWidth, cutSpacing);
            */
            if (hasLongArray)
                setArraySpacingLongArray();
            setArraySpacingWidth(viaWidth);
            setArraySpacingCut(cutSpacing);
            value = strtok(NULL, " ");
        } else if (strcmp(value, "ARRAYCUTS") == 0) {
            if (cutSpacing == 0) { // make sure cutSpacing is already set 
                /*
                sprintf(msg, "ERROR (LEFPARS-1311): Incorrect syntax defined for property LEF57_ARRAYSPACING: %s.\nCUTSPACING which is required is either has not been defined or defined in a wrong location.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
                lefiError(msg);
                */
                sprintf(msg, "Incorrect syntax defined for property LEF57_ARRAYSPACING: %s.\nCUTSPACING which is required is either has not been defined or defined in a wrong location.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
                lefError(1311, msg);
                free(wrkingStr);
                return;
            }
            value = strtok(NULL, " ");
            arrayCuts = atoi(value);
            value = strtok(NULL, " ");
            if (strcmp(value, "SPACING") != 0) {
                /*
                            sprintf(msg, "ERROR (LEFPARS-1312): Incorrect syntax defined for  property LEF57_ARRAYSPACING: %s.\nSPACING should be defined with ARRAYCUTS.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
                            lefiError(msg);
                */
                sprintf(msg, "Incorrect syntax defined for  property LEF57_ARRAYSPACING: %s.\nSPACING should be defined with ARRAYCUTS.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
                lefError(1312, msg);
                free(wrkingStr);
                return;
            }
            value = strtok(NULL, " ");
            arraySpacing = atof(value);
            /*
                     addArrayCuts(arrayCuts, arraySpacing);
            */
            addArraySpacingArray(arrayCuts, arraySpacing);
            value = strtok(NULL, " ");
            hasArrayCut = 1;
        } else {  // Doesn't match any of the format
            /*
            sprintf(msg, "ERROR (LEFPARS-1313): Incorrect syntax defined for property LEF57_ARRAYSPACING: %s.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
            lefiError(msg);
            */
            sprintf(msg, "Incorrect syntax defined for property LEF57_ARRAYSPACING: %s.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
            lefError(1313, msg);
            free(wrkingStr);
            return;

        }
    }

    if (hasArrayCut == 0) {  // ARRAYCUTS is required
        /*
        sprintf(msg, "ERROR (LEFPARS-1314): Incorrect syntax defined for property LEF57_ARRAYSPACING: %s\nARRAYCUTS is required but has not been defined.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
        */
        sprintf(msg, "Incorrect syntax defined for property LEF57_ARRAYSPACING: %s\nARRAYCUTS is required but has not been defined.\nCorrect syntax is ARRAYSPACING [LONGARRAY] [WIDTH viaWidth] CUTSPACING cutSpacing\n\tARRAYCUTS arrayCuts SPACING arraySpacing ...\n", values_[index]);
        lefError(1314, msg);
    }

    free(wrkingStr);
    return;
}

// PRIVATE 5.7
// MINSTEP minStepLength
// [MAXEDGES maxEdges] ; 
// Save the value lefData->first to make sure the syntax that is supported by the parser
void
lefiLayer::parseMinstep(int index)
{
    char    *wrkingStr = strdup(values_[index]);
    char    *value;
    double  minStepLength = 0, minAdjLength = 0, minBetLength = 0;
    int     maxEdges = 0, xSameCorners = 0, done = 0;
    char    msg[1024];

    if (strcmp(type(), "ROUTING") != 0) {
        /*
              sprintf(msg, "ERROR (LEFPARS-1323): The property LEF57_MINSTEP with value %s is for TYPE ROUTING only.\nThe current layer has the TYPE %s.\nUpdate the property of your lef file with the correct syntax or remove this property from your lef file.\n",
              values_[index], type());
              lefiError(msg);
        */
        sprintf(msg, "The property LEF57_MINSTEP with value %s is for TYPE ROUTING only.\nThe current layer has the TYPE %s.\nUpdate the property of your lef file with the correct syntax or remove this property from your lef file.\n",
                values_[index], type());
        lefError(1323, msg);
        free(wrkingStr);
        return;
    }

    value = strtok(wrkingStr, " ");
    if (strcmp(value, "MINSTEP") != 0) {
        /*
              sprintf(msg, "ERROR (LEFPARS-1315): Incorrect syntax defined for property LEF57_MINSTEP: %s.\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
              lefiError(msg);
        */
        sprintf(msg, "Incorrect syntax defined for property LEF57_MINSTEP: %s.\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
        lefError(1315, msg);
        free(wrkingStr);
        return;
    }

    value = strtok(NULL, " ");
    minStepLength = atof(value);
    //addMinstep(minStepLength);
    value = strtok(NULL, " ");
    while (done == 0) {
        if (value && *value != '\n') {
            if (strcmp(value, "MAXEDGES") == 0) {
                // MAXEDGES maxEdges
                if (maxEdges) { // MAXEDGES has already defined
                    /*
                    sprintf(msg, "ERROR (LEFPARS-1315): Incorrect syntax defined for property LEF57_MINSTEP: %s\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
                    lefiError(msg);
                    */
                    sprintf(msg, "Incorrect syntax defined for property LEF57_MINSTEP: %s\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
                    lefError(1315, msg);
                    free(wrkingStr);   // done parsing
                    return;
                }
                value = strtok(NULL, " ");
                maxEdges = atoi(value);
                //addMinstepMaxedges(maxEdges);
                value = strtok(NULL, " ");
            } else if (strcmp(value, "MINADJACENTLENGTH") == 0) {
                if (minBetLength) {
                    // MINBETWEENLENGTH has defined, it is either MINADJACENTLENGTH
                    // or MINBETWEENLENGTH but not both
                    /*
                    sprintf(msg, "ERROR (LEFPARS-1315): Incorrect syntax defined for property LEF57_MINSTEP: %s\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
                    lefiError(msg);
                    */
                    sprintf(msg, "Incorrect syntax defined for property LEF57_MINSTEP: %s\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
                    lefError(1315, msg);
                    free(wrkingStr);   // done parsing
                    return;
                }
                value = strtok(NULL, " ");
                minAdjLength = atof(value);
                //addMinstepMinAdjLength(minAdjLength);
                value = strtok(NULL, " ");
            } else if (strcmp(value, "MINBETWEENLENGTH") == 0) {
                if (minAdjLength) {
                    // minadjACENTLENGTH has defined, it is either MINBETWEENLENGTH
                    // or minADJACENTLENGTH but not both
                    /*
                    sprintf(msg, "ERROR (LEFPARS-1315): Incorrect syntax defined for property LEF57_MINSTEP: %s\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
                    lefierror(msg);
                    */
                    sprintf(msg, "Incorrect syntax defined for property LEF57_MINSTEP: %s\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
                    lefError(1315, msg);
                    free(wrkingStr);   // done parsing
                    return;
                }
                value = strtok(NULL, " ");
                minBetLength = atof(value);
                //addMinstepMinBetLength(minBetLength);
                value = strtok(NULL, " ");
            } else if (strcmp(value, "EXCEPTSAMECORNERS") == 0) {
                if (minBetLength) {
                    xSameCorners = 1;
                    //addMinstepXSameCorners();
                    value = strtok(NULL, " ");
                } else {
                    /*
                                   sprintf(msg, "ERROR (LEFPARS-1315): Incorrect syntax defined for property LEF57_MINSTEP: %s\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
                                   lefierror(msg);
                    */
                    sprintf(msg, "Incorrect syntax defined for property LEF57_MINSTEP: %s\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
                    lefError(1315, msg);
                    free(wrkingStr);   // done parsing
                    return;
                }
            } else if (strcmp(value, ";") != 0) {
                // an invalid value
                /*
                sprintf(msg, "ERROR (LEFPARS-1315): Incorrect syntax defined for property LEF57_MINSTEP: %s.\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
                lefierror(msg);
                */
                sprintf(msg, "Incorrect syntax defined for property LEF57_MINSTEP: %s.\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
                lefError(1315, msg);
                free(wrkingStr);   // done parsing
                return;
            } else
                done = 1;
        } else {
            // done parsing without ;
            /*
            sprintf(msg, "eRROR (LEFPARS-1315): Incorrect syntax defined for property LEF57_MINSTEP: %s\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
            lefierror(msg);
            */
            sprintf(msg, "incorrect syntax defined for property LEF57_MINSTEP: %s\nCorrect syntax is \"MINSTEP minStepLength [MAXEDGES maxEdges] [MINADJACENTLENGTH minAdjLength | MINBETWEENLENGTH minBetweenLength [EXCEPTSAMECORNERS]] ;\"\n", values_[index]);
            lefError(1315, msg);
            free(wrkingStr);   // done parsing
            return;
        }
    }

    if (minStepLength)
        addMinstep(minStepLength);
    if (maxEdges)
        addMinstepMaxedges(maxEdges);
    if (minAdjLength)
        addMinstepMinAdjLength(minAdjLength);
    if (minBetLength)
        addMinstepMinBetLength(minBetLength);
    if (xSameCorners)
        addMinstepXSameCorners();
    free(wrkingStr);
    return;
}

// PRIVATE 5.7
void
lefiLayer::parseAntennaCumRouting(int index)
{
    char    *wrkingStr = strdup(values_[index]);
    char    *value;
    char    msg[1024];

    value = strtok(wrkingStr, " ");
    if (strcmp(value, "ANTENNACUMROUTINGPLUSCUT") != 0) {
        /*
              sprintf(msg, "ERROR (LEFPARS-1316): Incorrect syntax defined for property LEF57_ANTENNACUMROUTINGPLUSCUT: %s.\nCorrect syntax is \"ANTANNACUMROUTINGPLUSCUT\"\n", values_[index]);
              lefiError(msg);
        */
        sprintf(msg, "Incorrect syntax defined for property LEF57_ANTENNACUMROUTINGPLUSCUT: %s.\nCorrect syntax is \"ANTANNACUMROUTINGPLUSCUT\"\n", values_[index]);
        lefError(1316, msg);
        free(wrkingStr);
        return;
    }

    setAntennaCumRoutingPlusCut();

    free(wrkingStr);
    return;
}

// PRIVATE 5.7
void
lefiLayer::parseAntennaGatePlus(int index)
{
    char    *wrkingStr = strdup(values_[index]);
    char    *value;
    double  pDiffFactor;
    char    msg[1024];

    value = strtok(wrkingStr, " ");
    if (strcmp(value, "ANTENNAGATEPLUSDIFF") != 0) {
        /*
              sprintf(msg, "ERROR (LEFPARS-1317): Incorrect syntax defined for property LEF57_ANTENNAGATEPLUSDIFF: %s.\nCorrect syntax is \"ANTENNAGATEPLUSDIFF plusDiffFactor\"\n", values_[index]);
              lefiError(msg);
        */
        sprintf(msg, "Incorrect syntax defined for property LEF57_ANTENNAGATEPLUSDIFF: %s.\nCorrect syntax is \"ANTENNAGATEPLUSDIFF plusDiffFactor\"\n", values_[index]);
        lefError(1317, msg);
        free(wrkingStr);
        return;
    }

    value = strtok(NULL, " ");
    pDiffFactor = atof(value);
    setAntennaGatePlusDiff(pDiffFactor);

    free(wrkingStr);
    return;
}

// PRIVATE 5.7
void
lefiLayer::parseAntennaAreaMinus(int index)
{
    char    *wrkingStr = strdup(values_[index]);
    char    *value;
    double  mDiffFactor;
    char    msg[1024];

    value = strtok(wrkingStr, " ");
    if (strcmp(value, "ANTENNAAREAMINUSDIFF") != 0) {
        /*
              sprintf(msg, "ERROR (LEFPARS-1318): Incorrect syntax defined for property LEF57_ANTENNAAREAMINUSDIFF: %s.\nCorrect syntax is \"ANTENNAAREAMINUSDIFF minusDiffFactor\"\n", values_[index]);
              lefiError(msg);
        */
        sprintf(msg, "Incorrect syntax defined for property LEF57_ANTENNAAREAMINUSDIFF: %s.\nCorrect syntax is \"ANTENNAAREAMINUSDIFF minusDiffFactor\"\n", values_[index]);
        lefError(1318, msg);
        free(wrkingStr);
        return;
    }

    value = strtok(NULL, " ");
    mDiffFactor = atof(value);
    setAntennaAreaMinusDiff(mDiffFactor);

    free(wrkingStr);
    return;
}

// PRIVATE 5.7
void
lefiLayer::parseAntennaAreaDiff(int index)
{
    char            *wrkingStr = strdup(values_[index]);
    char            *value;
    double          diffA, diffF;
    lefiAntennaPWL  *pwlPtr;
    int             done = 0;
    char            msg[1024];

    value = strtok(wrkingStr, " ");
    if (strcmp(value, "ANTENNAAREADIFFREDUCEPWL") != 0) {
        /*
              sprintf(msg, "ERROR (LEFPARS-1319): Incorrect syntax defined for property LEF57_ANTENNAAREADIFFREDUCEPWL: %s.\nCorrect syntax is \"ANTENNAAREADIFFREDUCEPWL (( diffArea1 metalDiffFactor1 ) ( diffArea2 metalDiffFactor2 )...)\"\n", values_[index]);
              lefiError(msg);
        */
        sprintf(msg, "Incorrect syntax defined for property LEF57_ANTENNAAREADIFFREDUCEPWL: %s.\nCorrect syntax is \"ANTENNAAREADIFFREDUCEPWL (( diffArea1 metalDiffFactor1 ) ( diffArea2 metalDiffFactor2 )...)\"\n", values_[index]);
        lefError(1319, msg);
        free(wrkingStr);
        return;
    }

    value = strtok(NULL, " ");
    if (strcmp(value, "(") == 0) { // beginning of ( ( d1 r1 ) ( d2 r2 ) ... )
        pwlPtr = lefiAntennaPWL::create();
        while (done == 0) {
            value = strtok(NULL, " ");
            if (strcmp(value, "(") == 0) {
                value = strtok(NULL, " ");
                diffA = atof(value);
                value = strtok(NULL, " ");
                diffF = atof(value);
                pwlPtr->addAntennaPWL(diffA, diffF);
                value = strtok(NULL, " ");
                if (strcmp(value, ")") != 0) {
                    break;
                }
            } else if (strcmp(value, ")") == 0)
                done = 1;
        }
        if (done) {
            setAntennaPWL(lefiAntennaADR, pwlPtr);
        } else {
            pwlPtr->Destroy();
            lefFree(pwlPtr);
        }
    }

    free(wrkingStr);
    return;
}

// PRIVATE 5.7
// [ENCLOSURE [ABOVE | BELOW] overhang1 overhang2
//     [WIDTH minWidth [EXCEPTEXTRACUT cutWithin]
//     |LENGTH minLength] ;
void
lefiLayer::parseLayerEnclosure(int index)
{
    char    *wrkingStr = strdup(values_[index]);
    char    *value;
    char    msg[1024];
    int     overh = 0, width = 0, except = 0, length = 0;
    char    *enclRule = 0;
    double  overhang1 = 0, overhang2 = 0, minWidth = 0, cutWithin = 0, minLength = 0;

    if (strcmp(type(), "CUT") != 0) {
        /*
              sprintf(msg, "ERROR (LEFPARS-1331): The property LEF57_ENCLOSURE with value %s is for TYPE CUT only.\nThe current layer has the TYPE %s.\nUpdate the property of your lef file with the correct syntax or remove this property from your lef file.\n",
              values_[index], type());
              lefiError(msg);
        */
        sprintf(msg, "The property LEF57_ENCLOSURE with value %s is for TYPE CUT only.\nThe current layer has the TYPE %s.\nUpdate the property of your lef file with the correct syntax or remove this property from your lef file.\n",
                values_[index], type());
        lefError(1331, msg);
        free(wrkingStr);
        return;
    }

    value = strtok(wrkingStr, " ");
    if (strcmp(value, "ENCLOSURE") != 0) {   // Unknown value
        /*
        sprintf(msg, "ERROR (LEFPARS-1330): Incorrect syntax defined for property LEF57_ENCLOSURE: %s\nCorrect syntax is \"ENCLOSURE [ABOVE|BELOW] overhang1 overhang2\n\t[WIDTH minWidth [EXCEPTEXTRACUT cutWithin]\n\t|LENGTH minLength] ;\"\n", values_[index]);
        lefiError(msg);
        */
        sprintf(msg, "Incorrect syntax defined for property LEF57_ENCLOSURE: %s\nCorrect syntax is \"ENCLOSURE [ABOVE|BELOW] overhang1 overhang2\n\t[WIDTH minWidth [EXCEPTEXTRACUT cutWithin]\n\t|LENGTH minLength] ;\"\n", values_[index]);
        lefError(1330, msg);
        free(wrkingStr);
        return;
    }

    value = strtok(NULL, " ");

    while (strcmp(value, ";") != 0) {
        if (strcmp(value, "CUTCLASS") == 0) {
            // This is 58 syntax but is not in OA data model.  Skip the parsing
            free(wrkingStr);
            return;
        } else if ((strcmp(value, "ABOVE") == 0) || (strcmp(value, "BELOW") == 0)) {
            // Parse the rest of the property value lefData->first and if it has the syntax
            // ENCLOSURE [ABOVE | BELOW] overhang1 overhang2
            //   [WIDTH minWidth [EXCEPTEXTRACUT cutWithin]
            //   |LENGTH minLength]
            if (overh) {
                /*
                            sprintf(msg, "ERROR (LEFPARS-1330): Incorrect syntax defined for property LEF57_ENCLOSURE: %s\nCorrect syntax is \"ENCLOSURE [ABOVE|BELOW] overhang1 overhang2\n\t[WIDTH minWidth [EXCEPTEXTRACUT cutWithin]\n\t|LENGTH minLength] ;\"\n", values_[index]);
                            lefiError(msg);
                */
                sprintf(msg, "Incorrect syntax defined for property LEF57_ENCLOSURE: %s\nCorrect syntax is \"ENCLOSURE [ABOVE|BELOW] overhang1 overhang2\n\t[WIDTH minWidth [EXCEPTEXTRACUT cutWithin]\n\t|LENGTH minLength] ;\"\n", values_[index]);
                lefError(1330, msg);
                free(wrkingStr);
                if (enclRule)
                    free(enclRule);
                return;
            }
            enclRule = strdup(value);
            value = strtok(NULL, " ");
        } else if (strcmp(value, "WIDTH") == 0) {
            if ((!overh)) {
                /*
                            sprintf(msg, "ERROR (LEFPARS-1330): Incorrect syntax defined for property LEF57_ENCLOSURE: %s\nCorrect syntax is \"ENCLOSURE [ABOVE|BELOW] overhang1 overhang2\n\t[WIDTH minWidth [EXCEPTEXTRACUT cutWithin]\n\t|LENGTH minLength] ;\"\n", values_[index]);
                            lefiError(msg);
                */
                sprintf(msg, "Incorrect syntax defined for property LEF57_ENCLOSURE: %s\nCorrect syntax is \"ENCLOSURE [ABOVE|BELOW] overhang1 overhang2\n\t[WIDTH minWidth [EXCEPTEXTRACUT cutWithin]\n\t|LENGTH minLength] ;\"\n", values_[index]);
                lefError(1330, msg);
                free(wrkingStr);
                if (enclRule)
                    free(enclRule);
                return;
            }
            minWidth = strtod(strtok(NULL, " "), 0);
            value = strtok(NULL, " ");
            width = 1;
            if (strcmp(value, "EXCEPTEXTRACUT") == 0) {  // continue with WIDTH
                except = 1;
                value = strtok(NULL, " ");
                cutWithin = strtod(value, 0);
                value = strtok(NULL, " ");
                if (strcmp(value, "NOSHAREDEDGE") == 0) {
                    // 5.8 syntax but not in OA data model
                    free(wrkingStr);
                    if (enclRule)
                        free(enclRule);
                    return;
                }
            }
        } else if (strcmp(value, "LENGTH") == 0) {
            if (width || (!overh)) {
                /*
                            sprintf(msg, "ERROR (LEFPARS-1330): Incorrect syntax defined for property LEF57_ENCLOSURE: %s\nCorrect syntax is \"ENCLOSURE [ABOVE|BELOW] overhang1 overhang2\n\t[WIDTH minWidth [EXCEPTEXTRACUT cutWithin\n\t|LENGTH minLength] ;\"\n", values_[index]);
                            lefiError(msg);
                */
                sprintf(msg, "Incorrect syntax defined for property LEF57_ENCLOSURE: %s\nCorrect syntax is \"ENCLOSURE [ABOVE|BELOW] overhang1 overhang2\n\t[WIDTH minWidth [EXCEPTEXTRACUT cutWithin\n\t|LENGTH minLength] ;\"\n", values_[index]);
                lefError(1330, msg);
                free(wrkingStr);
                if (enclRule)
                    free(enclRule);
                return;
            }
            minLength = strtod(strtok(NULL, " "), 0);
            value = strtok(NULL, " ");
            length = 1;
        } else {
            if (overh == 1) { // Already has overhang value
                /*
                sprintf(msg, "ERROR (LEFPARS-1330): Incorrect syntax defined for property LEF57_ENCLOSURE: %s\nCorrect syntax is \"ENCLOSURE [ABOVE|BELOW] overhang1 overhang2\n\t[WIDTH minWidth [EXCEPTEXTRACUT cutWithin]\n\t|LENGTH minLength] ;\"\n", values_[index]);
                lefiError(msg);
                */
                sprintf(msg, "Incorrect syntax defined for property LEF57_ENCLOSURE: %s\nCorrect syntax is \"ENCLOSURE [ABOVE|BELOW] overhang1 overhang2\n\t[WIDTH minWidth [EXCEPTEXTRACUT cutWithin]\n\t|LENGTH minLength] ;\"\n", values_[index]);
                lefError(1330, msg);
                free(wrkingStr);
                if (enclRule)
                    free(enclRule);
                return;
            }
            overhang1 = strtod(value, 0);
            overhang2 = strtod(strtok(NULL, " "), 0);
            overh = 1;      // set the flag on
            value = strtok(NULL, " ");
        }
    }
    if (!overh) {
        /*
              sprintf(msg, "ERROR (LEFPARS-1330): Incorrect syntax defined for property LEF57_ENCLOSURE: %s\nCorrect syntax is \"ENCLOSURE [ABOVE|BELOW] overhang1 overhang2\n\t[WIDTH minWidth [EXCEPTEXTRACUT cutWithin]\n\t|LENGTH minLength] ;\"\n", values_[index]);
        */
        sprintf(msg, "Incorrect syntax defined for property LEF57_ENCLOSURE: %s\nCorrect syntax is \"ENCLOSURE [ABOVE|BELOW] overhang1 overhang2\n\t[WIDTH minWidth [EXCEPTEXTRACUT cutWithin]\n\t|LENGTH minLength] ;\"\n", values_[index]);
        lefError(1330, msg);
    } else {
        addEnclosure(enclRule, overhang1, overhang2);

        if (width) {
            addEnclosureWidth(minWidth);
            if (except)
                addEnclosureExceptEC(cutWithin);
        }
        if (length)
            addEnclosureLength(minLength);
    }
    if (enclRule)
        free(enclRule);

    free(wrkingStr);
    return;
}

// 5.7
// This API will is created just for OA to call in 5.6 only.
// This API will be obsoleted in 5.7.
// It will look for all the properties in "this" that are type 'S' and
// property name starts with "LEF57_...
void
lefiLayer::parse65nmRules()
{
    int i;

    if (lefData->versionNum < 5.6)
        return;

    for (i = 0; i < numProps_; i++) {
        if ((strlen(names_[i]) > 6) && (types_[i] == 'S')) {
            if (strncmp(names_[i], "LEF57_", 6) == 0) {
                if (strcmp(names_[i], "LEF57_SPACING") == 0) {
                    parseSpacing(i);
                } else /* Not an OA data model
           if (strcmp(names_[i], "LEF57_MAXFLOATINGAREA") == 0) {
              parseMaxFloating(i);
           } else
*/
                if (strcmp(names_[i], "LEF57_ARRAYSPACING") == 0) {
                    parseArraySpacing(i);
                } else if (strcmp(names_[i], "LEF57_MINSTEP") == 0) {
                    parseMinstep(i);
                } else if (strcmp(names_[i], "LEF57_ANTENNACUMROUTINGPLUSCUT") == 0) {
                    parseAntennaCumRouting(i);
                } else if (strcmp(names_[i], "LEF57_ANTENNAGATEPLUSDIFF") == 0) {
                    parseAntennaGatePlus(i);
                } else if (strcmp(names_[i], "LEF57_ANTENNAAREAMINUSDIFF") == 0) {
                    parseAntennaAreaMinus(i);
                } else if (strcmp(names_[i], "LEF57_ANTENNAAREADIFFREDUCEPWL") == 0) {
                    parseAntennaAreaDiff(i);
                } else if (strcmp(names_[i], "LEF57_ENCLOSURE") == 0) {
                    parseLayerEnclosure(i);
                }
            }
        }
    }
}

// PRIVATE 5.8
// This function will parse LEF58_TYPE property value. It also checks
// if lef58 type is compatible with LAYER TYPE value.
void
lefiLayer::parseLayerType(int index)
{
    std::string propValue(values_[index]);
    int         tokenStart = 0;
    std::string firstToken = lefrSettings::getToken(propValue, tokenStart);

    // Wrong LEF58_TYPE syntax.
    if (firstToken != "TYPE") { 
        std::string msg = "Incorrect LEF58_TYPE property value syntax: '" + 
                           propValue + 
                           "'. Correct syntax: 'TYPE <type> ;'.\n";

        lefError(1329, msg.c_str());
        return;
    }

    std::string type(type_);
    std::string lef58Type(lefrSettings::getToken(propValue, tokenStart));
    std::string typesPair(lef58Type + " " + type);

    if (lefSettings->Lef58TypePairs.find(typesPair) != lefSettings->Lef58TypePairs.end()) {
        // In parser LayerType == lef58 type. 
        setLayerType(lef58Type.c_str());
        return;
    }

    std::string layerLef58Types = lefSettings->getLayerLef58Types(this->type_);

    // Wrong/incompatible lef58 type.
    if (layerLef58Types.empty()) {
        std::string msg = "Layers with TYPE " + 
                           type  + " cannot have LEF58_TYPE property.\n";

        lefError(1328, msg.c_str());        
    } else {
        std::string msg = "Property LEF58_TYPE has incorrect TYPE value: '" +
                          lef58Type + "'. For TYPE " +
                          type + " layers valid values are: " +
                          layerLef58Types + ".\n";

        lefError(1327, msg.c_str());  
    } 
}

// 5.8
// This API will is created just for OA to call in 5.7 only.
// This API will be obsoleted once 5.8 APIs are available and OA moves
// to using them.
// It will look for the properties in "this" that are type 'S' and
// property name is "LEF58_TYPE"
void
lefiLayer::parseLEF58Layer()
{
    int i;

    if (lefData->versionNum < 5.7)
        return;

    for (i = 0; i < numProps_; i++) {
        if (strlen(names_[i]) == 10) {
            if (strcmp(names_[i], "LEF58_TYPE") == 0)
                parseLayerType(i);
        }
    }
}

int
lefiLayer::need58PropsProcessing() const
{
    return lefData->versionNum >= 5.7;
}


END_LEFDEF_PARSER_NAMESPACE

