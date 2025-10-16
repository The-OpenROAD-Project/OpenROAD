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

#ifndef lefiUnits_h
#define lefiUnits_h

#include <cstdio>

#include "lefiKRDefs.hpp"

BEGIN_LEF_PARSER_NAMESPACE

class lefiUnits
{
 public:
  lefiUnits();
  void Init();

  void Destroy();
  ~lefiUnits();

  void setDatabase(const char* name, double num);
  void clear();
  void setTime(double num);
  void setCapacitance(double num);
  void setResistance(double num);
  void setPower(double num);
  void setCurrent(double num);
  void setVoltage(double num);
  void setFrequency(double num);

  int hasDatabase() const;
  int hasCapacitance() const;
  int hasResistance() const;
  int hasTime() const;
  int hasPower() const;
  int hasCurrent() const;
  int hasVoltage() const;
  int hasFrequency() const;

  const char* databaseName() const;
  double databaseNumber() const;
  double capacitance() const;
  double resistance() const;
  double time() const;
  double power() const;
  double current() const;
  double voltage() const;
  double frequency() const;

  // Debug print
  void print(FILE* f) const;

 protected:
  int hasDatabase_{0};
  int hasCapacitance_{0};
  int hasResistance_{0};
  int hasTime_{0};
  int hasPower_{0};
  int hasCurrent_{0};
  int hasVoltage_{0};
  int hasFrequency_{0};
  char* databaseName_{nullptr};
  double databaseNumber_{0.0};
  double capacitance_{0.0};
  double resistance_{0.0};
  double power_{0.0};
  double time_{0.0};
  double current_{0.0};
  double voltage_{0.0};
  double frequency_{0.0};
};

END_LEF_PARSER_NAMESPACE

#endif
