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
#include "lefiArray.hpp"
#include "lefiMisc.hpp"
#include "lefiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE
// *****************************************************************************
// lefiArrayFloorPlan
// *****************************************************************************

void
lefiArrayFloorPlan::Init(const char *name)
{
    int len = strlen(name) + 1;
    name_ = (char*) lefMalloc(len);
    strcpy(name_, CASE(name));
    numPatterns_ = 0;
    patternsAllocated_ = 2;
    types_ = (char**) lefMalloc(sizeof(char*) * 2);
    patterns_ = (lefiSitePattern**) lefMalloc(sizeof(lefiSitePattern*) * 2);
}

void
lefiArrayFloorPlan::Destroy()
{
    lefiSitePattern *s;
    int             i;
    for (i = 0; i < numPatterns_; i++) {
        s = patterns_[i];
        s->Destroy();
        lefFree((char*) s);
        lefFree((char*) (types_[i]));
    }
    lefFree((char*) (types_));
    lefFree((char*) (patterns_));
    lefFree(name_);
}

void
lefiArrayFloorPlan::addSitePattern(const char       *typ,
                                   lefiSitePattern  *s)
{
    int len = strlen(typ) + 1;
    if (numPatterns_ == patternsAllocated_) {
        int             i;
        int             lim;
        char            **nc;
        lefiSitePattern **np;

        if (patternsAllocated_ == 0)
            lim = patternsAllocated_ = 2;
        else
            lim = patternsAllocated_ = patternsAllocated_ * 2;
        nc = (char**) lefMalloc(sizeof(char*) * lim);
        np = (lefiSitePattern**) lefMalloc(sizeof(lefiSitePattern*) * lim);
        lim /= 2;
        for (i = 0; i < lim; i++) {
            nc[i] = types_[i];
            np[i] = patterns_[i];
        }
        lefFree((char*) (types_));
        lefFree((char*) (patterns_));
        types_ = nc;
        patterns_ = np;
    }
    types_[numPatterns_] = (char*) lefMalloc(len);
    strcpy(types_[numPatterns_], typ);
    patterns_[numPatterns_] = s;
    numPatterns_ += 1;
}

int
lefiArrayFloorPlan::numPatterns() const
{
    return numPatterns_;
}

lefiSitePattern *
lefiArrayFloorPlan::pattern(int index) const
{
    return patterns_[index];
}

char *
lefiArrayFloorPlan::typ(int index) const
{
    return types_[index];
}

const char *
lefiArrayFloorPlan::name() const
{
    return name_;
}

// *****************************************************************************
// lefiArray
// *****************************************************************************
lefiArray::lefiArray()
: nameSize_(0),
  name_(NULL),
  patternsAllocated_(0),
  numPatterns_(0),
  pattern_(NULL),
  canAllocated_(0),
  numCan_(0),
  canPlace_(NULL),
  cannotAllocated_(0),
  numCannot_(0),
  cannotOccupy_(NULL),
  tracksAllocated_(0),
  numTracks_(0),
  track_(NULL),
  gAllocated_(0),
  numG_(0),
  gcell_(NULL),
  hasDefault_(0),
  tableSize_(0),
  numDefault_(0),
  defaultAllocated_(0),
  minPins_(NULL),
  caps_(NULL),
  numFloorPlans_(0),
  floorPlansAllocated_(0),
  floors_(0)
{
    Init();
}

void
lefiArray::Init()
{
    nameSize_ = 16;
    name_ = (char*) lefMalloc(16);

    numPatterns_ = 0;
    patternsAllocated_ = 0;
    bump((void***) (&(pattern_)), numPatterns_, &(patternsAllocated_));

    numCan_ = 0;
    canAllocated_ = 0;
    bump((void***) (&(canPlace_)), numCan_, &(canAllocated_));

    numCannot_ = 0;
    cannotAllocated_ = 0;
    bump((void***) (&(cannotOccupy_)), numCannot_, &(cannotAllocated_));

    numTracks_ = 0;
    tracksAllocated_ = 0;
    bump((void***) (&(track_)), numTracks_, &(tracksAllocated_));

    numG_ = 0;
    gAllocated_ = 0;
    bump((void***) (&(gcell_)), numG_, &(gAllocated_));

    numDefault_ = 0;
    defaultAllocated_ = 4;
    minPins_ = (int*) lefMalloc(sizeof(int) * 4);
    caps_ = (double*) lefMalloc(sizeof(double) * 4);

    floorPlansAllocated_ = 0;
    numFloorPlans_ = 0;
    bump((void***) (&(floors_)), numFloorPlans_,
         &(floorPlansAllocated_));
}

void
lefiArray::Destroy()
{
    clear();

    lefFree((char*) (name_));
    lefFree((char*) (caps_));
    lefFree((char*) (minPins_));
    lefFree((char*) (floors_));
    lefFree((char*) (track_));
    lefFree((char*) (gcell_));
    lefFree((char*) (cannotOccupy_));
    lefFree((char*) (canPlace_));
    lefFree((char*) (pattern_));
}

lefiArray::~lefiArray()
{
    Destroy();
}

void
lefiArray::addSitePattern(lefiSitePattern *s)
{
    /*
      if (numPatterns_ == patternsAllocated_)
        bump((void***)(&(pattern_)), numPatterns_,
        &(patternsAllocated_));
    */
    if (numPatterns_ == patternsAllocated_) {
        lefiSitePattern **tpattern;
        int             i;

        if (patternsAllocated_ == 0)
            patternsAllocated_ = 2;
        else
            patternsAllocated_ = patternsAllocated_ * 2;
        tpattern = (lefiSitePattern**) lefMalloc(sizeof(lefiSitePattern*) *
                                                 patternsAllocated_);
        for (i = 0; i < numPatterns_; i++) {
            tpattern[i] = pattern_[i];
        }
        if (pattern_)
            lefFree((char*) (pattern_));
        pattern_ = tpattern;
        /*
            bump((void***)(&(pattern_)), numPatterns_,
            &(patternsAllocated_));
        */
    }

    pattern_[numPatterns_] = s;
    numPatterns_ += 1;
}

void
lefiArray::setName(const char *name)
{
    int len = strlen(name) + 1;
    if (len > nameSize_) {
        lefFree(name_);
        name_ = (char*) lefMalloc(len);
        nameSize_ = len;
    }
    strcpy(name_, CASE(name));
}

void
lefiArray::setTableSize(int tsize)
{
    tableSize_ = tsize;
    hasDefault_ = 1;
}

void
lefiArray::addDefaultCap(int    minPins,
                         double cap)
{
    if (numDefault_ == defaultAllocated_) {
        int     i;
        int     lim;
        double  *nc;
        int     *np;

        if (defaultAllocated_ == 0)
            lim = defaultAllocated_ = 2;
        else
            lim = defaultAllocated_ * 2;
        defaultAllocated_ = lim;
        nc = (double*) lefMalloc(sizeof(double) * lim);
        np = (int*) lefMalloc(sizeof(int) * lim);
        lim /= 2;
        for (i = 0; i < lim; i++) {
            nc[i] = caps_[i];
            np[i] = minPins_[i];
        }
        lefFree((char*) (caps_));
        lefFree((char*) (minPins_));
        caps_ = nc;
        minPins_ = np;
    }
    caps_[numDefault_] = cap;
    minPins_[numDefault_] = minPins;
    numDefault_ += 1;
}

void
lefiArray::addCanPlace(lefiSitePattern *s)
{
    if (numCan_ == canAllocated_) {
        lefiSitePattern **cplace;
        int             i;

        if (canAllocated_ == 0)
            canAllocated_ = 2;
        else
            canAllocated_ = canAllocated_ * 2;
        cplace = (lefiSitePattern**) lefMalloc(sizeof(lefiSitePattern*) *
                                               canAllocated_);
        for (i = 0; i < numCan_; i++) {
            cplace[i] = canPlace_[i];
        }
        if (canPlace_)
            lefFree((char*) (canPlace_));
        canPlace_ = cplace;
    }
    /*
        bump((void***)(&(canPlace_)), numCan_, &(canAllocated_));
    */
    canPlace_[numCan_] = s;
    numCan_ += 1;
}

void
lefiArray::addCannotOccupy(lefiSitePattern *s)
{
    if (numCannot_ == cannotAllocated_) {
        lefiSitePattern **cnplace;
        int             i;

        if (cannotAllocated_ == 0)
            cannotAllocated_ = 2;
        else
            cannotAllocated_ = cannotAllocated_ * 2;
        cnplace = (lefiSitePattern**) lefMalloc(sizeof(lefiSitePattern*) *
                                                cannotAllocated_);
        for (i = 0; i < numCannot_; i++) {
            cnplace[i] = cannotOccupy_[i];
        }
        if (cannotOccupy_)
            lefFree((char*) (cannotOccupy_));
        cannotOccupy_ = cnplace;
    }
    /*
      if (numCannot_ == cannotAllocated_)
        bump((void***)(&(cannotOccupy_)), numCannot_,
        &(cannotAllocated_));
    */
    cannotOccupy_[numCannot_] = s;
    numCannot_ += 1;
}

void
lefiArray::addTrack(lefiTrackPattern *t)
{
    if (numTracks_ == tracksAllocated_) {
        lefiTrackPattern    **tracks;
        int                 i;

        if (tracksAllocated_ == 0)
            tracksAllocated_ = 2;
        else
            tracksAllocated_ = tracksAllocated_ * 2;
        tracks = (lefiTrackPattern**) lefMalloc(sizeof(lefiTrackPattern*) *
                                                tracksAllocated_);
        for (i = 0; i < numTracks_; i++) {
            tracks[i] = track_[i];
        }
        if (track_)
            lefFree((char*) (track_));
        track_ = tracks;
    }
    /*
      if (numTracks_ == tracksAllocated_)
        bump((void***)(&(track_)), numTracks_, &(tracksAllocated_));
    */
    track_[numTracks_] = t;
    numTracks_ += 1;
}

void
lefiArray::addGcell(lefiGcellPattern *g)
{
    if (numG_ == gAllocated_) {
        lefiGcellPattern    **cells;
        int                 i;

        if (gAllocated_ == 0)
            gAllocated_ = 2;
        else
            gAllocated_ = gAllocated_ * 2;
        cells = (lefiGcellPattern**) lefMalloc(sizeof(lefiGcellPattern*) *
                                               gAllocated_);
        for (i = 0; i < numG_; i++) {
            cells[i] = gcell_[i];
        }
        if (gcell_)
            lefFree((char*) (gcell_));
        gcell_ = cells;
    }
    /*
      if (numG_ == gAllocated_)
        bump((void***)(&(gcell_)), numG_, &(gAllocated_));
    */
    gcell_[numG_] = g;
    numG_ += 1;
}

void
lefiArray::addFloorPlan(const char *name)
{
    lefiArrayFloorPlan *f;
    if (numFloorPlans_ == floorPlansAllocated_) {
        int                 i;
        lefiArrayFloorPlan  **tf;

        if (floorPlansAllocated_ == 0)
            floorPlansAllocated_ = 2;
        else
            floorPlansAllocated_ = floorPlansAllocated_ * 2;
        tf = (lefiArrayFloorPlan**) lefMalloc(sizeof(lefiArrayFloorPlan*) *
                                              floorPlansAllocated_);
        for (i = 0; i < numFloorPlans_; i++) {
            tf[i] = floors_[i];
        }
        if (floors_)
            lefFree((char*) (floors_));
        floors_ = tf;
    }
    /*
      if (numFloorPlans_ == floorPlansAllocated_) {
        bump((void***)(&(floors_)), numFloorPlans_,
        &(floorPlansAllocated_));
      }
    */
    f = (lefiArrayFloorPlan*) lefMalloc(sizeof(lefiArrayFloorPlan));
    f->Init(name);
    floors_[numFloorPlans_] = f;
    numFloorPlans_ += 1;
}

void
lefiArray::addSiteToFloorPlan(const char        *typ,
                              lefiSitePattern   *s)
{
    lefiArrayFloorPlan *f = floors_[numFloorPlans_ - 1];
    f->addSitePattern(typ, s);
}

void
lefiArray::bump(void    ***arr,
                int     used,
                int     *allocated)
{
    int     size = *allocated * 2;
    int     i;
    void    **newa;
    if (size == 0)
        size = 2;
    newa = (void**) lefMalloc(sizeof(void*) * size);

    for (i = 0; i < used; i++) {
        newa[i] = (*arr)[i];
    }

    if (*arr)
        lefFree((char*) (*arr));
    *allocated = size;
    *arr = newa;
}

void
lefiArray::clear()
{
    int                 i;
    lefiSitePattern     *p;
    lefiGcellPattern    *g;
    lefiTrackPattern    *t;
    lefiArrayFloorPlan  *f;

    for (i = 0; i < numPatterns_; i++) {
        p = pattern_[i];
        p->Destroy();
        lefFree((char*) p);
    }
    numPatterns_ = 0;

    for (i = 0; i < numCan_; i++) {
        p = canPlace_[i];
        p->Destroy();
        lefFree((char*) p);
    }
    numCan_ = 0;

    for (i = 0; i < numCannot_; i++) {
        p = cannotOccupy_[i];
        p->Destroy();
        lefFree((char*) p);
    }
    numCannot_ = 0;

    for (i = 0; i < numTracks_; i++) {
        t = track_[i];
        t->Destroy();
        lefFree((char*) t);
    }
    numTracks_ = 0;

    for (i = 0; i < numG_; i++) {
        g = gcell_[i];
        g->Destroy();
        lefFree((char*) g);
    }
    numG_ = 0;

    hasDefault_ = 0;
    tableSize_ = 0;
    numDefault_ = 0;

    for (i = 0; i < numFloorPlans_; i++) {
        f = floors_[i];
        f->Destroy();
        lefFree((char*) f);
    }
    numFloorPlans_ = 0;

}

int
lefiArray::numSitePattern() const
{
    return numPatterns_;
}

int
lefiArray::numCanPlace() const
{
    return numCan_;
}

int
lefiArray::numCannotOccupy() const
{
    return numCannot_;
}

int
lefiArray::numTrack() const
{
    return numTracks_;
}

int
lefiArray::numGcell() const
{
    return numG_;
}

int
lefiArray::hasDefaultCap() const
{
    return hasDefault_;
}

const char *
lefiArray::name() const
{
    return name_;
}

lefiSitePattern *
lefiArray::sitePattern(int index) const
{
    return pattern_[index];
}

lefiSitePattern *
lefiArray::canPlace(int index) const
{
    return canPlace_[index];
}

lefiSitePattern *
lefiArray::cannotOccupy(int index) const
{
    return cannotOccupy_[index];
}

lefiTrackPattern *
lefiArray::track(int index) const
{
    return track_[index];
}

lefiGcellPattern *
lefiArray::gcell(int index) const
{
    return gcell_[index];
}

int
lefiArray::tableSize() const
{
    return tableSize_;
}

int
lefiArray::numDefaultCaps() const
{
    return numDefault_;
}

int
lefiArray::defaultCapMinPins(int index) const
{
    return minPins_[index];
}

double
lefiArray::defaultCap(int index) const
{
    return caps_[index];
}

int
lefiArray::numFloorPlans() const
{
    return numFloorPlans_;
}

const char *
lefiArray::floorPlanName(int index) const
{
    const lefiArrayFloorPlan *f = floors_[index];
    return f->name();
}

int
lefiArray::numSites(int index) const
{
    const lefiArrayFloorPlan *f = floors_[index];
    return f->numPatterns();
}

const char *
lefiArray::siteType(int index,
                    int j) const
{
    const lefiArrayFloorPlan *f = floors_[index];
    return f->typ(j);
}

lefiSitePattern *
lefiArray::site(int index,
                int j) const
{
    const lefiArrayFloorPlan *f = floors_[index];
    return f->pattern(j);
}

void
lefiArray::print(FILE *f) const
{
    fprintf(f, "ARRAY %s\n", name());
}

END_LEFDEF_PARSER_NAMESPACE

