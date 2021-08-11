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

#ifndef lefiCrossTalk_h
#define lefiCrossTalk_h

#include <stdio.h>
#include "lefiKRDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

// Structure returned for the noise margin callback.
// This lef construct has two floating point numbers. 
struct lefiNoiseMargin {
  double high;
  double low;
};

class lefiNoiseVictim {
public:
  lefiNoiseVictim(double d);
  void Init(double d);

  void Destroy();
  ~lefiNoiseVictim();

  void clear();

  void addVictimNoise(double d);

  double length() const;
  int numNoises() const;
  double noise(int index) const;

protected:
  double length_;

  int numNoises_;
  int noisesAllocated_;
  double* noises_;
};

class lefiNoiseResistance {
public:
  lefiNoiseResistance();
  void Init();

  void Destroy();
  ~lefiNoiseResistance();

  void clear();

  void addResistanceNumber(double d);
  void addVictimLength(double d);
  void addVictimNoise(double d);

  int numNums() const;
  double num(int index) const;

  int numVictims() const;
  lefiNoiseVictim* victim(int index) const;

protected:
  int numNums_;
  int numsAllocated_;
  double* nums_;

  int numVictims_;
  int victimsAllocated_;
  lefiNoiseVictim** victims_;
};

class lefiNoiseEdge {
public:
  lefiNoiseEdge();
  void Init();

  void Destroy();
  ~lefiNoiseEdge();

  void clear();

  void addEdge(double d);
  void addResistance();
  void addResistanceNumber(double d);
  void addVictimLength(double d);
  void addVictimNoise(double d);

  double edge();
  int numResistances();
  lefiNoiseResistance* resistance(int index);

protected:
  double edge_;

  int numResistances_;
  int resistancesAllocated_;
  lefiNoiseResistance** resistances_;
};

class lefiNoiseTable {
public:
  lefiNoiseTable();
  void Init();

  void Destroy();
  ~lefiNoiseTable();

  void setup(int i);
  void newEdge();
  void addEdge(double d);
  void addResistance();
  void addResistanceNumber(double d);
  void addVictimLength(double d);
  void addVictimNoise(double d);

  void clear();

  int num();
  int numEdges();
  lefiNoiseEdge* edge(int index);

protected:
  int num_;

  int numEdges_;
  int edgesAllocated_;
  lefiNoiseEdge** edges_;
};

class lefiCorrectionVictim {
public:
  lefiCorrectionVictim(double d);
  void Init(double d);

  void Destroy();
  ~lefiCorrectionVictim();

  void clear();

  void addVictimCorrection(double d);

  double length();
  int numCorrections();
  double correction(int index);

protected:
  double length_;

  int numCorrections_;
  int correctionsAllocated_;
  double* corrections_;
};

class lefiCorrectionResistance {
public:
  lefiCorrectionResistance();
  void Init();

  void Destroy();
  ~lefiCorrectionResistance();

  void clear();

  void addResistanceNumber(double d);
  void addVictimLength(double d);
  void addVictimCorrection(double d);

  int numNums();
  double num(int index);

  int numVictims();
  lefiCorrectionVictim* victim(int index);

protected:
  int numNums_;
  int numsAllocated_;
  double* nums_;

  int numVictims_;
  int victimsAllocated_;
  lefiCorrectionVictim** victims_;
};

class lefiCorrectionEdge {
public:
  lefiCorrectionEdge();
  void Init();
 
  void Destroy();
  ~lefiCorrectionEdge();
 
  void clear();
 
  void addEdge(double d);
  void addResistance();
  void addResistanceNumber(double d);
  void addVictimLength(double d);
  void addVictimCorrection(double d);
 
  double edge();
  int numResistances();
  lefiCorrectionResistance* resistance(int index);
 
protected:
  double edge_;
 
  int numResistances_;
  int resistancesAllocated_;
  lefiCorrectionResistance** resistances_;
};

class lefiCorrectionTable {
public:
  lefiCorrectionTable();
  void Init();

  void Destroy();
  ~lefiCorrectionTable();

  void setup(int i);
  void newEdge();
  void addEdge(double d);
  void addResistance();
  void addResistanceNumber(double d);
  void addVictimLength(double d);
  void addVictimCorrection(double d);

  void clear();

  int num();
  int numEdges();
  lefiCorrectionEdge* edge(int index);

protected:
  int num_;

  int numEdges_;
  int edgesAllocated_;
  lefiCorrectionEdge** edges_;
};

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
