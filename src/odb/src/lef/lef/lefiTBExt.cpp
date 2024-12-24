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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "lefiKRDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

// Convert the orient from integer to string
//
// *****************************************************************************
char* lefiOrientStr(int orient)
{
  switch (orient) {
    case 0:
      return ((char*) "N");
    case 1:
      return ((char*) "W");
    case 2:
      return ((char*) "S");
    case 3:
      return ((char*) "E");
    case 4:
      return ((char*) "FN");
    case 5:
      return ((char*) "FW");
    case 6:
      return ((char*) "FS");
    case 7:
      return ((char*) "FE");
  };
  return ((char*) "");
}

END_LEFDEF_PARSER_NAMESPACE
