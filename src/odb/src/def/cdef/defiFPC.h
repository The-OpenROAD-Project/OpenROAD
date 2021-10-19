/* ************************************************************************** */
/* ************************************************************************** */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               */
/* ************************************************************************** */
/* ************************************************************************** */
/* Copyright 2013, Cadence Design Systems                                     */
/*                                                                            */
/* This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source             */
/* Distribution,  Product Version 5.8.                                        */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License");            */
/*    you may not use this file except in compliance with the License.        */
/*    You may obtain a copy of the License at                                 */
/*                                                                            */
/*        http://www.apache.org/licenses/LICENSE-2.0                          */
/*                                                                            */
/*    Unless required by applicable law or agreed to in writing, software     */
/*    distributed under the License is distributed on an "AS IS" BASIS,       */
/*    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or         */
/*    implied. See the License for the specific language governing            */
/*    permissions and limitations under the License.                          */
/*                                                                            */
/* For updates, support, or to become part of the LEF/DEF Community,          */
/* check www.openeda.org for details.                                         */
/*                                                                            */
/*  $Author: dell $                                                                  */
/*  $Revision: #1 $                                                                */
/*  $Date: 2017/06/06 $                                                                    */
/*  $State:  $                                                                */
/* ************************************************************************** */
/* ************************************************************************** */


#ifndef CDEFIFPC_H
#define CDEFIFPC_H

#include <stdio.h>
#include "defiTypedefs.h"

EXTERN const char* defiFPC_name (const defiFPC* obj);
EXTERN int defiFPC_isVertical (const defiFPC* obj);
EXTERN int defiFPC_isHorizontal (const defiFPC* obj);
EXTERN int defiFPC_hasAlign (const defiFPC* obj);
EXTERN int defiFPC_hasMax (const defiFPC* obj);
EXTERN int defiFPC_hasMin (const defiFPC* obj);
EXTERN int defiFPC_hasEqual (const defiFPC* obj);
EXTERN double defiFPC_alignMax (const defiFPC* obj);
EXTERN double defiFPC_alignMin (const defiFPC* obj);
EXTERN double defiFPC_equal (const defiFPC* obj);

EXTERN int defiFPC_numParts (const defiFPC* obj);

  /* Return the constraint number "index" where index is                      */
  /*    from 0 to numParts()                                                  */
  /* The returned corner is 'B' for bottom left  'T' for topright             */
  /* The returned typ is 'R' for rows   'C' for comps                         */
  /* The returned char* points to name of the item.                           */
EXTERN void defiFPC_getPart (const defiFPC* obj, int  index, int*  corner, int*  typ, char**  name);

  /* debug print                                                              */
EXTERN void defiFPC_print (const defiFPC* obj, FILE*  f);

#endif
