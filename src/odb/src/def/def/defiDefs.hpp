// *****************************************************************************
// *****************************************************************************
// Copyright 2013, Cadence Design Systems
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

/* Definitions header file for the DEF Interface */

#ifndef DEFI_DEFS_H
#define DEFI_DEFS_H

#include <stdio.h>
#include <limits.h>

#include "defiKRDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

/*=================== General Types and Definitions =================*/

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef NULL
#define NULL (0)
#endif

typedef struct defpoint defPOINT;

struct defpoint {
    int x;
    int y;
    };

typedef struct defrect defRECT;

struct defrect {
    defPOINT ll,ur;
    };

typedef struct deftoken defTOKEN;

struct deftoken {
    defTOKEN *next;
    int what;
    int data;
    defPOINT pt;
    };

#define START_LIST 10001
#define POINT_SPEC 10002
#define VIA_SPEC   10003
#define WIDTH_SPEC 10004
#define LAYER_SPEC 10005
#define SHAPE_SPEC 10006

#ifndef	MIN
#define MIN(x,y) ((x) < (y)? (x) : (y))
#endif

#ifndef	MIN
#define MAX(x,y) ((x) > (y)? (x) : (y))
#endif

#define ROUND(x) ((x) >= 0 ? (int)((x)+0.5) : (int)((x)-0.5))

//defTOKEN *TokenFromRect();

/*=================== Enumerated Types ============================*/
typedef int defiBoolean;

/* Every type of object has a unique identifier, and each object
 * which is created knows its type, by storing the defiObjectType_e
 * as the first member in the structure.
 *
 */
typedef enum
{
  /* decrease likelihood of accidentally correct values by starting
     at an unusual number */
  defiInvalidObject = 41713, 
  defiUnknownObject /* void * */
} defiObjectType_e;


/* The memory policy controls how an object which refers to or is composed of
 * other objects manages those sub-objects, particularly when the parent
 * object is copied or deleted.  The policy is specified as an argument to the
 * constructor or initializer, and it is stored with the parent object.
 *
 * The memory policy is a generalization of the common distinction between
 * deep and shallow copies.  When a shallow copy of a parent object is made,
 * the copy maintains pointers to the original sub-objects, and the original
 * parent remains responsible for deleting those sub-objects.  When a deep
 * copy of a parent object is made, the copy maintains pointers to new copies
 * of each of the sub-objects, and the copy is responsible for deleting the
 * new sub-objects.
 *
 * The defiPrivateSubObjects policy corresponds to a deep copy, while the the
 * defiReferencedSubObjects policy corresponds to a shallow copy.  Usually an
 * initial parent object will be created using defiPrivateSubObjects.  When a
 * copy is made of that parent object, the copy may either maintain its own
 * private versions of each sub-object, or it may refer to the original
 * sub-objects.
 *
 * In certain cases, it is useful to create a deep copy of a parent object,
 * even though the new parent object shouldn't be responsible for the new
 * sub-objects.  In this case, the defiOrphanSubObjects and
 * defiAdoptedSubObjects policies may be used.  defiOrphanSubObjects is
 * specified while creating the deep copy, and then defiAdoptedSubObjects is
 * specified while creating another parent which will take on the
 * responsibility for the orphans.
 *
 * An object's memory policy affects only the sub-objects which it directly
 * controls.  Those sub-objects themselves may have the same memory policy as
 * their parents, or they may have a different memory policy.  When a copy is
 * made of a child sub-object, the memory policy of the child controls
 * whether deep or shallow copies are made of the grandchildren.
 */
typedef enum
{
  /* decrease likelihood of accidentally correct values by starting
     at an unusual number */
  defiInvalidMemoryPolicy = 23950,
  defiPrivateSubObjects,      // deep copy + delete
  defiReferencedSubObjects,   // shallow copy, no delete
  defiOrphanSubObjects,       // deep copy, no delete
  defiAdoptedSubObjects       // shallow copy + delete
} defiMemoryPolicy_e;

/* An opaque pointer for passing user data through from one API
 * function to another.
 * A handle which a user can set to point to their own data
 * on a per-callback basis.  (See the comment in defwWriter.h)
 */
#define defiUserData void *
#define defiUserDataHandle void **

/* On SunOs 4.1.3 with acc, this is in libansi.a, but isn't properly
 * declared anywhere in the header files supplied with the compiler.
 */
#ifdef __SunOS_4_1_3
extern int strcasecmp(const char*, const char*);
#endif

#ifdef WIN32
#define strdup _strdup
#endif 

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
