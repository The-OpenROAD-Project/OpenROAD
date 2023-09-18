#include <vector>
using namespace std;

#pragma once 

struct Point {
	float x = 0, y = 0;
};

struct Tray {
	Point pt;
	vector<Point> slots;
	int cand[70];
};

struct Flop {
	Point pt;
	int idx = 0;
	float prob = 0;

	bool operator < (const Flop& a) const {
		return prob < a.prob;
	}
};


struct Path {
	int a = 0, b = 0;
};



// ORTools Usage 
namespace operations_research {
		
	float RunLP(vector<Flop> flops, vector<Tray> &trays, vector<pair<int, int> > clusters, int sz);

	namespace sat {
		void RunILP(vector<Flop> flops, vector<Path> paths, vector<vector<Tray> > all_trays, float ALPHA, float BETA);
	}

}


namespace gpl {
	float GetDist(Point a, Point b);

	class MBFF {

		public:
			MBFF(int num_flops, int num_paths, vector<float> x, vector<float> y, vector<pair<int, int> > paths, int threads); 
			~MBFF();

			void Run(int mx_sz, float ALPHA, float BETA);


		private:
			vector<Flop> FLOPS;
			vector<Path> PATHS;
			int NUM_THREADS;

			int GetRows(int K);

			vector<Point> GetSlots(Point tray, int rows, int cols);
			Flop GetNewFlop(vector<Flop> prob_dist, float tot_dist);

			vector<Tray> GetStartTrays(vector<Flop> &flops, int num_trays);
			Tray GetOneBit(Point pt);

			vector<pair<int, int> > MinCostFlow(vector<Flop> flops, vector<Tray> &trays, int sz);


			float GetSilh(vector<Flop> flops, vector<Tray> trays, vector<pair<int, int> > clusters);
			vector<vector<Flop> > KMeans(vector<Flop> flops, int K);
			vector<vector<Flop> > KMeansDecomp(vector<Flop> flops, int MAX_SZ);
	};

}





