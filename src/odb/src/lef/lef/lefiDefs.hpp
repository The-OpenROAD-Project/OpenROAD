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

// Definitions header file for the DEF Interface

#ifndef LEFI_DEFS_H
#define LEFI_DEFS_H

#include <climits>
#include <cstdio>

#include "lefiKRDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

//=================== General Types and Definitions =================

struct lefPOINT
{
  double x;
  double y;
};

struct lefRECT
{
  lefPOINT ll, ur;
};

struct lefTOKEN
{
  lefTOKEN* next;
  int what;
  int data;
  lefPOINT pt;
};

//=================== Enumerated Types ============================
using lefiBoolean = int;

// Every type of object has a unique identifier, and each object
//  which is created knows its type, by storing the lefiObjectType_e
//  as the first member in the structure.

enum lefiObjectType_e
{
  // decrease likelihood of accidentally correct values by starting
  // at an unusual number
  lefiInvalidObject = 41713,
  lefiUnknownObject  // void *
};

// The memory policy controls how an object which refers to or is composed of
//  other objects manages those sub-objects, particularly when the parent
//  object is copied or deleted.  The policy is specified as an argument to the
//  constructor or initializer, and it is stored with the parent object.
//
//  The memory policy is a generalization of the common distinction between
//  deep and shallow copies.  When a shallow copy of a parent object is made,
//  the copy maintains pointers to the original sub-objects, and the original
//  parent remains responsible for deleting those sub-objects.  When a deep
//  copy of a parent object is made, the copy maintains pointers to new copies
//  of each of the sub-objects, and the copy is responsible for deleting the
//  new sub-objects.
//
//  The lefiPrivateSubObjects policy corresponds to a deep copy, while the the
//  lefiReferencedSubObjects policy corresponds to a shallow copy.  Usually an
//  initial parent object will be created using lefiPrivateSubObjects.  When a
//  copy is made of that parent object, the copy may either maintain its own
//  private versions of each sub-object, or it may refer to the original
//  sub-objects.
//
//  In certain cases, it is useful to create a deep copy of a parent object,
//  even though the new parent object shouldn't be responsible for the new
//  sub-objects.  In this case, the lefiOrphanSubObjects and
//  lefiAdoptedSubObjects policies may be used.  lefiOrphanSubObjects is
//  specified while creating the deep copy, and then lefiAdoptedSubObjects is
//  specified while creating another parent which will take on the
//  responsibility for the orphans.
//
//  An object's memory policy affects only the sub-objects which it directly
//  controls.  Those sub-objects themselves may have the same memory policy as
//  their parents, or they may have a different memory policy.  When a copy is
//  made of a child sub-object, the memory policy of the child controls
//  whether deep or shallow copies are made of the grandchildren.
//
enum lefiMemoryPolicy_e
{
  // decrease likelihood of accidentally correct values by starting
  // at an unusual number
  lefiInvalidMemoryPolicy = 23950,
  lefiPrivateSubObjects,     // deep copy + delete
  lefiReferencedSubObjects,  // shallow copy, no delete
  lefiOrphanSubObjects,      // deep copy, no delete
  lefiAdoptedSubObjects      // shallow copy + delete
};

// An opaque pointer for passing user data through from one API
//  function to another.
//  A handle which a user can set to point to their own data
//  on a per-callback basis.  (See the comment in lefwWriter.h)

using lefiUserData = void*;
using lefiUserDataHandle = void**;

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
