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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#   include <time.h>
#else /* not WIN32 */
#   include <sys/time.h>
#endif
#if ibmrs
#include <time.h>
#endif


#include "defiKRDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

/* Not needed
time_t defiCalcTime() {
    * Calculate the number for the given date
    * The date is 5/1/99

    * Used to calculate the UTC for a time bomb date in libcai.a
    * see caiInitWork() function
    *
    struct tm	ts;

    ts.tm_sec = 0;
    ts.tm_min = 0;
    ts.tm_hour = 0;
    ts.tm_mday = 1;
    ts.tm_mon = 5;
    ts.tm_year = 1999 - 1900;
    ts.tm_wday = 0;
    ts.tm_yday = 0;
    ts.tm_isdst = 0;

*
    printf("May 1, 1999 in UTC is %d\n", mktime(&ts));
    ts.tm_mday = 2;
    printf("May 2, 1999 in UTC is %d\n", mktime(&ts));

    printf("Right now is %d\n", time(0));
*
    return(mktime(&ts));
}
*/

////////////////////////////////////////////////////
//
//    defiTimeBomb
//
////////////////////////////////////////////////////

// Check the current date against the date given
/*
int defiValidTime() {
    time_t	bombTime = defiCalcTime();
    time_t	curTime;

    curTime = time((time_t *)NULL);
    if (curTime == -1 || curTime > bombTime)
    {
	(void)printf("The demonstration version of this code is no longer\n"
		     "available.  Please contact your Lef/Def Parser\n"
		     "software provider for up to date code.\n");
	return(0);
    }
    return (1);
}
*/

////////////////////////////////////////////////////
//
//    Convert the orient from integer to string
//
////////////////////////////////////////////////////
const char* defiOrientStr(int orient) {
  switch (orient) {
      case 0: return ((char*)"N");
      case 1: return ((char*)"W");
      case 2: return ((char*)"S");
      case 3: return ((char*)"E");
      case 4: return ((char*)"FN");
      case 5: return ((char*)"FW");
      case 6: return ((char*)"FS");
      case 7: return ((char*)"FE");
  };
  return ((char*)"");
}

END_LEFDEF_PARSER_NAMESPACE
