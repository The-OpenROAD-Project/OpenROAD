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
#include "lefiProp.hpp"
#include "lefiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

lefiProp::lefiProp()
: propType_(NULL),
  propName_(NULL),
  nameSize_(0),
  hasRange_(0),
  hasNumber_(0),
  hasNameMapString_(0),
  dataType_(0),
  stringData_(NULL),
  stringLength_(0),
  left_(0.0),
  right_(0.0),
  d_(0.0)
{
    Init();
}

void
lefiProp::Init()
{
    stringLength_ = 16;
    stringData_ = (char*) lefMalloc(16);
    nameSize_ = 16;
    propName_ = (char*) lefMalloc(16);
    clear();
}

void
lefiProp::Destroy()
{
    lefFree(stringData_);
    lefFree(propName_);
}

lefiProp::~lefiProp()
{
    Destroy();
}

void
lefiProp::setPropType(const char    *typ,
                      const char    *string)
{
    int len;
    propType_ = (char*) typ;
    if ((len = strlen(string) + 1) > nameSize_)
        bumpName(len);
    strcpy(propName_, CASE(string));
}

void
lefiProp::setRange(double   left,
                   double   right)
{
    hasRange_ = 1;
    left_ = left;
    right_ = right;
}

void
lefiProp::setNumber(double d)
{
    hasNumber_ = 1;
    d_ = d;
}

void
lefiProp::setPropInteger()
{
    dataType_ = 'I';
}

void
lefiProp::setPropReal()
{
    dataType_ = 'R';
}

void
lefiProp::setPropString()
{
    dataType_ = 'S';
}

void
lefiProp::setPropQString(const char *string)
{
    int len;
    dataType_ = 'Q';
    if ((len = strlen(string) + 1) > stringLength_)
        bumpSize(len);
    strcpy(stringData_, CASE(string));
}

void
lefiProp::setPropNameMapString(const char *string)
{
    int len;
    dataType_ = 'N';
    hasNameMapString_ = 1;
    if ((len = strlen(string) + 1) > stringLength_)
        bumpSize(len);
    strcpy(stringData_, CASE(string));
}

const char *
lefiProp::string() const
{
    return stringData_;
}

const char *
lefiProp::propType() const
{
    return propType_;
}

int
lefiProp::hasNumber() const
{
    return (int) (hasNumber_);
}

int
lefiProp::hasRange() const
{
    return (int) (hasRange_);
}

double
lefiProp::number() const
{
    return d_;
}

double
lefiProp::left() const
{
    return left_;
}

double
lefiProp::right() const
{
    return right_;
}

void
lefiProp::bumpSize(int size)
{
    lefFree(stringData_);
    stringData_ = (char*) lefMalloc(size);
    stringLength_ = size;
    *(stringData_) = '\0';
}

void
lefiProp::bumpName(int size)
{
    lefFree(propName_);
    propName_ = (char*) lefMalloc(size);
    nameSize_ = size;
    *(propName_) = '\0';
}

void
lefiProp::clear()
{
    if (stringData_)
        *(stringData_) = '\0';
    if (stringData_)
        *(propName_) = '\0';
    propType_ = 0;
    hasRange_ = 0;
    hasNumber_ = 0;
    hasNameMapString_ = 0;
    dataType_ = 'B'; // bogus 
    d_ = left_ = right_ = 0.0;
}

int
lefiProp::hasString() const
{
    return *(stringData_) ? 1 : 0;
}

int
lefiProp::hasNameMapString() const
{
    return (hasNameMapString_) ? 1 : 0;
}

const char *
lefiProp::propName() const
{
    return (propName_);
}

char
lefiProp::dataType() const
{
    return (dataType_);
}

void
lefiProp::print(FILE *f) const
{
    fprintf(f, "Prop type '%s'\n", propType());
    if (hasString()) {
        fprintf(f, "  string '%s'\n", string());
    }
    if (hasNumber()) {
        fprintf(f, "  number %5.2f\n", number());
    }
    if (hasRange()) {
        fprintf(f, "  range %5.2f - %5.2f\n",
                left(), right());
    }
}

END_LEFDEF_PARSER_NAMESPACE

