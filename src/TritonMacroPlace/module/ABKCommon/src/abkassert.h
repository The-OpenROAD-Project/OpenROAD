/**************************************************************************
***    
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2010 Regents of the University of Michigan,
***               Saurabh N. Adya, Jarrod A. Roy, David A. Papa and
***               Igor L. Markov
***
***  Contact author(s): abk@cs.ucsd.edu, imarkov@umich.edu
***  Original Affiliation:   UCLA, Computer Science Department,
***                          Los Angeles, CA 90095-1596 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining 
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation 
***  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
***  and/or sell copies of the Software, and to permit persons to whom the 
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/


//! author="Igor Markov" 

//! CONTACTS="Igor Max Vidhani ABK"

/* June 17, 1997  imarkov   added abkguess()
 June 17, 1997  imarkov   added abkassert_stop_here(), abkguess_stop_here()
 970824   ilm   added std::endl before assert/fatal/warn/guess messages
 970923   ilm   added abk_dump_stack()
 971214   mro   made abkassert() and abkfatal() take care of seeds.out
                  note: abkrand.h is included at *end* of file, because
                  it's needed to make sure SeedCleaner is defined -- but
                  SeedCleaner is used in macros, so not needed until expanded.
                  Putting abkrand.h at beginning of file can cause errors
                  due to cyclic includes.
 980303   ilm   added abk_dump_stack()

 This file to be included into all projects in the group
*/

#ifndef  _ABKASSERT_H_
#define  _ABKASSERT_H_

#include <iostream>
#include "abkseed.h"

//:  name abkassert_stop_here(); abkguess_stop_here(); ....
//   Breakpoints for abkassert, abkguess, abkfatal, abkwarn
//   When  debugging, you can put breakpoints in abkassert_stop_here(),
//   but this should be redundant since abkassert() is always fatal
extern void abkassert_stop_here();
extern void  abkguess_stop_here();

extern void  abkfatal_stop_here();
extern void   abkwarn_stop_here();

//: defined in platfDepend.cxx 
extern void   abk_dump_stack();    
//: defined in platfDepend.cxx 
extern void   abk_call_debugger();  

#ifdef ABKDEBUG
#define STACK_DUMP abk_dump_stack();
#define abkfatal_breakpoint abkfatal_stop_here();
#define abkwarn_breakpoint abkwarn_stop_here();
#else
#define STACK_DUMP static_cast<void>(0); /*do nothing, but do not produce a warning*/
#define abkfatal_breakpoint static_cast<void>(0); /*do nothing, but do not produce a warning*/
#define abkwarn_breakpoint static_cast<void>(0); /*do nothing, but do not produce a warning*/
#endif

const char* SgnPartOfFileName(const char * fileName);

char* BaseFileName(char *filename);

#ifdef ABKDEBUG
//: if ABKDEBUG is defined it checks the condition and
// if it fails, prints the message. If NDEBUG is not
// defined, it prints line number and filename, then stops.
// If NDEBUG is defined, it continues (standard ssert.h behavior)
// To have ABKDEBUG defined, your compiler should use -DDEBUG argument
// Otherwise, there is 0 overhead to have abkassert()
#define abkassert(CONDITION, ERRMESSAGE)                                \
        {  if (!(CONDITION))                                            \
       {  std::cerr << std::endl << (ERRMESSAGE) << std::endl;                         \
          std::cerr << "  (Error in " << SgnPartOfFileName(__FILE__)         \
                                       << ":" << __LINE__  <<")"<<std::endl; \
          std::cerr <<"The random seed for this run was "                    \
               << SeedHandler::getExternalSeed() << std::endl;               \
          {SeedCleaner clean_err_exit;}                                 \
          abk_dump_stack();                                             \
          abkassert_stop_here();                                        \
          abort();                         /* Armageddon now */         \
       }                                                                \
    }                               
#else
#define abkassert(CONDITION, ERRMESSAGE) ((void) 0)
#endif

//        char * __p=(char *) 0;__p[0]=0;  /* Armageddon now */         

#ifdef ABKDEBUG
#define abkassert2(CONDITION, ERR1, ERR2)                               \
        {  if (!(CONDITION))                                            \
       {  std::cerr << std::endl << (ERR1) << (ERR2) << std::endl;                     \
          std::cerr << "  (Error in " << SgnPartOfFileName(__FILE__)         \
                                       << ":" << __LINE__  <<")"<<std::endl; \
          std::cerr <<"The random seed for this run was "                    \
               << SeedHandler::getExternalSeed() << std::endl;               \
          {SeedCleaner clean_err_exit;}                                 \
          abk_dump_stack();                                             \
          abkassert_stop_here();                                        \
          abort(); char * __p=(char *) 0;__p[0]=0;                      \
       }                                                                \
    }                               
#else
#define abkassert2(CONDITION, ERR1, ERR2) ((void) 0)
#endif

#ifdef ABKDEBUG
#define abkassert3(CONDITION, ERR1, ERR2, ERR3)                         \
        {  if (!(CONDITION))                                            \
       {  std::cerr << std::endl << (ERR1) << (ERR2) <<(ERR3) << std::endl;            \
          std::cerr << "  (Error in " << SgnPartOfFileName(__FILE__)         \
                                       << ":" << __LINE__  <<")"<<std::endl; \
          std::cerr <<"The random seed for this run was "                    \
               << SeedHandler::getExternalSeed() << std::endl;               \
          {SeedCleaner clean_err_exit;}                                 \
          abk_dump_stack();                                             \
          abkassert_stop_here();                                        \
          abort();char * __p=(char *) 0;__p[0]=0; /* Armageddon now */  \
       }                                                                \
    }                               
#else
#define abkassert3(CONDITION, ERR1, ERR2, ERR3) ((void) 0)
#endif

#ifdef ABKDEBUG
//: Same as abkassert, but does not stop the program.
#define abkguess(CONDITION, ERRMESSAGE)                                 \
        {  if (!(CONDITION))                                            \
       {  std::cerr << std::endl << (ERRMESSAGE);                                 \
          std::cerr << " in line "<<__LINE__<<", file "<<__FILE__<<std::endl;     \
          abkguess_stop_here();                                         \
       }                                                                \
    }                               
#else
#define abkguess(CONDITION, ERRMESSAGE) ((void) 0)
#endif

#ifdef ABKDEBUG
#define abkguess2(CONDITION, ERR1,ERR2 )                                \
        {  if (!(CONDITION))                                            \
       {  std::cerr << std::endl << (ERR1) << (ERR2);                             \
          std::cerr << " in line "<<__LINE__<<", file "<<__FILE__<<std::endl;     \
          abkguess_stop_here();                                         \
       }                                                                \
    }                               
#else
#define abkguess2(CONDITION, ERR1, ERR2) ((void) 0)
#endif

#ifdef ABKDEBUG
#define abkguess3(CONDITION,ERR1,ERR2,ERR3)                             \
        {  if (!(CONDITION))                                            \
       {  std::cerr << std::endl << (ERR1) << (ERR2) << (ERR3);                   \
          std::cerr << " in line "<<__LINE__<<", file "<<__FILE__<<std::endl;     \
          abkguess_stop_here();                                         \
       }                                                                \
    }                               
#else
#define abkguess3(CONDITION, ERR1, ERR2, ERR3) ((void) 0)
#endif

//: just like abkassert(), but does not depend on  whether
// a symbol is defined. "unconditional assertion"
// should be used in all noncritical places
#define abkfatal(CONDITION, ERRMESSAGE)                                 \
        {  if (!(CONDITION))                                            \
       {  std::cerr << std::endl << (ERRMESSAGE) ;                                \
          std::cerr <<"  (Fatal error in " << SgnPartOfFileName(__FILE__)    \
                                       << ":" << __LINE__  <<")"<<std::endl; \
          std::cerr <<"The random seed for this run was "                    \
               << SeedHandler::getExternalSeed() << std::endl;               \
          {SeedCleaner clean_err_exit;}                                 \
          STACK_DUMP;                                                   \
          abkfatal_breakpoint;                                          \
          abort(); char * __p=(char *) 0;__p[0]=0;  /* Armageddon now */\
       }                                                                \
    }                               

#define abkfatal2(CONDITION, ERR1, ERR2)                                \
        {  if (!(CONDITION))                                            \
       {  std::cerr << std::endl << (ERR1) << (ERR2);                             \
          std::cerr<<"  (Fatal error in " << SgnPartOfFileName(__FILE__)     \
                                       << ":" << __LINE__  <<")"<<std::endl; \
          std::cerr <<"The random seed for this run was "                    \
               << SeedHandler::getExternalSeed() << std::endl;               \
          {SeedCleaner clean_err_exit;}                                 \
          STACK_DUMP;                                                   \
          abkfatal_breakpoint;                                          \
          abort(); char * __p=(char *) 0;__p[0]=0;  /* Armageddon now */\
       }                                                                \
    }                               

#define abkfatal3(CONDITION, ERR1, ERR2, ERR3)                          \
        {  if (!(CONDITION))                                            \
       {  std::cerr << std::endl << (ERR1) << (ERR2) << (ERR3);                   \
          std::cerr<<"  (Fatal error in " << SgnPartOfFileName(__FILE__)     \
                                       << ":" << __LINE__  <<")"<<std::endl; \
          std::cerr <<"The random seed for this run was "                    \
               << SeedHandler::getExternalSeed() << std::endl;               \
          {SeedCleaner clean_err_exit;}                                 \
          STACK_DUMP;                                                   \
          abkfatal_breakpoint;                                          \
          abort(); char * __p=(char *) 0;__p[0]=0;  /* Armageddon now */\
       }                                                                \
    }                               

//: just like abkguess() 3d, but does not depend whether
// a symbol is defined. "unconditional warning"
// should be used in all noncritical places
#define abkwarn(CONDITION, ERRMESSAGE)                                  \
        {  if (!(CONDITION))                                            \
       {  std::cerr << std::endl << (ERRMESSAGE);                                 \
          std::cerr << " (Warning in " << SgnPartOfFileName(__FILE__)        \
                                     << ":" << __LINE__ <<")"<<std::endl;    \
          abkwarn_breakpoint;                                           \
       }                                                                \
    }
                               
#define abkwarn2(CONDITION, ERR1, ERR2)                                 \
        {  if (!(CONDITION))                                            \
       {  std::cerr << std::endl << (ERR1) << (ERR2);                             \
          std::cerr << " (Warning in " << SgnPartOfFileName(__FILE__)        \
                                     << ":" << __LINE__ <<")"<<std::endl;    \
          abkwarn_breakpoint;                                           \
       }                                                                \
    }

#define abkwarn3(CONDITION, ERR1, ERR2, ERR3)                           \
        {  if (!(CONDITION))                                            \
       {  std::cerr << std::endl << (ERR1) << (ERR2) << (ERR3);                   \
          std::cerr << " (Warning in " << SgnPartOfFileName(__FILE__)        \
                                     << ":" << __LINE__ <<")"<<std::endl;    \
          abkwarn_breakpoint;                                           \
       }                                                                \
    }
#endif
