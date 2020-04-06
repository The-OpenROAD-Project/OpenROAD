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

// created on 09/09/98 by Andrew Caldwell (caldwell@cs.ucla.edu)

#ifndef __CLUSTEREDHGRAPH_H_
#define __CLUSTEREDHGRAPH_H_

#include "hemClustHG.h"
#include "pinHemClustHG.h"
#include "bhemClustHG.h"
#include "grhemClustHG.h"
#include "baseClustHGraph.h"
#include "cutOptClustHG.h"
#include "randomClustHG.h"
#include "FilledHier/fillHier.h"
#include "Placement/placeWOri.h"

class FillableHierarchy;

class ClusteredHGraph : public virtual ClHG_ClusterTreeBase, protected HEMClusteredHGraph, protected PinHEMClusteredHGraph, protected BestHEMClusteredHGraph, protected GreedyHEMClusteredHGraph, protected CutOptClusteredHGraph, protected RandomClusteredHGraph {

       public:
        ClusteredHGraph(const HGraphFixed &graph, const Parameters &params);

        ClusteredHGraph(const HGraphFixed &graph, const Parameters &params, const Partitioning &fixed);

        ClusteredHGraph(const HGraphFixed &graph, const Parameters &params, const Partitioning &fixed, const Partitioning &curPart,
                        // argument added by sadya+ramania for analytical
                        // clustering
                        PlacementWOrient *placement = NULL);

        ClusteredHGraph(const ClusteredHGraph &) : ClHG_ClusterTreeBase(HGraphFixed(), Parameters(), NULL), HEMClusteredHGraph(HGraphFixed(), Parameters()), PinHEMClusteredHGraph(HGraphFixed(), Parameters()), BestHEMClusteredHGraph(HGraphFixed(), Parameters()), GreedyHEMClusteredHGraph(HGraphFixed(), Parameters()), CutOptClusteredHGraph(HGraphFixed(), Parameters()), RandomClusteredHGraph(HGraphFixed(), Parameters()) { abkfatal(0, "copy construction is not alowed"); }

        // this constructor levelizes a hierarchy.
        // it assumes that the fanout, etc. are acceptable, and only
        // looks to construct the best levels it can.
        ClusteredHGraph(FillableHierarchy &fillH, const HGraphFixed &leafHG, const Parameters &params);

        // TODO: add a version that takes a 'fixed' partitioning

        virtual ~ClusteredHGraph() {}

       private:
        void setupTree() {
                Timer clTimer;
                switch (_params.clType) {
                        case ClHG_ClusteringType::HEM: {
                                if (_params.verb.getForActions() > 1) std::cout << " HEM Clustering Tree" << std::endl;
                                HEMClusteredHGraph::populateTree();
                                break;
                        }
                        case ClHG_ClusteringType::PHEM: {
                                if (_params.verb.getForActions() > 1) std::cout << " PinHEM Clustering Tree" << std::endl;
                                PinHEMClusteredHGraph::populateTree();
                                break;
                        }
                        case ClHG_ClusteringType::BHEM: {
                                if (_params.verb.getForActions() > 1) std::cout << " BestHEM Clustering Tree" << std::endl;
                                BestHEMClusteredHGraph::populateTree();
                                break;
                        }
                        case ClHG_ClusteringType::GRHEM: {
                                if (_params.verb.getForActions() > 1) std::cout << " GreedyHEM Clustering Tree" << std::endl;
                                GreedyHEMClusteredHGraph::populateTree();
                                break;
                        }
                        case ClHG_ClusteringType::CutOpt: {
                                if (_params.verb.getForActions() > 1) std::cout << " CutOpt Clustering Tree" << std::endl;
                                CutOptClusteredHGraph::populateTree();
                                break;
                        }
                        case ClHG_ClusteringType::Random: {
                                if (_params.verb.getForActions() > 1) std::cout << " Random Clustering Tree" << std::endl;
                                RandomClusteredHGraph::populateTree();
                                break;
                        }
                        default: {
                                abkfatal(0, "Cluster Type without a ctor");
                                std::cout << _params.clType << std::endl;
                        }
                };

                clTimer.stop();
                if (_params.verb.getForMajStats() > 5) std::cout << "ClTree Construction took " << clTimer.getUserTime() << std::endl;
        }

        void createLevels(const FillableHierarchy &fillH);
};

#endif
