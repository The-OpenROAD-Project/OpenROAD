#include <vector>

#pragma once

struct Point {
    float x = 0, y = 0;
};

struct Tray {
    Point pt;
    std::vector<Point> slots;
    int cand[70];
};

struct Flop {
    Point pt;
    int idx = 0;
    float prob = 0;

    bool operator<(const Flop &a) const { return prob < a.prob; }
};

struct Path {
    int a = 0, b = 0;
};

namespace gpl {

class MBFF {

  public:
    MBFF(int num_flops, int num_paths, std::vector<float> x,
         std::vector<float> y, std::vector<std::pair<int, int> > paths,
         int threads);
    ~MBFF();

    void Run(int mx_sz, float ALPHA, float BETA);

  private:
    std::vector<Flop> FLOPS;
    std::vector<Path> PATHS;
    int NUM_THREADS;

    float GetDist(Point a, Point b);
    int GetRows(int K);

    int GetBitCnt(int bit_idx);

    std::vector<Point> GetSlots(Point tray, int rows, int cols);
    Flop GetNewFlop(std::vector<Flop> prob_dist, float tot_dist);

    std::vector<Tray> GetStartTrays(std::vector<Flop> &flops, int num_trays,
                                    float AR);
    Tray GetOneBit(Point pt);

    std::vector<std::pair<int, int> >
    MinCostFlow(std::vector<Flop> flops, std::vector<Tray> &trays, int sz);

    float GetSilh(std::vector<Flop> flops, std::vector<Tray> trays,
                  std::vector<std::pair<int, int> > clusters);

    std::vector<std::vector<Flop> > KMeans(std::vector<Flop> flops, int K);
    std::vector<std::vector<Flop> > KMeansDecomp(std::vector<Flop> flops,
                                                 int MAX_SZ);

    float RunLP(std::vector<Flop> flops, std::vector<Tray> &trays,
                std::vector<std::pair<int, int> > clusters, int sz);
    void RunILP(std::vector<Flop> flops, std::vector<Path> paths,
                std::vector<std::vector<Tray> > all_trays, float ALPHA,
                float BETA);
};
}
