/*
 * This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
 * Distribution,  Product Version 5.7, and is subject to the Cadence LEF/DEF
 * Open Source License Agreement.   Your  continued  use  of this file
 * constitutes your acceptance of the terms of the LEF/DEF Open Source
 * License and an agreement to abide by its  terms.   If you  don't  agree
 * with  this, you must remove this and any other files which are part of the
 * distribution and  destroy any  copies made.
 * 
 * For updates, support, or to become part of the LEF/DEF Community, check
 * www.openeda.org for details.
 */
#include <string.h>

/*
 * lefiTimeBomb
 *//* Internal time bomb.  Always return date current *//* Check the current
    date against the date given */ int 
lefiValidTime()
{
  return (1);
}

/*
 * Internal check, always return ok
 *//* Check the current date against the date given */ int 
lefiValidUser()
{
  return (1);
}

/*
 * Internal, return Cadence Design Systems
 *//* Check the current date against the date given */ char *
lefiUser()
{
  return ((char *) "Cadence Design Systems");
}
