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
//  $Date: 2020/09/29 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#ifndef lefiArray_h
#define lefiArray_h

#include <cstdio>

#include "lefiKRDefs.hpp"
#include "lefiMisc.hpp"

BEGIN_LEF_PARSER_NAMESPACE

class lefiArrayFloorPlan
{
 public:
  void Init(const char* name);
  void Destroy();
  void addSitePattern(const char* typ, lefiSitePattern* s);

  int numPatterns() const;
  lefiSitePattern* pattern(int index) const;
  char* typ(int index) const;
  const char* name() const;

 protected:
  int numPatterns_;
  int patternsAllocated_;
  lefiSitePattern** patterns_;
  char** types_;
  char* name_;
};

class lefiArray
{
 public:
  lefiArray();
  void Init();

  void Destroy();
  ~lefiArray();

  void setName(const char* name);
  void addSitePattern(lefiSitePattern* s);
  void setTableSize(int tsize);
  void addDefaultCap(int minPins, double cap);
  void addCanPlace(lefiSitePattern* s);
  void addCannotOccupy(lefiSitePattern* s);
  void addTrack(lefiTrackPattern* t);
  void addGcell(lefiGcellPattern* g);
  void addFloorPlan(const char* name);
  void addSiteToFloorPlan(const char* typ, lefiSitePattern* p);
  void clear();
  void bump(void*** arr, int used, int* allocated);

  int numSitePattern() const;
  int numCanPlace() const;
  int numCannotOccupy() const;
  int numTrack() const;
  int numGcell() const;
  int hasDefaultCap() const;

  const char* name() const;
  lefiSitePattern* sitePattern(int index) const;
  lefiSitePattern* canPlace(int index) const;
  lefiSitePattern* cannotOccupy(int index) const;
  lefiTrackPattern* track(int index) const;
  lefiGcellPattern* gcell(int index) const;

  int tableSize() const;
  int numDefaultCaps() const;
  int defaultCapMinPins(int index) const;
  double defaultCap(int index) const;

  int numFloorPlans() const;
  const char* floorPlanName(int index) const;
  int numSites(int index) const;
  const char* siteType(int floorIndex, int siteIndex) const;
  lefiSitePattern* site(int floorIndex, int siteIndex) const;

  // Debug print
  void print(FILE* f) const;

 protected:
  int nameSize_{0};
  char* name_{nullptr};

  int patternsAllocated_{0};
  int numPatterns_{0};
  lefiSitePattern** pattern_{nullptr};

  int canAllocated_{0};
  int numCan_{0};
  lefiSitePattern** canPlace_{nullptr};

  int cannotAllocated_{0};
  int numCannot_{0};
  lefiSitePattern** cannotOccupy_{nullptr};

  int tracksAllocated_{0};
  int numTracks_{0};
  lefiTrackPattern** track_{nullptr};

  int gAllocated_{0};
  int numG_{0};
  lefiGcellPattern** gcell_{nullptr};

  int hasDefault_{0};
  int tableSize_{0};
  int numDefault_{0};
  int defaultAllocated_{0};
  int* minPins_{nullptr};
  double* caps_{nullptr};

  int numFloorPlans_{0};
  int floorPlansAllocated_{0};
  lefiArrayFloorPlan** floors_{nullptr};
};

END_LEF_PARSER_NAMESPACE

#endif
