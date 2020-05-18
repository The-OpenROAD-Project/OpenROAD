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

// Created by Igor Markov on 11/29/98

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "Combi/grayPart.h"
#include "HGraph/hgFixed.h"
#include "Partitioning/partProb.h"
#include "PartEvals/netCut2way.h"
#include "enumPart.h"

using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::max;
using uofm::vector;

const unsigned MAXNUMNODES = 100;

EnumPart::EnumPart(PartitioningProblem& problem, Verbosity verb)
    //    : BaseSmallPartitioner(problem, verb)
{
        Timer tm;
        PartitioningBuffer& buf = problem.getSolnBuffers();
        Partitioning& result = buf[buf.beginUsedSoln()];
        problem.setBestSolnNum(buf.beginUsedSoln());

        const Partitioning& fixedConstr = problem.getFixedConstr();
        const HGraphFixed& hgraph = problem.getHGraph();
        unsigned nPart = problem.getNumPartitions();
        unsigned nMovables = 0, nNodes = hgraph.getNumNodes();

        abkfatal(nPart == 2, " #parts != 2 not supported yet");

        unsigned part[MAXNUMNODES], bestPart[MAXNUMNODES], movables[MAXNUMNODES];
        PartitionIds part0, part1, part01;
        part0.setToPart(0);
        part1.setToPart(1);
        part01.setToAll(2);

        unsigned k;
        for (k = nNodes; k != 0;) {
                k--;
                if (fixedConstr[k].isEmpty() || fixedConstr[k] == part01) {  // unconstrained node
                        result[k] = part0;
                        part[nMovables] = 0;
                        movables[nMovables++] = k;
                }
                //    else result[k]=fixedConstr[k];
        }
        std::copy(part, part + nMovables, bestPart);

        abkfatal(nMovables != 0, " Partitioning problem with everything fixed\n");
        abkfatal3(nMovables <= MAXNUMNODES, " Small part problem has too many moveables (", nNodes, ")\n");
        if (verb.getForMajStats()) cout << " Found " << nMovables << " movables" << endl;

        GrayCodeForPartitionings grayCode(nMovables, nPart);
        NetCut2way eval(problem);  // need to resetTo(result) later
        // SingleBalanceLegality    leg(problem,result);
        double part0min = problem.getMinCapacities()[0][0], part0max = problem.getMaxCapacities()[0][0];
        //        part0tar=problem.getCapacities()[0][0];
        double part0actual = 0.0;

        double weights[MAXNUMNODES];
        for (unsigned n = 0; n != nMovables; n++) {
                part0actual += weights[n] = hgraph.getWeight(movables[n]);
        }
        // for(unsigned n=0; n!=result.size(); n++)
        //   if (result[n][0]) part0actual+=hgraph.getNodeByIdx(n).getWeight();
        // for(n=0; n!=nMovables; n++)
        //   weights[n]=hgraph.getNodeByIdx(movables[n]).getWeight();

        // first make sure there are legal partitions
        // The loop below either finishes with the first legal solution in
        // bestPart
        //  or one of the most balanced, but still illegal, solution in bestPart
        // In the latter case  grayCode.finished() and no work needs to be done,
        // in the latter -- need to start using eval and do another loop

        // double   bestMaxViol   =leg.getMaxViolation();
        double bestMaxViol = max(part0actual - part0max, part0min - part0actual);

        while (!grayCode.finished()) {
                unsigned flippedNode = grayCode.nextIncrement();
                unsigned from = part[flippedNode];
                part[flippedNode] = 1 - part[flippedNode];
                //    unsigned to  =part[flippedNode];
                //    unsigned nodeIdx=movables[flippedNode];

                //    leg.moveModuleTo(nodeIdx,from,to);
                if (from == 0)
                        part0actual -= weights[flippedNode];
                else
                        part0actual += weights[flippedNode];

                //       double currMaxViol=leg.getMaxViolation();
                double currMaxViol = max(part0actual - part0max, part0min - part0actual);
                if (currMaxViol >= bestMaxViol) continue;

                std::copy(part, part + nMovables, bestPart);
                bestMaxViol = currMaxViol;

                if (bestMaxViol <= 0.0) {
                        bestMaxViol = 0.0;
                        break;
                }
        }

        for (k = 0; k != nMovables; k++) result[movables[k]].setToPart(bestPart[k]);
        eval.resetTo(result);
        unsigned bestCut = eval.getTotalCost();

        if (grayCode.finished()) {
                abkassert(0, " No legal solutions ");
                goto end;
        }

        if (verb.getForMajStats()) cout << "Legal solution found" << endl;

        while (!grayCode.finished()) {  // a legal solution has been already
                                        // found, so skip all illegals
                unsigned flippedNode = grayCode.nextIncrement();
                unsigned from = part[flippedNode];
                part[flippedNode] = 1 - part[flippedNode];
                unsigned to = part[flippedNode];
                unsigned nodeIdx = movables[flippedNode];

                eval.moveModuleTo(nodeIdx, from, to);

                //    leg.moveModuleTo(nodeIdx,from,to);
                if (from == 0)
                        part0actual -= weights[flippedNode];
                else
                        part0actual += weights[flippedNode];

                //    if  ( ! leg.isPartLegal() )
                if (part0actual < part0min || part0actual > part0max) continue;

                unsigned currCut = eval.getTotalCost();
                if (eval.getTotalCost() < bestCut) {
                        bestCut = currCut;
                        std::copy(part, part + nMovables, bestPart);
                        bestMaxViol = 0;
                        if (currCut == 0) break;
                }
        }

        for (k = 0; k != nMovables; k++) result[movables[k]].setToPart(bestPart[k]);

end:

        vector<double>& viols = const_cast<vector<double>&>(problem.getViolations());
        // vector<double>&
        // imbals=const_cast<vector<double>&>(problem.getImbalances());
        /* the latter won't be set */
        vector<double>& costs = const_cast<vector<double>&>(problem.getImbalances());
        viols[buf.beginUsedSoln()] = bestMaxViol;
        costs[buf.beginUsedSoln()] = bestCut;

        tm.stop();
        if (verb.getForMajStats()) {
                cout << flush << " Resulting cut is " << bestCut << endl;
                eval.resetTo(result);
                cerr << flush;
                cout << flush << " Max violations   " << bestMaxViol << endl;
                cerr << flush;
                cout << " Checked cost  is " << eval.getTotalCost() << endl;
        }

        if (verb.getForSysRes()) cout << " Exhaustive search took " << tm;
}
