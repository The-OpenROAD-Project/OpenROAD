/**************************************************************************
***
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
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

#include "fmPartPlus.h"
#include "Partitioners/greedyHER.h"
#include "fmPart.h"

void FMPartitionerPlus::doOneFMPlus(unsigned initSoln) {
        //     cout<<"DPDEBUG running an FM "<<endl;
        doOneFM(initSoln);

        //     cout<<"DPDEBUG running a conservative HER "<<endl;
        _herParams.acceptZeroGainMoves = false;
        doOneGreedyHER(initSoln);

        //     cout<<"DPDEBUG running an exploratory HER "<<endl;
        _herParams.acceptZeroGainMoves = true;
        doOneGreedyHER(initSoln);
}

FMPartitionerPlus::FMPartitionerPlus(PartitioningProblem& problem, /* MaxMem* maxMem,*/ const MultiStartPartitioner::Parameters& params, bool skipSolnGen) : MultiStartPartitioner(problem, params), FMPartitioner(problem, /* maxMem,*/ params, skipSolnGen, true), GreedyHERPartitioner(problem, GreedyHERPartitioner::Parameters(params), true, true) { /*FMPartitioner::*/ runMultiStart(); }
