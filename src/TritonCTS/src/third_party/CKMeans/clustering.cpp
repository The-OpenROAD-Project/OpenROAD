////////////////////////////////////////////////////////////////////////////////////
// Authors: Jiajia Li
//
// BSD 3-Clause License
//
// Copyright (c) 2018, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////


#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <stack>
#include <limits.h>
#include <sys/timeb.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <algorithm>
#include <lemon/list_graph.h>
#include <lemon/network_simplex.h>
#include <lemon/maps.h>
#include "clustering.h"

#define CPLEX_CLUSTERING

namespace CKMeans{
using namespace std;
using namespace lemon;

int myrandom (int i) {
    //srand(time(NULL));
    return rand()%i;
}


clustering::clustering(const vector<std::pair<float, float>>& sinks, 
                       float xBranch, float yBranch) {
	for (unsigned i = 0; i < sinks.size(); ++i) {
        	flops.push_back(new flop(sinks[i].first, sinks[i].second, i));
		flops.back()->dists.resize(1);	
		flops.back()->match_idx.resize(1);	
		flops.back()->silhs.resize(1);	
	}
	srand(56);
	
	branchingPoint = {xBranch, yBranch};	
}

clustering::~clustering() {
	for (unsigned i = 0; i < flops.size(); ++i) {
		delete flops[i];
	}
}

/*** Capacitated K means **************************************************/
void clustering::iterKmeans(unsigned ITER, unsigned N, unsigned CAP, unsigned IDX, vector<pair<float, float>>& _means, unsigned MAX) {

    vector<pair<float, float>> sol;
    sol.resize(flops.size());
	
	unsigned midIdx = _means.size() / 2 - 1;
	segmentLength = std::abs(_means[midIdx].first - _means[midIdx + 1].first) +
					std::abs(_means[midIdx].second - _means[midIdx + 1].second);
        //std::cout << " _means[midIdx].first " << _means[midIdx].first << " _means[midIdx + 1].first "  << _means[midIdx + 1].first << "\n";
        //std::cout << " _means[midIdx].second " << _means[midIdx].second << " _means[midIdx + 1].second "  << _means[midIdx + 1].second << "\n";
	//cout << "Segment length = " << segmentLength << "\n";
	
	//branchingPoint = make_pair(_means[midIdx].first + (_means[midIdx + 1].first - _means[midIdx].first)/2.0,
							   //_means[midIdx].second + (_means[midIdx + 1].second - _means[midIdx].second)/2.0);
	
    float max_silh = -1;
    for (unsigned i = 0; i < ITER; ++i) {
        //if (verbose > 0)
	//		cout << "Iteration " << i << " ";
        //if (TEST_ITER == 1) 
	//		cout << "Iteration " << i << endl;
        vector<pair<float,float>> means = _means;
        float silh = Kmeans(N, CAP, IDX, means, MAX);
        if (silh > max_silh) {
            max_silh = silh;
            for (unsigned j = 0; j < flops.size(); ++j)
                sol[j] = flops[j]->match_idx[IDX];
            _means.resize(means.size());
            for (unsigned j = 0; j < means.size(); ++j)
                _means[j] = means[j];
        }
    }

    for (unsigned i = 0; i < flops.size(); ++i) 
        flops[i]->match_idx[IDX] = sol[i];

    // print clustering solution
    if (TEST_LAYOUT == 1) {
        ofstream outFile;
        //outFile.open("cluster.sol");
        for (unsigned i = 0; i < _means.size(); ++i) {
            outFile << "TRAY " << i << " " << _means[i].first << " " << _means[i].second << endl;
        }
        for (unsigned i = 0; i < flops.size(); ++i) {
            flop * f = flops[i];
            //outFile << "FLOP " << f->name << " " << flops[i]->match_idx[IDX].first << endl;
        }
        outFile.close();
    }

    //if (CAP < 10) 
    //    cout << "Best SILH  (" << CAP  << ") is " << max_silh << endl;
    //else 
    //    cout << "Best SILH (" << CAP  << ") is " << max_silh << endl;
	
//	cout << "Means position (" << _means.size() << "):\n";
//	for (unsigned i = 0; i < _means.size(); ++i) {
//		cout << _means[i].first << " " << _means[i].second << "\n";
//	}
	
//	cout << "Segment length = " << segmentLength << "\n";
//	cout << "Actual segment sizes before fix: ";
//	for (unsigned i = 0; i < _means.size() - 1; ++i) {
//		float currSegmentLength = std::abs(_means[i].first - _means[i + 1].first) +
//								  std::abs(_means[i].second - _means[i + 1].second);
//		cout << currSegmentLength << " "; 
//	}
//	cout << "\n";
	
	fixSegmentLengths(_means);
	
}

void clustering::fixSegmentLengths(vector<pair<float, float>>& _means) {
	// First, fix the middle positions
	unsigned midIdx = _means.size() / 2 - 1;
	
	const bool overwriteBranchingPoint = false;
	
	if (overwriteBranchingPoint) {
		float minX = std::min(_means[midIdx].first, _means[midIdx + 1].first);
		float minY = std::min(_means[midIdx].second, _means[midIdx + 1].second);
		float maxX = std::max(_means[midIdx].first, _means[midIdx + 1].first);
		float maxY = std::max(_means[midIdx].second, _means[midIdx + 1].second);
		branchingPoint = make_pair(minX + (maxX - minX) / 2.0, minY + (maxY - minY) / 2.0);
	}
	
	fixSegment(branchingPoint, _means[midIdx], segmentLength / 2.0);
	fixSegment(branchingPoint, _means[midIdx + 1], segmentLength / 2.0);
	
	// Fix lower branch
	for (unsigned i = midIdx; i > 0; --i) {
		fixSegment(_means[i], _means[i - 1], segmentLength);
	}
	
	// Fix upper branch
	for (unsigned i = midIdx + 1; i < _means.size() - 1; ++i) {
		fixSegment(_means[i], _means[i + 1], segmentLength);
	}
}

void clustering::fixSegment(const pair<float, float>& fixedPoint, pair<float, float>& movablePoint, float targetDist) {
	float actualDist = calcDist(fixedPoint, movablePoint);
	float ratio =  targetDist / actualDist;
        //std::cout << "tDist: " << targetDist << "\n";
	//ratio = std::min(1.75, std::max(0.25, (double)ratio));
	
	float dx = (fixedPoint.first - movablePoint.first) * ratio;
	float dy = (fixedPoint.second - movablePoint.second) * ratio;

	//cout << "Fixed = (" << fixedPoint.first << ", " << fixedPoint.second << ")\n";
	//cout << "Movable = (" << movablePoint.first << ", " << movablePoint.second << ")\n";
	//cout << "delta = (" << dx << ", " << dy << ")\n";
	
	movablePoint.first = fixedPoint.first - dx;
	movablePoint.second = fixedPoint.second - dy;
}

float clustering::Kmeans (unsigned N, unsigned CAP, unsigned IDX, vector<pair<float, float>>& means, unsigned MAX) {
	vector<vector<flop*>> clusters;    

	for (unsigned i = 0; i < flops.size(); ++i) {
		flops[i]->dists[IDX] = 0;
		for (unsigned j = 0; j < N; ++j)
		{
			flops[i]->dists[IDX] += 
				calcDist(make_pair(means[j].first, means[j].second), flops[i]);
		}
	}
	
    // initialize matching indexes for flops
    for (unsigned i = 0; i < flops.size(); ++i) {
        flop * f = flops[i];
        f->match_idx[IDX] = make_pair(-1, -1);
    }

    bool stop = false;
    // Kmeans optimization
    unsigned iter = 1;
    while (!stop) {

        //if (TEST_LAYOUT == 1 || verbose > 1)
        //cout << "ITERATION " << iter << endl;

        // report initial means
        //if (TEST_LAYOUT == 1 || verbose > 1) {
        //cout << "INIT Tray locations " << endl;
        //for (unsigned i = 0; i < N; ++i)
        //    cout << means[i].first << " " << means[i].second << endl;
        // cout << endl;
        //}

        if (verbose > 1) {
			cout << "match .." << endl;
		}
    
		fixSegmentLengths(means);
		
		// flop to slot matching based on min-cost flow
        if (iter == 1) 
            minCostFlow(means, CAP, IDX, 5200);
        else if (iter == 2) 
            minCostFlow(means, CAP, IDX, 5200);
        else if (iter > 2) 
            minCostFlow(means, CAP, IDX, 5200);

        // collect results
        clusters.clear();
        clusters.resize(N);
        for (unsigned i = 0; i < flops.size(); ++i) {
            flop * f = flops[i];
            clusters[f->match_idx[IDX].first].push_back(f);
        }

        // always use mode 1
        unsigned update_mode = 1;

        if (verbose > 1) 
        	cout << "move .." << endl;
        float delta = 0;
        if (update_mode == 0) {
            // LP-based tray movement
            // delta = LPMove(means, CAP, IDX);
        } else if (update_mode == 1) {
            // use weighted center
            for (unsigned i = 0; i < N; ++i) {
                float sum_x = 0, sum_y = 0;
                for (unsigned j = 0; j < clusters[i].size(); ++j) {
                    sum_x += clusters[i][j]->x;
                    sum_y += clusters[i][j]->y;
                }
                float pre_x = means[i].first;
                float pre_y = means[i].second;
                //means[i] = make_pair(sum_x/clusters[i].size(), sum_y/clusters[i].size());
                //delta += abs(pre_x - means[i].first) + abs(pre_y - means[i].second);
				
				if (clusters[i].size() > 0) {
					means[i] = make_pair(sum_x/clusters[i].size(), sum_y/clusters[i].size());
					delta += abs(pre_x - means[i].first) + abs(pre_y - means[i].second);
				} else {
					cout << "WARNING: Empty cluster [" << i << "] " << 
						"in a level with " << clusters.size() << " clusters.\n"; 
				}
            }
        }

        // report clustering solution
//        if (TEST_LAYOUT == 1 || verbose > 1) {
//        	for (unsigned i = 0; i < N; ++i) {
//            	cout << "Cluster " << i << " (" << clusters[i].size() << ")" << endl;
//            	for (unsigned j = 0; j < clusters[i].size(); ++j) {
//                	cout << clusters[i][j]->x << " " << clusters[i][j]->y << endl;
//            	}
//            	cout << endl;
//        	}
			//plotClusters(clusters, means);
//        }
		
		this->clusters = clusters;
		
        // report final means
		//cout << "Branching point\n";
		//cout << branchPoint.first << " " << branchPoint.second << "\n";
        //if (TEST_LAYOUT == 1 || verbose > 1) {
        	//cout << "FINAL Tray locations " << endl;
	        //for (unsigned i = 0; i < N; ++i) {
    	    //    cout << means[i].first << " " << means[i].second << " (" << calcDist(means[i], branchPoint) << ")"  << endl;
 	        //}
    	    //cout << endl;
        //}

        if (TEST_LAYOUT == 1 || TEST_ITER == 1) {
        	float silh = calcSilh(means, CAP, IDX);        	
        }

        if (iter > MAX || delta < 0.5)
            stop = true;

        ++iter;
    }

    float silh = calcSilh(means, CAP, IDX);
    //if (verbose > 0)
    //		cout << "SILH (" << CAP  << ") is " << silh << endl;

    return silh;
}

float clustering::calcSilh(const vector<pair<float, float>>& means, unsigned CAP, unsigned IDX) {
    float sum_silh = 0;
    for (unsigned i = 0; i < flops.size(); ++i) {
        flop * f = flops[i];
        float in_d = 0, out_d = INT_MAX;
        for (unsigned j = 0; j < means.size(); ++j) {
            float _x = means[j].first;
            float _y = means[j].second;
            if (f->match_idx[IDX].first == j) {
                // within the cluster
                unsigned k = f->match_idx[IDX].second;
                in_d = calcDist(make_pair(_x, _y), f);
            } else {
                // outside of the cluster
                for (unsigned k = 0; k < CAP; ++k) {
                    float d = calcDist(make_pair(_x, _y), f);
                    if (d < out_d)
                        out_d = d;
                }
            }
        }
        float temp = max(out_d, in_d);
        if (temp == 0) {
            if (out_d == 0) 
                f->silhs[IDX] = -1;
            if (in_d == 0) 
                f->silhs[IDX] =  1;
        } else  {
            f->silhs[IDX] = (out_d - in_d) / temp;
        }
        sum_silh += f->silhs[IDX]; 
    }
    return sum_silh / flops.size();
}

/*** Min-Cost Flow ********************************************************/
void clustering::minCostFlow (const vector<pair<float, float>>& means, unsigned CAP, unsigned IDX, float DIST) {
	int remaining = flops.size() % means.size();
    
	ListDigraph g;
    // collection of nodes in the flow
    vector<ListDigraph::Node> f_nodes, c_nodes;
    //vector<vector<ListDigraph::Node>> s_nodes;
    // collection of edges in the flow
    vector<ListDigraph::Arc> i_edges, d_edges, /*g_edges,*/ o_edges;

    // source and sink
    ListDigraph::Node s = g.addNode();
    ListDigraph::Node t = g.addNode();

    // add nodes / edges to graph
    // nodes for flops
    for (unsigned i = 0; i < flops.size(); ++i) {
        ListDigraph::Node v = g.addNode();
        f_nodes.push_back(v);
    }
    // nodes for slots
//    for (unsigned i = 0; i < means.size(); ++i) {
//        vector<ListDigraph::Node> dummy;
//        for (unsigned j = 0; j < CAP; ++j) {
//            ListDigraph::Node v = g.addNode();
//            dummy.push_back(v);
//        }
//        s_nodes.push_back(dummy);
//    }
    // nodes for clusters
    for (unsigned i = 0; i < means.size(); ++i) {
        ListDigraph::Node v = g.addNode();
        c_nodes.push_back(v);
    }
    // edges between source and flops
    for (unsigned i = 0; i < flops.size(); ++i) {
        ListDigraph::Arc e = g.addArc(s, f_nodes[i]);
        i_edges.push_back(e);
    }
    // edges between flops and slots
    vector<float> costs;
    for (unsigned i = 0; i < flops.size(); ++i) {
        for (unsigned j = 0; j < means.size(); ++j) {
            float _x = means[j].first;
            float _y = means[j].second;
            //for (unsigned k = 0; k < CAP; ++k) {
//                pair<float, float> slot_loc;
//                if (TEST_LAYOUT == 1) {
//                    //slot_loc = make_pair(_x, _y);
//                    slot_loc = make_pair(_x, _y);
//                } else {
//                    slot_loc = make_pair(_x, _y);
//                }
                //float d = calcDist(slot_loc, flops[i]);
				float d = calcDist(make_pair(_x, _y), flops[i]);
                if (d > DIST)
                    continue;
                ListDigraph::Arc e = g.addArc(f_nodes[i], c_nodes[j]);
                d_edges.push_back(e);
                costs.push_back(int(d*100));
            //}
        }
    }
    // edges between slots and clusters
//    for (unsigned i = 0; i < means.size(); ++i) {
//        for (unsigned j = 0; j < CAP; ++j) {
//            ListDigraph::Arc e = g.addArc(c_nodes[i][j], c_nodes[i]);
//            g_edges.push_back(e);
//        }
//    }
    // edges between clusters and sink
    for (unsigned i = 0; i < means.size(); ++i) {
        ListDigraph::Arc e = g.addArc(c_nodes[i], t);
        o_edges.push_back(e);
    }

    if (verbose > 1)
    cout << "Graph have " << countNodes(g) << " nodes and " << countArcs(g) << " edges" << endl;

    // formulate min-cost flow
    NetworkSimplex<ListDigraph, int, int> flow(g);
    ListDigraph::ArcMap<int> f_cost(g), f_cap(g), f_sol(g);
    ListDigraph::NodeMap<pair<int,pair<int,int>>> node_map(g);
    for (unsigned i = 0; i < i_edges.size(); ++i) {
        f_cap[i_edges[i]]  = 1;
    }
    for (unsigned i = 0; i < d_edges.size(); ++i) {
        f_cap[d_edges[i]]  = 1;
        f_cost[d_edges[i]] = costs[i];
    }
//    for (unsigned i = 0; i < g_edges.size(); ++i) {
//        f_cap[g_edges[i]]  = 1;
//    }
	
	
	//cout << "CAP" << CAP << " flops size: " << flops.size() << " remaining " << remaining << "\n";
    for (unsigned i = 0; i < o_edges.size(); ++i) {
		if (i < remaining) {
			f_cap[o_edges[i]]  = CAP + 1;
		} else {
			f_cap[o_edges[i]]  = CAP;
		}
    }

    for (unsigned i = 0; i < f_nodes.size(); ++i) 
        node_map[f_nodes[i]] = make_pair(i, make_pair(-1,-1));
//    for (unsigned i = 0; i < s_nodes.size(); ++i) 
//        for (unsigned j = 0; j < CAP; ++j) 
//            node_map[s_nodes[i][j]] = make_pair(-1, make_pair(i,j));
    for (unsigned i = 0; i < c_nodes.size(); ++i) 
        node_map[c_nodes[i]] = make_pair(-1, make_pair(i,0));
    node_map[s] = make_pair(-2, make_pair(-2, -2));
    node_map[t] = make_pair(-2, make_pair(-2, -2));

    if (verbose > 1) {
		for (ListDigraph::ArcIt it(g); it != INVALID; ++it) {
			cout << g.id(g.source(it)) << "-" << g.id(g.target(it));
			cout << " cost = " << f_cost[it];
			cout << " cap = " << f_cap[it] << endl;
		}
    }

    flow.costMap(f_cost);
    flow.upperMap(f_cap);
    flow.stSupply(s, t, means.size()*CAP + remaining);
    flow.run();
    flow.flowMap(f_sol);
    //if (verbose > 0)
    //cout << "Total flow cost = " << flow.totalCost() << endl;

    for (ListDigraph::ArcIt it(g); it != INVALID; ++it) {
        if (f_sol[it] != 0) {
            if (node_map[g.source(it)].second.first == -1 
            && node_map[g.target(it)].first == -1) {
                if (verbose > 1) {
                cout << "Flow from: flop_" << node_map[g.source(it)].first;
                cout << " to cluster_" << node_map[g.target(it)].second.first;
                cout << " slot_" << node_map[g.target(it)].second.second;
                cout << " flow = " << f_sol[it] << endl;
                }
                flops[node_map[g.source(it)].first]->match_idx[IDX] = node_map[g.target(it)].second;
            }
        }
    }
    if (verbose > 1) 
    cout << endl;
}

void clustering::plotClusters(const vector<vector<flop*>>& clusters, const vector<pair<float, float>>& means) const {
	ofstream fout;
    stringstream  sol_ss;
    sol_ss << plotFile;
    fout.open(sol_ss.str());
    fout << "#! /home/kshan/anaconda2/bin/python" << endl << endl;
    fout << "import numpy as np" << endl;
    fout << "import matplotlib.pyplot as plt" << endl;
    fout << "import matplotlib.path as mpath" << endl;
    fout << "import matplotlib.lines as mlines" << endl;
    fout << "import matplotlib.patches as mpatches" << endl;
    fout << "from matplotlib.collections import PatchCollection" << endl;

    fout << endl;
    //fout << "fig, ax = plt.subplots()" << endl;
    fout << "patches = []" << endl;

	//std::array<char, 6> colors = {'b', 'r', 'g', 'm', 'c', 'y'};
	//for (size_t i = 0; i < clusters.size(); i++) {
	//	//std::cout << "clusters " << i << "\n";
	//	for (size_t j = 0; j < clusters[i].size(); j++) {
	//		fout << "plt.scatter(" << clusters[i][j]->x << ", " 
	//			<< clusters[i][j]->y << ", color=\'" << colors[i % colors.size()] << "\')\n";
	//		
	//		if (clusters[i][j]->x < means[i].first) {
	//			fout << "plt.plot([" << clusters[i][j]->x << ", "  << means[i].first << 
	//				"], [" << clusters[i][j]->y << ", " << means[i].second << 
	//				"], color = '" << colors[i % colors.size()] << "')\n";
	//		} else {
	//			fout << "plt.plot([" << means[i].first << ", "  << clusters[i][j]->x  << 
	//				"], [" << means[i].second << ", " <<  clusters[i][j]->y<< 
	//				"], color = '" << colors[i % colors.size()] << "')\n";
	//		}
	//	}
	//}

	for (size_t i = 0; i < means.size(); i++) {
		fout << "plt.scatter(" << means[i].first << ", " 
			<< means[i].second << ", color=\'k\')\n"; 	
	}

	fout << "plt.show()"; 
	fout.close();
};

void clustering::getClusters(vector<vector<unsigned>>& newClusters) {
        newClusters.clear();
        newClusters.resize(this->clusters.size());
        for (unsigned i = 0; i < clusters.size(); ++i) {
                newClusters[i].resize(clusters[i].size());
	        for(unsigned j = 0; j < clusters[i].size(); ++j) {
			newClusters[i][j] = clusters[i][j]->sinkIdx;
		} 
	}
}

}
