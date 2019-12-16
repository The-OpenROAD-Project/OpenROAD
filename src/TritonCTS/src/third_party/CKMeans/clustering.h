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

#include <vector>
#include <cmath>

namespace CKMeans {
using namespace std;

class flop {
public:
    // location
    float x, y;
    unsigned x_idx, y_idx;
    vector<float> dists;
    unsigned idx;
    vector<pair<int, int>> match_idx;
    vector<float> silhs;
    unsigned sinkIdx;
    flop(const float x, const float y, unsigned idx) : x(x), y(y), sinkIdx(idx) {};
};


class clustering {
	vector<flop*> flops;
	vector<vector<flop*>> clusters;
	
	//map<unsigned, vector<pair<float, float>>> shifts;  // relative x- and y-shifts of slots w.r.t tray 
	int verbose = 1;
	int TEST_LAYOUT = 1;
	int TEST_ITER = 1;
	std::string plotFile;
	
	//std::pair<float,float> branchPoint;
	//float minDist;
	//float maxDist;
	
	float segmentLength;
	std::pair<float, float> branchingPoint;
	
public:
	clustering(const vector<std::pair<float, float>>&, float, float);
	~clustering();
	float Kmeans(unsigned, unsigned, unsigned, vector<pair<float, float>>&, unsigned);
	void iterKmeans(unsigned, unsigned, unsigned, unsigned, vector<pair<float, float>>&, unsigned MAX = 15);
	float calcSilh(const vector<pair<float,float>>&, unsigned, unsigned);
	void minCostFlow (const vector<pair<float, float>>&, unsigned, unsigned, float); 
	void setPlotFileName(const std::string fileName) { plotFile = fileName; }
	void getClusters(vector<vector<unsigned>>&);
	void fixSegmentLengths(vector<pair<float, float>>&);
	void fixSegment(const pair<float, float>& fixedPoint, pair<float, float>& movablePoint, float targetDist);

	inline float calcDist (const pair<float, float>& loc, flop * f) const {
    	return (fabs(loc.first - f->x) + fabs(loc.second - f->y));
	}
	
	inline float calcDist (const pair<float, float>& loc1, pair<float, float>& loc2) const {
    	return (fabs(loc1.first - loc2.first) + fabs(loc1.second - loc2.second));
	}

	void plotClusters(const vector<vector<flop*>>&, const vector<pair<float, float>>&) const;
};

}
