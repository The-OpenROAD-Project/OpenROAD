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




//  Created : November 1997, Mike Oliver, VLSI CAD ABKGROUP UCLA 
//  020811  ilm   ported to g++ 3.0

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "abktimer.h"
#include "abkrand.h"
#include "abkMD5.h"
#include <fstream>
#include <cstdio>

#ifdef _MSC_VER
#define _X86_
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#endif

using std::ios;
using std::ofstream;
using std::ifstream;
using std::cout;
using std::cerr;
using std::endl;
using std::map;

ofstream *SeedHandler::_PseedOutFile = NULL;
ifstream *SeedHandler::_PseedInFile = NULL;
unsigned  SeedHandler::_nextSeed = UINT_MAX;
bool      SeedHandler::_loggingOff=false;
bool      SeedHandler::_haveRandObj=false;
bool      SeedHandler::_cleaned=false;
unsigned  SeedHandler::_externalSeed=UINT_MAX;
unsigned  SeedHandler::_progOverrideExternalSeed=UINT_MAX;

map<std::string,unsigned> SeedHandler::_counters;

SeedCleaner   cleaner; //when this object goes out of scope at the end of
                   //execution, it will make sure that SeedHandler::clean()
                   //gets called


//: Maintains all random seeds used by a process.
//Idea:  First time you create a random object, an initial nondeterministic
//       seed (also called the "external seed" is created from system
//       information and logged to seeds.out.
//       Any random object created with a seed of UINT_MAX ("nondeterministic"),
//       and that does not use the snazzy new "local independence" feature,
//       then uses seeds from a sequence starting with the initial value
//       and increasing by 1 with each new nondeterministic random object
//       created.
//
//       If the seed is overridden with a value other than UINT_MAX it is used,
//       and logged to seeds.out .
//
//       However if seeds.in exists, the initial nondeterministic value
//       is taken from the second line of seeds.in rather than the system
//       clock, and subsequent *deterministic* values are taken from successive
//       lines of seeds.in rather than from the value passed.
//
//       If seeds.out is in use or other errors occur trying to create it,
//       a warning is generated that the seeds will not be logged, and
//       the initial nondeterministic seed is printed (which should be
//       sufficient to recreate the run provided that all "deterministic"
//       seeds truly *are* deterministic (e.g. don't depend on external
//       devices).
//
//       You can disable the logging function by calling the static
//       function SeedHandler::turnOffLogging().  This will not disable
//       the ability to override seeding with seeds.in .
//NEW ULTRA-COOL FEATURE: (17 June 1999)
//       You now have the option, which I (Mike) highly recommend,
//       of specifying, instead of a seed, a "local identifier", which
//       is a string that should identify uniquely the random variable
//       (not necessarily the random object) being used.  The suggested
//       format of this string is as follows: If your
//       RNG is called _rng and is a member of MyClass, pass
//       the string "MyClass::_rng".  If it's a local variable
//       called rng in the method MyClass::myMethod(), pass
//       the string "MyClass::myMethod(),_rng".
//
//       SeedHandler will maintain a collection of counters associated
//       with each of these strings.  The sequence of random numbers
//       from a random object will be determined by three things:
//            1) the "external seed" (same as "initial nondeterm seed").
//            2) the "local identifier" (the string described above).
//            3) the counter saying how many RNG objects have previously
//                 been created with the same local identifier.
//       When any one of these changes, the output should be effectively
//       independent of the previous RNG.  Moreover you don't have
//       to worry about your regressions changing simply because someone
//       else's code which you call has changed the number of RNG objects
//       it uses.
//ANOTHER NEW FEATURE (21 February 2001)
//       At times you may wish to control the external seed yourself, e.g.
//       by specifying the seed on the command line, rather than
//       using either seeds.in or the nondeterministic chooser.
//       To enable this, a static method SeedHandler::overrideExternalSeed()
//       has been added.  If you call this method before any random
//       object has been created, you can set the external seed.
//       The file seeds.out will still be created unless you call
//       SeedHandler::turnOffLogging().  If seeds.in exists, it
//       will control -- your call to overrideExternalSeed() will
//       print a warning message and otherwise be ignored.


//****************************************************************************
//****************************************************************************
//****************************************************************************

void SeedHandler::_init()
    {
    abkfatal(!_cleaned,"Can't create random object"
                       " after calling SeedHandler::clean()");

    if (!_haveRandObj)
        {
        _chooseInitNondetSeed();

        if (!_loggingOff)
            {
            _initializeOutFile();
            }
        }

    _haveRandObj=true;

    }

//****************************************************************************
//****************************************************************************

SeedHandler::SeedHandler(unsigned seed):
               _locIdent(NULL),_counter(UINT_MAX),_isSeedMultipartite(false)
    {
    _init();
    if (seed==UINT_MAX)
        _seed = _nextSeed++;
    else                     //deterministic seed, take value given unless
                             //seeds.in exists with valid value
        {
        if (_PseedInFile)
            {
            *_PseedInFile>>_seed;
            //cerr << "Seed read was " << _seed << endl;

            if (_PseedInFile->fail()) //if we've run out, go back to accepting
                                      //seeds given
                {
                _PseedInFile->close();
                delete _PseedInFile;
                _PseedInFile = NULL;
                _seed=seed;
                //cerr << "But read failed, substituting seed " << _seed;
                }

            }
        else
            _seed = seed;

        if (!_loggingOff)
            {
            abkfatal(_PseedOutFile,"Internal error: unable to log "
                                   "random seed to file");
            *_PseedOutFile << _seed << endl;
            }
        }
    }

//****************************************************************************
//****************************************************************************
SeedHandler::SeedHandler(const char *locIdent,
                         unsigned counterOverride):
            _isSeedMultipartite(true)
    {
    _init();
    _locIdent = strdup(locIdent);

    if (counterOverride!=UINT_MAX)
        _counter=counterOverride;
    else
        {
        map<std::string,unsigned>::iterator iC;
        iC = _counters.find(_locIdent);
        if (iC==_counters.end()) _counters[_locIdent]=_counter=0;
        else { iC->second++; _counter=iC->second; }
        }
    }

//****************************************************************************
//****************************************************************************
SeedHandler::~SeedHandler()
    {
    if (!_isSeedMultipartite)
        {
        abkassert(_locIdent==NULL,"Non-null _locIdent with old-style seed");
        }
    else
        {
          free(_locIdent);
          _locIdent=NULL;
        }
    }
//****************************************************************************
//****************************************************************************

void SeedHandler::turnOffLogging()
    {
    _loggingOff=true;
    }

//****************************************************************************
//****************************************************************************

void SeedHandler::overrideExternalSeed(unsigned extseed)
    {
    abkfatal(!_haveRandObj,"Can't call SeedHandler::overrideExternalSeed() "
        "after creating a random object");
    abkfatal(extseed != UINT_MAX,
        "Can't pass UINT_MAX to SeedHandler::overrideExternalSeed()");
    _progOverrideExternalSeed=extseed;
    }

//****************************************************************************
//****************************************************************************

void SeedHandler::clean()
    {
    _cleaned=true;

    if(_PseedOutFile)
        {
        _PseedOutFile->seekp(0,ios::beg); //rewind file
        _PseedOutFile->put('0');          //mark not in use
        _PseedOutFile->close();
        }

    if(_PseedInFile)_PseedInFile->close();

    delete _PseedOutFile;delete _PseedInFile;
    _PseedOutFile=NULL;
    _PseedInFile=NULL;
    }


//****************************************************************************
//****************************************************************************

void SeedHandler::_chooseInitNondetSeed()
    {
    _PseedInFile = new ifstream("seeds.in",ios::in);

    if (!*_PseedInFile)
        {
        delete _PseedInFile;
        _PseedInFile=NULL;
        }

    if (_PseedInFile)
        {
        unsigned DummyInUseLine;
        cerr<<"File seeds.in exists and will override all seeds, including "
              "hardcoded ones " << endl;
        *_PseedInFile >> DummyInUseLine;
        abkwarn(!DummyInUseLine,"File seeds.in appears to come from a process "
                                "that never completed\n");
        *_PseedInFile >> _nextSeed;
        //cerr << "Init nondet seed read was "<<_nextSeed << endl;
        abkfatal(!(_PseedInFile->fail()),"File seeds.in exists but program "
                                         "was unable to read initial seed "
                                         "therefrom.");
        abkwarn(_progOverrideExternalSeed==UINT_MAX,
            "\nThere is a programmatic override of the external seed,"
            " which will be ignored because seeds.in exists");
        }
    else if (_progOverrideExternalSeed==UINT_MAX)
        {
        Timer tm;

        char buf[255];
#if defined(WIN32)
        int procID=_getpid();
        LARGE_INTEGER hiPrecTime;
        ::QueryPerformanceCounter(&hiPrecTime);
        LONGLONG hiP1=hiPrecTime.QuadPart;
        sprintf(buf,"%g %d %I64d",tm.getUnixTime(),procID,hiP1);
#else
        unsigned procID=getpid();
        unsigned rndbuf;
        FILE *rnd=fopen("/dev/urandom","r");
        if(rnd)
          {
           fread(&rndbuf,sizeof(rndbuf),1,rnd);
           fclose(rnd);
           sprintf(buf,"%g %d %d",tm.getUnixTime(),procID,rndbuf);
          }
        else
          sprintf(buf,"%g %d",tm.getUnixTime(),procID);
#endif

        MD5 hash(buf);
        _nextSeed=hash;
        }
    else
        {
        _nextSeed=_progOverrideExternalSeed;
        }
    _externalSeed=_nextSeed;
    }

//****************************************************************************
//****************************************************************************

void SeedHandler::_initializeOutFile()
    {
    unsigned inUse;

    ifstream outFileAsInFile("seeds.out",ios::in);

    if (!!outFileAsInFile)
        {

        outFileAsInFile >> inUse;
        if (outFileAsInFile.fail())
            {
            char errtxt[1023];

            sprintf(errtxt,"Unable to determine whether file seeds.out is"
                           " in use; random seeds from "
                           "this process will not be logged.\n  Initial non-"
                           "deterministic seed is %u",_nextSeed);

            abkwarn(0,errtxt);
            _loggingOff=true;
            }
        else
            {
            if (inUse)
                {
                char errtxt[1023];

                sprintf(errtxt,"File seeds.out is in use; random seeds from "
                               "this process will not be logged.\nInitial non-"
                               "deterministic seed is %u.\nIf there is no "
                               "other process running in this directory, then "
                               "an earlier\nprocess may have terminated\n"
                               "abnormally. "
                               "You should remove or rename seeds.out\n"
                               ,_nextSeed);

                abkwarn(0,errtxt);
                _loggingOff=true;
                }
            }
        }
        outFileAsInFile.close();

        if (!_loggingOff)
            {
            _PseedOutFile=new ofstream("seeds.out",ios::out); //overwrite file

            if (!_PseedOutFile)
                {
                char errtxt[1023];

                sprintf(errtxt,"Unable to create file seeds.out; random seeds from "
                               "this process will not be logged.\n  Initial non-"
                               "deterministic seed is %u",_nextSeed);

                abkwarn(0,errtxt);
                _loggingOff=true;
                delete _PseedOutFile;_PseedOutFile=NULL;
                }

            *_PseedOutFile << "1" << endl; //mark as in use

            *_PseedOutFile << _nextSeed << endl; //initial nondet seed
            }

    }

//****************************************************************************
//****************************************************************************
//****************************************************************************
//****************************************************************************



