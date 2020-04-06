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

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "ABKCommon/abkcommon.h"
#include "ABKCommon/abklimits.h"
#include "Stats/stats.h"
#include "hgFixed.h"

#include <cfloat>
#include <ABKCommon/uofm_alloc.h>

using std::ostream;
using std::endl;
using std::setw;
using uofm::vector;

void HGraphFixed::printEdgeSizeStats(ostream& out) const {
        itHGFEdgeGlobal e = edgesBegin();
        unsigned k, nEdges = getNumEdges();
        vector<unsigned> edgeSizes(nEdges);
        KeyCounter keyCounter;

        for (k = 0; k != nEdges; k++, e++) {
                unsigned deg = (*e)->getDegree();
                edgeSizes[k] = deg;
                keyCounter.addKey(deg);
        }

        out << endl << " Edge sizes : " << TrivialStats(edgeSizes) << keyCounter;
}

void HGraphFixed::printEdgeWtStats(ostream& out) const {
        out << std::setprecision(4);
        vector<double> weights;

        if (getNumMultiWeights() > 1) out << " ----- Statistics for edge weight" << endl;

        unsigned numZeros = 0, numMinNonZero = 0;
        double minNonZero = DBL_MAX;
        double maxWt = 0.0;
        double totalWt = 0.0;

        itHGFEdgeGlobal e;
        for (e = edgesBegin(); e != edgesEnd(); e++) {
                double wt = (*e)->getWeight();
                maxWt = std::max(maxWt, wt);
                totalWt += wt;
                weights.push_back(wt);
                if (wt == 0.0)
                        numZeros++;
                else if (wt == minNonZero)
                        numMinNonZero++;
                else if (wt < minNonZero) {
                        minNonZero = wt;
                        numMinNonZero = 1;
                }
        }

        out << TrivialStats(weights);
        out << " Total Wt: " << setw(8) << totalWt << " Max:  " << setw(7) << (maxWt / totalWt) * 100 << "%   "
            << " Min nonzero: " << minNonZero << endl;
        unsigned numOthers = getNumEdges() - (numZeros + numMinNonZero);
        out << " Num zeros: " << setw(5) << numZeros << "  "
            << " Num minNZ: " << setw(5) << numMinNonZero << " "
            << " Num others:  " << numOthers << endl;
}

void HGraphFixed::printNodeWtStats(ostream& out) const {

        unsigned oldPrec = out.precision();
        out << std::setprecision(4);
        unsigned nNodes = getNumNodes(), numWeights = getNumMultiWeights();
        vector<vector<double> > weights(numWeights);

        unsigned nw;
        for (nw = 0; nw != numWeights; nw++) {
                if (getNumMultiWeights() > 1) out << " ----- Statistics for weight " << nw << " ----- " << endl;
                vector<double>& currWeights = weights[nw];

                unsigned numZeros = 0, numMinNonZero = 0;
                double minNonZero = DBL_MAX;
                double maxWt = 0.0;
                double totalWt = 0.0;

                itHGFNodeGlobal n = nodesBegin();
                for (; n != nodesEnd(); n++) {
                        double wt = getWeight((*n)->getIndex(), nw);
                        maxWt = std::max(maxWt, wt);
                        totalWt += wt;
                        currWeights.push_back(wt);
                        if (wt == 0.0)
                                numZeros++;
                        else if (wt == minNonZero)
                                numMinNonZero++;
                        else if (wt < minNonZero) {
                                minNonZero = wt;
                                numMinNonZero = 1;
                        }
                }

                if (numWeights == 1) out << "\n Weights :";
                out << TrivialStats(currWeights);
                out << " Total Wt: " << setw(8) << totalWt << " Max:  " << setw(7) << (maxWt / totalWt) * 100 << "%   "
                    << " Min nonzero: " << minNonZero << endl;
                unsigned numOthers = nNodes - (numZeros + numMinNonZero);
                out << " Num zeros: " << setw(5) << numZeros << "  "
                    << " Num minNZ: " << setw(5) << numMinNonZero << " "
                    << " Num others:  " << numOthers << endl;
                if (numOthers * 10 > nNodes) out << CumulativeFrequencyDistribution(currWeights, 20) << endl;
        }

        if (nw > 1) {
                out << " --- Correlations between weights --- " << endl;
                for (unsigned w1 = 0; w1 != numWeights; w1++) {
                        for (unsigned w2 = 0; w2 != numWeights; w2++) {
                                if (w2 < w1) {
                                        out << "          ";
                                        continue;
                                }
                                if (w2 == w1) {
                                        out << setw(7) << 1 << " ";
                                        continue;
                                }
                                out << setw(9) << Correlation(weights[w1], weights[w2]) << " ";
                        }
                        out << endl;
                }
        }
        out << std::setprecision(oldPrec);
}

void HGraphFixed::printNodeDegreeStats(ostream& out) const {
        itHGFNodeGlobal n = nodesBegin();
        unsigned k, nNodes = getNumNodes();
        vector<unsigned> nodeDegs(nNodes);
        KeyCounter keyCounter;
        unsigned totalPins = 0;

        for (k = 0; k != nNodes; k++, n++) {
                unsigned deg = (*n)->getDegree();
                nodeDegs[k] = deg;
                keyCounter.addKey(deg);
                totalPins += deg;
        }

        out << " Node degrees: ";
        out << TrivialStats(nodeDegs);
        out << " Total Pins: " << totalPins << endl;
        out << keyCounter;
}

HGN operator<<(const HGraphFixed& hg, const HGFNode& node) {
        // out<<"Node: "<<getNodeNameByIndex(node.getIndex())<<" (#
        // "<<node.getIndex()<<")"<<endl;
        // out<<" degree="<<node.getDegree()<<" weight(s)=";

        // unsigned i = 0;
        // for(i=0; i!=node.getNumWeights(); ++i)
        //    out<<node.getWeight(i)<<" ";

        // return out;
        return HGN(hg, node);
}

ostream& operator<<(ostream& out, const HGN& hgn) {
        out << "Node: " << hgn._hg.getNodeNameByIndex(hgn._node.getIndex()) << " (# " << hgn._node.getIndex() << ")" << endl;
        out << " degree=" << hgn._node.getDegree() << " weight(s)=";

        unsigned i = 0;
        for (i = 0; i != hgn._hg.getNumWeights(); ++i) out << hgn._hg.getWeight(hgn._node.getIndex(), i) << " ";

        return out;
}

// ostream& operator<<(ostream& out, const HGFNode& node)
//{
//    out<<"Node: "<<getNodeNameByIndex(node.getIndex())<<" (#
// "<<node.getIndex()<<")"<<endl;
//    out<<" degree="<<node.getDegree()<<" weight(s)=";
//
//    unsigned i = 0;
//    for(i=0; i!=node.getNumWeights(); ++i)
//        out<<node.getWeight(i)<<" ";
//
//    return out;
//}

ostream& operator<<(ostream& out, const HGFEdge& edge) {
        out << "hyperedge id=" << edge.getIndex() << " degree=" << edge.getDegree() << " weight=" << edge.getWeight();

#ifdef SIGNAL_DIRECTIONS
        out << "  srcs:\n";
        printRange(out, edge.srcsBegin(), edge.srcsEnd());
        out << "  snks:\n";
        printRange(out, edge.snksBegin(), edge.snksEnd());
        out << "  srcsnks:\n";
        printRange(out, edge.srcSnksBegin(), edge.srcSnksEnd());
#else
        out << "  srcsnks:\n";
        printRange(out, edge.nodesBegin(), edge.nodesEnd());
#endif

        return out;
}

ostream& operator<<(ostream& out, const HGraphFixed& graph) {
        // out                  <<TimeStamp();
        out << "format HG\n";  // format name

        out << setw(8) << graph.getNumNodes() << " nodes\n";
        out << setw(8) << graph.getNumEdges() << " hyperedges\n";
        out << setw(8) << graph.getNumMultiWeights() << " weights on each node\n";
        out << setw(8) << graph.getMaxNodeDegree() << " is max node degree\n";
        out << setw(8) << graph.getMaxEdgeDegree() << " is max edge degree\n";

        itHGFNodeGlobal node;
        for (node = graph.nodesBegin(); node != graph.nodesEnd(); ++node) {
                out << (graph << **node) << '\n';
        }

        itHGFEdgeGlobal edge;
        for (edge = graph.edgesBegin(); edge != graph.edgesEnd(); ++edge) {
                out << **edge << "\n";
        }

        abkfatal(out, "could not write to stream");
        return out;
}
