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


#ifndef CDEFISITE_H
#define CDEFISITE_H

#include <stdio.h>
#include "defiTypedefs.h"

/*
 * Struct holds the data for one site.
 * It is also used for a canplace and cannotoccupy.
 */

EXTERN double defiSite_x_num (const defiSite* obj);
EXTERN double defiSite_y_num (const defiSite* obj);
EXTERN double defiSite_x_step (const defiSite* obj);
EXTERN double defiSite_y_step (const defiSite* obj);
EXTERN double defiSite_x_orig (const defiSite* obj);
EXTERN double defiSite_y_orig (const defiSite* obj);
EXTERN int defiSite_orient (const defiSite* obj);
EXTERN const char* defiSite_orientStr (const defiSite* obj);
EXTERN const char* defiSite_name (const defiSite* obj);

EXTERN void defiSite_print (const defiSite* obj, FILE*  f);

/* Struct holds the data for a Box */
  /* Use the default destructor and constructor.                              */
  /* 5.6 changed to use it own constructor & destructor                       */

  /* NOTE: 5.6                                                                */
  /* The following methods are still here for backward compatibility          */
  /* For new reader they should use numPoints & getPoint to get the           */
  /* data.                                                                    */
EXTERN int defiBox_xl (const defiBox* obj);
EXTERN int defiBox_yl (const defiBox* obj);
EXTERN int defiBox_xh (const defiBox* obj);
EXTERN int defiBox_yh (const defiBox* obj);

EXTERN struct defiPoints defiBox_getPoint (const defiBox* obj);

EXTERN void defiBox_print (const defiBox* obj, FILE*  f);

#endif
