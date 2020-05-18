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

// created by Igor Markov on 05/20/98
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "Partitioning/partitionData.h"
#include "Partitioning/partProb.h"
#include "univPartEval.h"
#include "Stats/stats.h"
#include "HGraph/hgFixed.h"

// Now include headers for all evaluators available
#include "netCutWBits.h"
#include "netCutWConfigIds.h"
#include "netCutWNetVec.h"
#include "bbox1dim.h"
/*
#include "bbox2dim.h"
#include "bbox1dimWCheng.h"
#include "bbox2dimWCheng.h"
#include "bbox2dimWRSMT.h"
#include "hbbox.h"
#include "hbboxWCheng.h"
#include "hbboxWRSMT.h"
#include "hbbox0.h"
#include "hbbox0wCheng.h"
#include "hbbox0wRSMT.h"
*/
#include "strayNodes.h"
#include "netCut2way.h"
#include "netCut2wayWWeights.h"
#include "strayNodes2way.h"

using std::cout;
using std::endl;
using uofm::vector;

#define COMPUTE_COST(problem, part, EVAL)   \
        {                                   \
                ::EVAL eval(problem, part); \
                return eval.getTotalCost(); \
        }

unsigned UniversalPartEval::computeCost(const PartitioningProblem& problem, const Partitioning& part) const {
        switch (_type) {
                case NetCutWBits:
                        COMPUTE_COST(problem, part, NetCutWBits)
                case NetCutWConfigIds:
                        COMPUTE_COST(problem, part, NetCutWConfigIds)
                case NetCutWNetVec:
                        COMPUTE_COST(problem, part, NetCutWNetVec)
                case BBox1Dim:
                        COMPUTE_COST(problem, part, BBox1Dim)
                /*
                      case BBox2Dim         :
                   COMPUTE_COST(problem,part,BBox2Dim)
                      case BBox1DimWCheng   :
                   COMPUTE_COST(problem,part,BBox1DimWCheng)
                      case BBox2DimWCheng   :
                   COMPUTE_COST(problem,part,BBox2DimWCheng)
                      case BBox2DimWRSMT    :
                   COMPUTE_COST(problem,part,BBox2DimWRSMT)
                      case HBBox            : COMPUTE_COST(problem,part,HBBox)
                      case HBBoxWCheng      :
                   COMPUTE_COST(problem,part,HBBoxWCheng)
                      case HBBoxWRSMT       :
                   COMPUTE_COST(problem,part,HBBoxWRSMT)
                      case HBBox0           : COMPUTE_COST(problem,part,HBBox0)
                      case HBBox0wCheng     :
                   COMPUTE_COST(problem,part,HBBox0wCheng)
                      case HBBox0wRSMT      :
                   COMPUTE_COST(problem,part,HBBox0wRSMT)
                */
                case StrayNodes:
                        COMPUTE_COST(problem, part, StrayNodes)
                case NetCut2way:
                        COMPUTE_COST(problem, part, NetCut2way)
                case NetCut2wayWWeights:
                        COMPUTE_COST(problem, part, NetCut2wayWWeights)
                case StrayNodes2way:
                        COMPUTE_COST(problem, part, StrayNodes2way)
                default:
                        abkfatal(0,
                                 " Evaluator requested unknown to PartEvalType "
                                 "class\n");
        }
        return UINT_MAX;
}

#define INSTANTIATE_EVAL(problem, part, EVAL)     \
        {                                         \
                eval = new ::EVAL(problem, part); \
                break;                            \
        }

void UniversalPartEval::printStatsForNon0CostNets(const PartitioningProblem& problem, const Partitioning& part) const {
        const HGraphFixed& hg = problem.getHGraph();
        PartEvalXFace* eval = NULL;
        switch (_type) {
                case NetCut2way:
                        INSTANTIATE_EVAL(problem, part, NetCut2way);
                case NetCut2wayWWeights:
                        INSTANTIATE_EVAL(problem, part, NetCut2wayWWeights);
                case StrayNodes2way:
                        INSTANTIATE_EVAL(problem, part, StrayNodes2way);
                case NetCutWConfigIds:
                        INSTANTIATE_EVAL(problem, part, NetCutWConfigIds)
                case NetCutWNetVec:
                        INSTANTIATE_EVAL(problem, part, NetCutWNetVec)
                case BBox1Dim:
                        INSTANTIATE_EVAL(problem, part, BBox1Dim)
                /*
                      case BBox2Dim         :
                   INSTANTIATE_EVAL(problem,part,BBox2Dim)
                      case BBox1DimWCheng   :
                   INSTANTIATE_EVAL(problem,part,BBox1DimWCheng)
                      case BBox2DimWCheng   :
                   INSTANTIATE_EVAL(problem,part,BBox2DimWCheng)
                      case BBox2DimWRSMT    :
                   INSTANTIATE_EVAL(problem,part,BBox2DimWRSMT)
                      case HBBox            :
                   INSTANTIATE_EVAL(problem,part,HBBox)
                      case HBBoxWCheng      :
                   INSTANTIATE_EVAL(problem,part,HBBoxWCheng)
                      case HBBoxWRSMT       :
                   INSTANTIATE_EVAL(problem,part,HBBoxWRSMT)
                      case HBBox0           :
                   INSTANTIATE_EVAL(problem,part,HBBox0)
                      case HBBox0wCheng     :
                   INSTANTIATE_EVAL(problem,part,HBBox0wCheng)
                      case HBBox0wRSMT      :
                   INSTANTIATE_EVAL(problem,part,HBBox0wRSMT)
                */
                case StrayNodes:
                        INSTANTIATE_EVAL(problem, part, StrayNodes)
                default:
                        abkfatal(0,
                                 " Evaluator requested unknown to PartEvalType "
                                 "class\n");
        };

        itHGFEdgeGlobal e = hg.edgesBegin();
        KeyCounter keyCounter;
        unsigned k = 0;
        vector<unsigned> edgeSizes;
        edgeSizes.reserve(hg.getNumEdges());
        for (; e != hg.edgesEnd(); e++)
                if (eval->getNetCost(k++) != 0) {
                        unsigned deg = (*e)->getDegree();
                        edgeSizes.push_back(deg);
                        keyCounter.addKey(deg);
                }
        cout << endl << " Cut edge sizes : ";
        if (edgeSizes.size())
                cout << TrivialStats(edgeSizes) << keyCounter;
        else
                cout << "none" << endl;
        delete eval;
        eval = NULL;
}

unsigned UniversalPartEval::getNumModulesOnNon0CostNets(const PartitioningProblem& problem, const Partitioning& part) const {

        uofm_bit_vector _moduleCounted(problem.getHGraph().getNumNodes(), false);
        const HGraphFixed& hg = problem.getHGraph();

        if (_type == NetCut2way || _type == NetCut2wayWWeights || _type == StrayNodes2way) {
                TalliesWCosts2way* eval = NULL;
                switch (_type) {
                        case NetCut2way:
                                INSTANTIATE_EVAL(problem, part, NetCut2way);
                                break;
                        case NetCut2wayWWeights:
                                INSTANTIATE_EVAL(problem, part, NetCut2wayWWeights);
                                break;
                        case StrayNodes2way:
                                INSTANTIATE_EVAL(problem, part, StrayNodes2way);
                                break;
                        default:
                                abkfatal(0, "Control should not reach here\n");
                }

                unsigned numModulesCounted = 0;
                for (unsigned netIdx = hg.getNumEdges(); netIdx != 0;) {
                        if (eval->getNetCost(--netIdx) == 0) continue;
                        const HGFEdge& e = hg.getEdgeByIdx(netIdx);
                        for (itHGFNodeLocal node = e.nodesBegin(); node != e.nodesEnd(); node++) {
                                unsigned nodeIdx = (*node)->getIndex();
                                if (_moduleCounted[nodeIdx]) continue;
                                if (hg.isTerminal(nodeIdx)) continue;
                                _moduleCounted[nodeIdx] = true;
                                numModulesCounted++;
                        }
                }
                delete eval;
                return numModulesCounted;
        } else {
                TalliesWCosts* eval = NULL;

                switch (_type) {
                        case NetCutWBits:
                                abkfatal(0, "NetCutWBits not supported here");
                        case NetCutWConfigIds:
                                INSTANTIATE_EVAL(problem, part, NetCutWConfigIds)
                        case NetCutWNetVec:
                                INSTANTIATE_EVAL(problem, part, NetCutWNetVec)
                        case BBox1Dim:
                                INSTANTIATE_EVAL(problem, part, BBox1Dim)
                        /*
                              case BBox2Dim         :
                           INSTANTIATE_EVAL(problem,part,BBox2Dim)
                              case BBox1DimWCheng   :
                           INSTANTIATE_EVAL(problem,part,BBox1DimWCheng)
                              case BBox2DimWCheng   :
                           INSTANTIATE_EVAL(problem,part,BBox2DimWCheng)
                              case BBox2DimWRSMT    :
                           INSTANTIATE_EVAL(problem,part,BBox2DimWRSMT)
                              case HBBox            :
                           INSTANTIATE_EVAL(problem,part,HBBox)
                              case HBBoxWCheng      :
                           INSTANTIATE_EVAL(problem,part,HBBoxWCheng)
                              case HBBoxWRSMT       :
                           INSTANTIATE_EVAL(problem,part,HBBoxWRSMT)
                              case HBBox0           :
                           INSTANTIATE_EVAL(problem,part,HBBox0)
                              case HBBox0wCheng     :
                           INSTANTIATE_EVAL(problem,part,HBBox0wCheng)
                              case HBBox0wRSMT      :
                           INSTANTIATE_EVAL(problem,part,HBBox0wRSMT)
                        */
                        case StrayNodes:
                                INSTANTIATE_EVAL(problem, part, StrayNodes)
                        default:
                                abkfatal(0,
                                         " Evaluator requested unknown to "
                                         "PartEvalType class\n");
                }
                unsigned numModulesCounted = 0;
                for (unsigned netIdx = hg.getNumEdges(); netIdx != 0;) {
                        if (eval->getNetCost(--netIdx) == 0) continue;
                        const HGFEdge& e = hg.getEdgeByIdx(netIdx);
                        for (itHGFNodeLocal node = e.nodesBegin(); node != e.nodesEnd(); node++) {
                                unsigned nodeIdx = (*node)->getIndex();
                                if (_moduleCounted[nodeIdx]) continue;
                                if (hg.isTerminal(nodeIdx)) continue;
                                _moduleCounted[nodeIdx] = true;
                                numModulesCounted++;
                        }
                }
                delete eval;
                return numModulesCounted;
        }
        abkfatal(0, "Internal error: control should not reach here ");
        return UINT_MAX;
}
