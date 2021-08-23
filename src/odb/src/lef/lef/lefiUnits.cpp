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

#include <string.h>
#include <stdlib.h>
#include "lex.h"
#include "lefiUnits.hpp"
#include "lefiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

// *****************************************************************************
// lefiUnits
// *****************************************************************************

lefiUnits::lefiUnits()
: hasDatabase_(0),
  hasCapacitance_(0),
  hasResistance_(0),
  hasTime_(0),
  hasPower_(0),
  hasCurrent_(0),
  hasVoltage_(0),
  hasFrequency_(0),
  databaseName_(NULL),
  databaseNumber_(0.0),
  capacitance_(0.0),
  resistance_(0.0),
  power_(0.0),
  time_(0.0),
  current_(0.0),
  voltage_(0.0),
  frequency_(0.0)
{
    Init();
}

void
lefiUnits::Init()
{
    clear();
}

LEF_COPY_CONSTRUCTOR_C( lefiUnits ) {
    LEF_COPY_FUNC( hasDatabase_ );
    LEF_COPY_FUNC( hasCapacitance_ );
    LEF_COPY_FUNC( hasResistance_ );
    LEF_COPY_FUNC( hasTime_ );
    LEF_COPY_FUNC( hasPower_ );
    LEF_COPY_FUNC( hasCurrent_ );
    LEF_COPY_FUNC( hasVoltage_ );
    LEF_COPY_FUNC( hasFrequency_ );

    LEF_MALLOC_FUNC( databaseName_, char, sizeof(char) * (strlen(prev.databaseName_) +1));
    LEF_COPY_FUNC( databaseNumber_ );
    LEF_COPY_FUNC( capacitance_ );
    LEF_COPY_FUNC( resistance_ );
    LEF_COPY_FUNC( power_ );
    LEF_COPY_FUNC( time_ );
    LEF_COPY_FUNC( current_ );
    LEF_COPY_FUNC( voltage_ );
    LEF_COPY_FUNC( frequency_ );

}

LEF_ASSIGN_OPERATOR_C( lefiUnits ) {
    CHECK_SELF_ASSIGN
    LEF_COPY_FUNC( hasDatabase_ );
    LEF_COPY_FUNC( hasCapacitance_ );
    LEF_COPY_FUNC( hasResistance_ );
    LEF_COPY_FUNC( hasTime_ );
    LEF_COPY_FUNC( hasPower_ );
    LEF_COPY_FUNC( hasCurrent_ );
    LEF_COPY_FUNC( hasVoltage_ );
    LEF_COPY_FUNC( hasFrequency_ );

    LEF_MALLOC_FUNC( databaseName_, char, sizeof(char) * (strlen(prev.databaseName_) +1));
    LEF_COPY_FUNC( databaseNumber_ );
    LEF_COPY_FUNC( capacitance_ );
    LEF_COPY_FUNC( resistance_ );
    LEF_COPY_FUNC( power_ );
    LEF_COPY_FUNC( time_ );
    LEF_COPY_FUNC( current_ );
    LEF_COPY_FUNC( voltage_ );
    LEF_COPY_FUNC( frequency_ );
    return *this;
}

void
lefiUnits::Destroy()
{
    clear();
}

lefiUnits::~lefiUnits()
{
    Destroy();
}

void
lefiUnits::setDatabase(const char   *name,
                       double       num)
{
    int len = strlen(name) + 1;
    databaseName_ = (char*) lefMalloc(len);
    strcpy(databaseName_, CASE(name));
    databaseNumber_ = num;
    hasDatabase_ = 1;
}

void
lefiUnits::clear()
{
    if (databaseName_)
        lefFree(databaseName_);
    hasTime_ = 0;
    hasCapacitance_ = 0;
    hasResistance_ = 0;
    hasPower_ = 0;
    hasCurrent_ = 0;
    hasVoltage_ = 0;
    hasDatabase_ = 0;
    hasFrequency_ = 0;
    databaseName_ = 0;
}

void
lefiUnits::setTime(double num)
{
    hasTime_ = 1;
    time_ = num;
}

void
lefiUnits::setCapacitance(double num)
{
    hasCapacitance_ = 1;
    capacitance_ = num;
}

void
lefiUnits::setResistance(double num)
{
    hasResistance_ = 1;
    resistance_ = num;
}

void
lefiUnits::setPower(double num)
{
    hasPower_ = 1;
    power_ = num;
}

void
lefiUnits::setCurrent(double num)
{
    hasCurrent_ = 1;
    current_ = num;
}

void
lefiUnits::setVoltage(double num)
{
    hasVoltage_ = 1;
    voltage_ = num;
}

void
lefiUnits::setFrequency(double num)
{
    hasFrequency_ = 1;
    frequency_ = num;
}

int
lefiUnits::hasDatabase() const
{
    return hasDatabase_;
}

int
lefiUnits::hasCapacitance() const
{
    return hasCapacitance_;
}

int
lefiUnits::hasResistance() const
{
    return hasResistance_;
}

int
lefiUnits::hasPower() const
{
    return hasPower_;
}

int
lefiUnits::hasCurrent() const
{
    return hasCurrent_;
}

int
lefiUnits::hasVoltage() const
{
    return hasVoltage_;
}

int
lefiUnits::hasFrequency() const
{
    return hasFrequency_;
}

int
lefiUnits::hasTime() const
{
    return hasTime_;
}

const char *
lefiUnits::databaseName() const
{
    return databaseName_;
}

double
lefiUnits::databaseNumber() const
{
    return databaseNumber_;
}

double
lefiUnits::capacitance() const
{
    return capacitance_;
}

double
lefiUnits::resistance() const
{
    return resistance_;
}

double
lefiUnits::power() const
{
    return power_;
}

double
lefiUnits::current() const
{
    return current_;
}

double
lefiUnits::time() const
{
    return time_;
}

double
lefiUnits::voltage() const
{
    return voltage_;
}

double
lefiUnits::frequency() const
{
    return frequency_;
}

void
lefiUnits::print(FILE *f) const
{
    fprintf(f, "Units:\n");
    if (hasTime())
        fprintf(f, "  %g nanoseconds\n", time());
    if (hasCapacitance())
        fprintf(f, "  %g picofarads\n", capacitance());
    if (hasResistance())
        fprintf(f, "  %g ohms\n", resistance());
    if (hasPower())
        fprintf(f, "  %g milliwatts\n", power());
    if (hasCurrent())
        fprintf(f, "  %g milliamps\n", current());
    if (hasVoltage())
        fprintf(f, "  %g volts\n", voltage());
    if (hasFrequency())
        fprintf(f, "  %g frequency\n", frequency());
    if (hasDatabase())
        fprintf(f, "  %s %g\n", databaseName(),
                databaseNumber());
}

END_LEFDEF_PARSER_NAMESPACE

