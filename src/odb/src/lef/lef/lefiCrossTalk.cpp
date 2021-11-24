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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lex.h"
#include "lefiCrossTalk.hpp"
#include "lefiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

// *****************************************************************************
// lefiNoiseVictim
// *****************************************************************************

lefiNoiseVictim::lefiNoiseVictim(double d)
: length_(0.0),
  numNoises_(0),
  noisesAllocated_(0),
  noises_(NULL)
{
    Init(d);
}

void
lefiNoiseVictim::Init(double d)
{
    length_ = d;

    numNoises_ = 0;
    noisesAllocated_ = 2;
    noises_ = (double*) lefMalloc(sizeof(double) * 2);
}

void
lefiNoiseVictim::clear()
{
    numNoises_ = 0;
}

void
lefiNoiseVictim::Destroy()
{
    clear();
    lefFree((char*) (noises_));
}

lefiNoiseVictim::~lefiNoiseVictim()
{
    Destroy();
}

void
lefiNoiseVictim::addVictimNoise(double d)
{
    if (numNoises_ == noisesAllocated_) {
        int     max;
        double  *ne;
        int     i;

        if (noisesAllocated_ == 0) {
            max = noisesAllocated_ = 2;
            numNoises_ = 0;
        } else
            max = noisesAllocated_ = numNoises_ * 2;
        ne = (double*) lefMalloc(sizeof(double) * max);
        max /= 2;
        for (i = 0; i < max; i++)
            ne[i] = noises_[i];
        lefFree((char*) (noises_));
        noises_ = ne;
    }
    noises_[numNoises_] = d;
    numNoises_ += 1;
}

int
lefiNoiseVictim::numNoises() const
{
    return numNoises_;
}

double
lefiNoiseVictim::noise(int index) const
{
    return noises_[index];
}

double
lefiNoiseVictim::length() const
{
    return length_;
}

// *****************************************************************************
// lefiNoiseResistance
// *****************************************************************************

lefiNoiseResistance::lefiNoiseResistance()
: numNums_(0),
  numsAllocated_(0),
  nums_(NULL),
  numVictims_(0),
  victimsAllocated_(0),
  victims_(NULL)
{
    Init();
}

void
lefiNoiseResistance::Init()
{
    numNums_ = 0;
    numsAllocated_ = 1;
    nums_ = (double*) lefMalloc(sizeof(double) * 1);

    numVictims_ = 0;
    victimsAllocated_ = 2;
    victims_ = (lefiNoiseVictim**) lefMalloc(sizeof(
                                             lefiNoiseVictim*) * 2);
}

void
lefiNoiseResistance::clear()
{
    int             i;
    lefiNoiseVictim *r;
    int             max = numVictims_;

    for (i = 0; i < max; i++) {
        r = victims_[i];
        r->Destroy();
        lefFree((char*) r);
    }
    numVictims_ = 0;
    numNums_ = 0;
}

void
lefiNoiseResistance::Destroy()
{
    clear();
    lefFree((char*) (nums_));
    lefFree((char*) (victims_));
}

lefiNoiseResistance::~lefiNoiseResistance()
{
    Destroy();
}

void
lefiNoiseResistance::addResistanceNumber(double d)
{
    if (numNums_ == numsAllocated_) {
        int     max;
        double  *ne;
        int     i;

        if (numsAllocated_ == 0) {
            max = numsAllocated_ = 2;
            numNums_ = 0;
        } else
            max = numsAllocated_ = numNums_ * 2;
        ne = (double*) lefMalloc(sizeof(double) * max);
        max /= 2;
        for (i = 0; i < max; i++)
            ne[i] = nums_[i];
        lefFree((char*) (nums_));
        nums_ = ne;
    }
    nums_[numNums_] = d;
    numNums_ += 1;
}

void
lefiNoiseResistance::addVictimNoise(double d)
{
    lefiNoiseVictim *r = victims_[numVictims_ - 1];
    r->addVictimNoise(d);
}

void
lefiNoiseResistance::addVictimLength(double d)
{
    lefiNoiseVictim *r;
    if (numVictims_ == victimsAllocated_) {
        int             max;
        lefiNoiseVictim **ne;
        int             i;

        if (victimsAllocated_ == 0) {
            max = victimsAllocated_ = 2;
            numVictims_ = 0;
        } else
            max = victimsAllocated_ = numVictims_ * 2;

        ne = (lefiNoiseVictim**) lefMalloc(sizeof(lefiNoiseVictim*) * max);
        max /= 2;
        for (i = 0; i < max; i++)
            ne[i] = victims_[i];
        lefFree((char*) (victims_));
        victims_ = ne;
    }
    r = (lefiNoiseVictim*) lefMalloc(sizeof(lefiNoiseVictim));
    r->Init(d);
    victims_[numVictims_] = r;
    numVictims_ += 1;
}

int
lefiNoiseResistance::numVictims() const
{
    return numVictims_;
}

lefiNoiseVictim *
lefiNoiseResistance::victim(int index) const
{
    return victims_[index];
}

int
lefiNoiseResistance::numNums() const
{
    return numNums_;
}

double
lefiNoiseResistance::num(int index) const
{
    return nums_[index];
}

// *****************************************************************************
// lefiNoiseEdge
// *****************************************************************************

lefiNoiseEdge::lefiNoiseEdge()
{
    Init();
}

void
lefiNoiseEdge::Init()
{
    edge_ = 0;

    numResistances_ = 0;
    resistancesAllocated_ = 2;
    resistances_ = (lefiNoiseResistance**) lefMalloc(sizeof(
                                                     lefiNoiseResistance*) * 2);
}

void
lefiNoiseEdge::clear()
{
    int                 i;
    lefiNoiseResistance *r;
    int                 maxr = numResistances_;

    for (i = 0; i < maxr; i++) {
        r = resistances_[i];
        r->Destroy();
        lefFree((char*) r);
    }

    edge_ = 0;
    numResistances_ = 0;
}

void
lefiNoiseEdge::Destroy()
{
    clear();
    lefFree((char*) (resistances_));
}

lefiNoiseEdge::~lefiNoiseEdge()
{
    Destroy();
}

void
lefiNoiseEdge::addEdge(double d)
{
    edge_ = d;
}

void
lefiNoiseEdge::addResistanceNumber(double d)
{
    lefiNoiseResistance *r = resistances_[numResistances_ - 1];
    r->addResistanceNumber(d);
}


void
lefiNoiseEdge::addResistance()
{
    lefiNoiseResistance *r;
    if (numResistances_ == resistancesAllocated_) {
        int                 max;
        lefiNoiseResistance **ne;
        int                 i;

        if (resistancesAllocated_ == 0) {
            max = resistancesAllocated_ = 2;
            numResistances_ = 0;
        } else
            max = resistancesAllocated_ = numResistances_ * 2;
        ne = (lefiNoiseResistance**) lefMalloc(sizeof(lefiNoiseResistance*) * max);
        max /= 2;
        for (i = 0; i < max; i++)
            ne[i] = resistances_[i];
        lefFree((char*) (resistances_));
        resistances_ = ne;
    }
    r = (lefiNoiseResistance*) lefMalloc(sizeof(lefiNoiseResistance));
    r->Init();
    resistances_[numResistances_] = r;
    numResistances_ += 1;
}

void
lefiNoiseEdge::addVictimNoise(double d)
{
    lefiNoiseResistance *r = resistances_[numResistances_ - 1];
    r->addVictimNoise(d);
}

void
lefiNoiseEdge::addVictimLength(double d)
{
    lefiNoiseResistance *r = resistances_[numResistances_ - 1];
    r->addVictimLength(d);
}

int
lefiNoiseEdge::numResistances()
{
    return numResistances_;
}

lefiNoiseResistance *
lefiNoiseEdge::resistance(int index)
{
    return resistances_[index];
}

double
lefiNoiseEdge::edge()
{
    return edge_;
}

// *****************************************************************************
// lefiNoiseTable
// *****************************************************************************

lefiNoiseTable::lefiNoiseTable()
{
    Init();
}

void
lefiNoiseTable::Init()
{
    numEdges_ = 0;
    edgesAllocated_ = 2;
    edges_ = (lefiNoiseEdge**) lefMalloc(sizeof(lefiNoiseEdge*) * 2);
}

void
lefiNoiseTable::clear()
{
    int             i;
    lefiNoiseEdge   *r;
    int             max = numEdges_;

    for (i = 0; i < max; i++) {
        r = edges_[i];
        r->Destroy();
        lefFree((char*) r);
    }
    numEdges_ = 0;
}

void
lefiNoiseTable::Destroy()
{
    clear();
    lefFree((char*) (edges_));
}

lefiNoiseTable::~lefiNoiseTable()
{
    Destroy();
}

void
lefiNoiseTable::setup(int i)
{
    num_ = i;
    clear();
}

void
lefiNoiseTable::newEdge()
{
    lefiNoiseEdge *r;
    if (numEdges_ == edgesAllocated_) {
        int             max;
        lefiNoiseEdge   **ne;
        int             i;

        if (edgesAllocated_ == 0) {
            max = edgesAllocated_ = 2;
            numEdges_ = 0;
        } else
            max = edgesAllocated_ = numEdges_ * 2;
        ne = (lefiNoiseEdge**) lefMalloc(sizeof(lefiNoiseEdge*) * max);
        max /= 2;
        for (i = 0; i < max; i++)
            ne[i] = edges_[i];
        lefFree((char*) (edges_));
        edges_ = ne;
    }
    r = (lefiNoiseEdge*) lefMalloc(sizeof(lefiNoiseEdge));
    r->Init();
    edges_[numEdges_] = r;
    numEdges_ += 1;
}

void
lefiNoiseTable::addEdge(double d)
{
    lefiNoiseEdge *r = edges_[numEdges_ - 1];
    r->addEdge(d);
}

void
lefiNoiseTable::addResistance()
{
    lefiNoiseEdge *r = edges_[numEdges_ - 1];
    r->addResistance();
}

void
lefiNoiseTable::addResistanceNumber(double d)
{
    lefiNoiseEdge *r = edges_[numEdges_ - 1];
    r->addResistanceNumber(d);
}

void
lefiNoiseTable::addVictimLength(double d)
{
    lefiNoiseEdge *r = edges_[numEdges_ - 1];
    r->addVictimLength(d);
}

void
lefiNoiseTable::addVictimNoise(double d)
{
    lefiNoiseEdge *r = edges_[numEdges_ - 1];
    r->addVictimNoise(d);
}

int
lefiNoiseTable::num()
{
    return num_;
}

int
lefiNoiseTable::numEdges()
{
    return numEdges_;
}

lefiNoiseEdge *
lefiNoiseTable::edge(int index)
{
    return edges_[index];
}

// *****************************************************************************
// lefiCorrectionVictim
// *****************************************************************************

lefiCorrectionVictim::lefiCorrectionVictim(double d)
{
    Init(d);
}

void
lefiCorrectionVictim::Init(double d)
{
    length_ = d;

    numCorrections_ = 0;
    correctionsAllocated_ = 2;
    corrections_ = (double*) lefMalloc(sizeof(double) * 2);
}

void
lefiCorrectionVictim::clear()
{
    numCorrections_ = 0;
}

void
lefiCorrectionVictim::Destroy()
{
    clear();
    lefFree((char*) (corrections_));
}

lefiCorrectionVictim::~lefiCorrectionVictim()
{
    Destroy();
}

void
lefiCorrectionVictim::addVictimCorrection(double d)
{
    if (numCorrections_ == correctionsAllocated_) {
        int     max;
        double  *ne;
        int     i;

        if (correctionsAllocated_ == 0) {
            max = correctionsAllocated_ = 2;
            numCorrections_ = 0;
        } else
            max = correctionsAllocated_ = numCorrections_ * 2;
        ne = (double*) lefMalloc(sizeof(double) * max);
        max /= 2;
        for (i = 0; i < max; i++)
            ne[i] = corrections_[i];
        lefFree((char*) (corrections_));
        corrections_ = ne;
    }
    corrections_[numCorrections_] = d;
    numCorrections_ += 1;
}

int
lefiCorrectionVictim::numCorrections()
{
    return numCorrections_;
}

double
lefiCorrectionVictim::correction(int index)
{
    return corrections_[index];
}

double
lefiCorrectionVictim::length()
{
    return length_;
}

// *****************************************************************************
// lefiCorrectionResistance
// *****************************************************************************

lefiCorrectionResistance::lefiCorrectionResistance()
{
    Init();
}

void
lefiCorrectionResistance::Init()
{
    numNums_ = 0;
    numsAllocated_ = 1;
    nums_ = (double*) lefMalloc(sizeof(double) * 1);

    numVictims_ = 0;
    victimsAllocated_ = 2;
    victims_ = (lefiCorrectionVictim**) lefMalloc(sizeof(
                                                  lefiCorrectionVictim*) * 2);
}

void
lefiCorrectionResistance::clear()
{
    int                     i;
    lefiCorrectionVictim    *r;
    int                     max = numVictims_;

    for (i = 0; i < max; i++) {
        r = victims_[i];
        r->Destroy();
        lefFree((char*) r);
    }
    numVictims_ = 0;
    numNums_ = 0;
}

void
lefiCorrectionResistance::Destroy()
{
    clear();
    lefFree((char*) (nums_));
    lefFree((char*) (victims_));
}

lefiCorrectionResistance::~lefiCorrectionResistance()
{
    Destroy();
}

void
lefiCorrectionResistance::addResistanceNumber(double d)
{
    if (numNums_ == numsAllocated_) {
        int     max;
        double  *ne;
        int     i;

        if (numsAllocated_) {
            max = numsAllocated_ = 2;
            numNums_ = 0;
        } else
            max = numsAllocated_ = numNums_ * 2;
        ne = (double*) lefMalloc(sizeof(double) * max);
        max /= 2;
        for (i = 0; i < max; i++)
            ne[i] = nums_[i];
        lefFree((char*) (nums_));
        nums_ = ne;
    }
    nums_[numNums_] = d;
    numNums_ += 1;
}

void
lefiCorrectionResistance::addVictimCorrection(double d)
{
    lefiCorrectionVictim *r = victims_[numVictims_ - 1];
    r->addVictimCorrection(d);
}

void
lefiCorrectionResistance::addVictimLength(double d)
{
    lefiCorrectionVictim *r;
    if (numVictims_ == victimsAllocated_) {
        int                     max;
        lefiCorrectionVictim    **ne;
        int                     i;

        if (victimsAllocated_ == 0) {
            max = victimsAllocated_ = 2;
            numVictims_ = 0;
        } else
            max = victimsAllocated_ = numVictims_ * 2;
        ne = (lefiCorrectionVictim**) lefMalloc(sizeof(lefiCorrectionVictim*) * max);
        max /= 2;
        for (i = 0; i < max; i++)
            ne[i] = victims_[i];
        lefFree((char*) (victims_));
        victims_ = ne;
    }
    r = (lefiCorrectionVictim*) lefMalloc(sizeof(lefiCorrectionVictim));
    r->Init(d);
    victims_[numVictims_] = r;
    numVictims_ += 1;
}

int
lefiCorrectionResistance::numVictims()
{
    return numVictims_;
}

lefiCorrectionVictim *
lefiCorrectionResistance::victim(int index)
{
    return victims_[index];
}

int
lefiCorrectionResistance::numNums()
{
    return numNums_;
}

double
lefiCorrectionResistance::num(int index)
{
    return nums_[index];
}

// *****************************************************************************
// lefiCorrectionEdge
// *****************************************************************************


lefiCorrectionEdge::lefiCorrectionEdge()
{
    Init();
}


void
lefiCorrectionEdge::Init()
{
    edge_ = 0;

    numResistances_ = 0;
    resistancesAllocated_ = 2;
    resistances_ = (lefiCorrectionResistance**) lefMalloc(sizeof(
                                                          lefiCorrectionResistance*) * 2);
}


void
lefiCorrectionEdge::clear()
{
    int                         i;
    lefiCorrectionResistance    *r;
    int                         maxr = numResistances_;

    for (i = 0; i < maxr; i++) {
        r = resistances_[i];
        r->Destroy();
        lefFree((char*) r);
    }

    edge_ = 0;
    numResistances_ = 0;
}


void
lefiCorrectionEdge::Destroy()
{
    clear();
    lefFree((char*) (resistances_));
}


lefiCorrectionEdge::~lefiCorrectionEdge()
{
    Destroy();
}


void
lefiCorrectionEdge::addEdge(double d)
{
    edge_ = d;
}


void
lefiCorrectionEdge::addResistanceNumber(double d)
{
    lefiCorrectionResistance *r = resistances_[numResistances_ - 1];
    r->addResistanceNumber(d);
}


void
lefiCorrectionEdge::addResistance()
{
    lefiCorrectionResistance *r;
    if (numResistances_ == resistancesAllocated_) {
        int                         max;
        lefiCorrectionResistance    **ne;
        int                         i;

        if (resistancesAllocated_ == 0) {
            max = resistancesAllocated_ = 2;
            numResistances_ = 0;
        } else
            max = resistancesAllocated_ = numResistances_ * 2;
        ne = (lefiCorrectionResistance**) lefMalloc
            (sizeof(lefiCorrectionResistance*) * max);
        max /= 2;
        for (i = 0; i < max; i++)
            ne[i] = resistances_[i];
        lefFree((char*) (resistances_));
        resistances_ = ne;
    }
    r = (lefiCorrectionResistance*) lefMalloc(sizeof(lefiCorrectionResistance));
    r->Init();
    resistances_[numResistances_] = r;
    numResistances_ += 1;
}


void
lefiCorrectionEdge::addVictimCorrection(double d)
{
    lefiCorrectionResistance *r = resistances_[numResistances_ - 1];
    r->addVictimCorrection(d);
}


void
lefiCorrectionEdge::addVictimLength(double d)
{
    lefiCorrectionResistance *r = resistances_[numResistances_ - 1];
    r->addVictimLength(d);
}


int
lefiCorrectionEdge::numResistances()
{
    return numResistances_;
}


lefiCorrectionResistance *
lefiCorrectionEdge::resistance(int index)
{
    return resistances_[index];
}


double
lefiCorrectionEdge::edge()
{
    return edge_;
}


// *****************************************************************************
// lefiCorrectionTable
// *****************************************************************************

lefiCorrectionTable::lefiCorrectionTable()
{
    Init();
}

void
lefiCorrectionTable::Init()
{
    numEdges_ = 0;
    edgesAllocated_ = 2;
    edges_ = (lefiCorrectionEdge**) lefMalloc(sizeof(lefiCorrectionEdge*) * 2);
}

void
lefiCorrectionTable::clear()
{
    int                 i;
    lefiCorrectionEdge  *r;
    int                 max = numEdges_;

    for (i = 0; i < max; i++) {
        r = edges_[i];
        r->Destroy();
        lefFree((char*) r);
    }
    numEdges_ = 0;
}

void
lefiCorrectionTable::Destroy()
{
    clear();
    lefFree((char*) (edges_));
}

lefiCorrectionTable::~lefiCorrectionTable()
{
    Destroy();
}

void
lefiCorrectionTable::setup(int i)
{
    num_ = i;
    clear();
}

void
lefiCorrectionTable::newEdge()
{
    lefiCorrectionEdge *r;
    if (numEdges_ == edgesAllocated_) {
        int                 max;
        lefiCorrectionEdge  **ne;
        int                 i;

        if (edgesAllocated_ == 0) {
            max = edgesAllocated_ = 2;
            numEdges_ = 0;
        } else
            max = edgesAllocated_ = numEdges_ * 2;
        ne = (lefiCorrectionEdge**) lefMalloc(sizeof(lefiCorrectionEdge*) * max);
        max /= 2;
        for (i = 0; i < max; i++)
            ne[i] = edges_[i];
        lefFree((char*) (edges_));
        edges_ = ne;
    }
    r = (lefiCorrectionEdge*) lefMalloc(sizeof(lefiCorrectionEdge));
    r->Init();
    edges_[numEdges_] = r;
    numEdges_ += 1;
}

void
lefiCorrectionTable::addEdge(double d)
{
    lefiCorrectionEdge *r = edges_[numEdges_ - 1];
    r->addEdge(d);
}

void
lefiCorrectionTable::addResistanceNumber(double d)
{
    lefiCorrectionEdge *r = edges_[numEdges_ - 1];
    r->addResistanceNumber(d);
}

void
lefiCorrectionTable::addResistance()
{
    lefiCorrectionEdge *r = edges_[numEdges_ - 1];
    r->addResistance();
}

void
lefiCorrectionTable::addVictimLength(double d)
{
    lefiCorrectionEdge *r = edges_[numEdges_ - 1];
    r->addVictimLength(d);
}

void
lefiCorrectionTable::addVictimCorrection(double d)
{
    lefiCorrectionEdge *r = edges_[numEdges_ - 1];
    r->addVictimCorrection(d);
}

int
lefiCorrectionTable::num()
{
    return num_;
}

int
lefiCorrectionTable::numEdges()
{
    return numEdges_;
}

lefiCorrectionEdge *
lefiCorrectionTable::edge(int index)
{
    return edges_[index];
}
END_LEFDEF_PARSER_NAMESPACE

