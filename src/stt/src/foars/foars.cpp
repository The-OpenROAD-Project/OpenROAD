#include "stt/foars.h"

#include <queue>

#include "stt/flute.h"

namespace stt {
namespace foars {

// reference point for cross product
Point ref_;
std::vector<Point> vertices_;
std::vector<Obs> obstacles_;
std::vector<Obs> top_;
std::vector<Obs> bottom_;
std::vector<Obs> left_;
// mapping (x, y) to (ind, orientation)
std::map<std::pair<int, int>, std::pair<int, int>> inds_;
utl::Logger* logger_;

struct Graph
{
  void AddEdge(Point u, Point v);
  void AddPoint(Point pt);
  void Merge(const Graph& g, Point a, Point b);
  std::vector<Graph> SplitGraph(Point a, Point b);
  std::set<Point> BFS(Point root, Point stop);
  void DFS(Point cur, Point prev, int depth);
  void ClearVars();
  std::pair<std::pair<Point, Point>, Obs> GetEdgeAndObs();
  std::vector<Point> GetIntersectingPoints(Point pt,
                                           Obs left,
                                           Obs top,
                                           Obs bottom);
  std::vector<Point> PointOnObs(const std::vector<Point>& points);
  std::vector<Point> GetPoints(int option);
  std::map<Point, int> depths;
  std::map<Point, int> sz;
  std::map<Point, Point> pars;
  std::map<Point, std::vector<Point>> adj;
};

// obstacle-aware spanning graph
struct OASG
{
  OASG(int n, int m)
  {
    num_vertices = n;
    num_obstacles = m;
  }
  void SolveOctant(int octant);
  std::vector<Point> GetPointsInOct(Point v,
                                    const std::set<Point>& active,
                                    int octant);

  int num_vertices;
  int num_obstacles;
  Graph g;
};

// DSU
struct DSU
{
  DSU(int n)
  {
    num_vertices = n;
    par.resize(num_vertices);
    sz.resize(num_vertices);
  }
  void Init();
  int FindSet(int x);
  bool UniteSets(int a, int b);

  int num_vertices;
  std::vector<int> par;
  std::vector<int> sz;
};

// minimum terminal spanning tree
struct MTST
{
  // extended dijkstra's
  void SPT(OASG G);
  // extended kruskal's
  void MST(const OASG& G);

  int num_vertices;
  int num_obstacles;
  std::vector<std::pair<Point, Point>> edges;
  Graph g;
};

struct OPMST
{
  OPMST(int a, int b, std::vector<std::pair<Point, Point>> c, Graph d)
  {
    num_vertices = a;
    num_obstacles = b;
    edges = std::move(c);
    g = std::move(d);
  }
  void Init();

  int num_vertices;
  int num_obstacles;
  std::vector<std::pair<Point, Point>> edges;
  Graph g;
};

struct OAST
{
  OAST(Graph a)
  {
    input_opmst = std::move(a);
    inds = inds_;
    nxt_unused = static_cast<int>(vertices_.size());
  }
  void Init();
  Graph RunFLUTE(std::vector<Point>& points);
  Graph Partition(Graph T);
  Graph OA_FLUTE(std::vector<Point> points);
  Tree ConstructTree(int drvr);
  Graph input_opmst;
  Graph output_oast;
  // map (x, y) to (ind, orientation)
  std::map<std::pair<int, int>, std::pair<int, int>> inds;

  int nxt_unused;
  int tree_length;
};

struct OBTree
{
  int x_min;
  int y_min;
  int x_max;
  int y_max;
  std::vector<Obs> obs;
  OBTree* child_1;
  OBTree* child_2;
};

// OBTree "root"
OBTree overall_;

OBTree CreateOBTree(std::vector<Obs> OB, int K)
{
  OBTree new_tree;
  new_tree.obs = OB;

  if (K <= 1) {
    return new_tree;
  }

  std::vector<int> x;
  std::vector<int> y;
  for (const auto& ob : OB) {
    x.push_back(ob.bottom_left_corner.x);
    x.push_back(ob.top_right_corner.x);
    y.push_back(ob.bottom_left_corner.y);
    y.push_back(ob.top_right_corner.y);
  }
  std::sort(x.begin(), x.end());
  std::sort(y.begin(), y.end());

  new_tree.x_min = x.front();
  new_tree.x_max = x.back();
  new_tree.y_min = y.front();
  new_tree.y_max = y.back();

  if (new_tree.x_max - new_tree.x_min > new_tree.y_max - new_tree.y_min) {
    std::sort(OB.begin(), OB.end(), [](const Obs& a, const Obs& b) {
      return a.bottom_left_corner.x < b.bottom_left_corner.x;
    });
  } else {
    std::sort(OB.begin(), OB.end(), [](const Obs& a, const Obs& b) {
      return a.bottom_left_corner.y < b.bottom_left_corner.y;
    });
  }

  int H = (K + 1) / 2;
  std::vector<Obs> A;
  std::vector<Obs> B;
  for (int i = 0; i < OB.size(); i++) {
    if (i < H) {
      A.push_back(OB[i]);
    } else {
      B.push_back(OB[i]);
    }
  }
  *new_tree.child_1 = CreateOBTree(A, H);
  *new_tree.child_2 = CreateOBTree(B, K - H);

  return new_tree;
}

Obs CheckSteinerNode(Point alp, OBTree pi)
{
  Point dummy = {-1, -1, -1, -1};
  if (pi.obs.empty()) {
    return Obs(dummy, dummy);
  }

  bool bad = ((!(pi.x_min < alp.x && alp.x < pi.x_max))
              | (!(pi.y_min < alp.y && alp.y < pi.y_max)));

  if (bad) {
    return Obs(dummy, dummy);
  }

  if (pi.obs.size() == 1) {
    return pi.obs.front();
  }

  Obs from_child_1 = CheckSteinerNode(alp, *pi.child_1);
  Obs from_child_2 = CheckSteinerNode(alp, *pi.child_2);

  if (!(from_child_1 == Obs(dummy, dummy))) {
    return from_child_1;
  }
  return from_child_2;
}

int Point::GetDist(const Point& pt)
{
  return (std::max(x - pt.x, pt.x - x)) + (std::max(y - pt.y, pt.y - y));
}

std::vector<Point> Graph::GetPoints(int option)
{
  std::vector<Point> points;
  for (const auto& edges : adj) {
    if (edges.first.ind >= 0) {
      points.push_back(edges.first);
    } else {
      if (option) {
        points.push_back(edges.first);
      }
    }
  }
  return points;
}

void Graph::AddEdge(Point u, Point v)
{
  adj[u].push_back(v);
  adj[v].push_back(u);
}

// list of points that have v in octant $octant
std::vector<Point> OASG::GetPointsInOct(Point v,
                                        const std::set<Point>& active,
                                        int octant)
{
  std::vector<Point> ret;
  for (Point u : active) {
    if (u.x <= v.x) {
      if (u.y <= v.y) {
        if (v.y - u.y >= v.x - u.x) {
          if (octant == 1) {
            ret.push_back(u);
          }
        } else {
          if (octant == 2) {
            ret.push_back(u);
          }
        }
      } else {
        if (v.y - u.y >= u.x - v.x) {
          if (octant == 3) {
            ret.push_back(u);
          }
        } else {
          if (octant == 4) {
            ret.push_back(u);
          }
        }
      }
    }
  }
  return ret;
}

// get intersecting boundary for e(u, v)
Obs IntersectingObstacle(Point u,
                         Point v,
                         const std::vector<Obs>& bottom_or_top,
                         const std::vector<Obs>& left,
                         int octant)
{
  for (const Obs& obs : bottom_or_top) {
    int x_min = obs.bottom_left_corner.x;
    int x_max = obs.top_right_corner.x;
    int y_all = obs.bottom_left_corner.y;

    if (x_min < u.x && v.x < x_max) {
      if (octant <= 2) {
        if (u.y < y_all && y_all < v.y) {
          return obs;
        }
      } else {
        if (v.y < y_all && y_all < u.y) {
          return obs;
        }
      }
    }
  }

  for (const Obs& obs : left) {
    int x_all = obs.bottom_left_corner.x;
    int y_min = obs.bottom_left_corner.y;
    int y_max = obs.top_right_corner.y;

    if (u.x < x_all && x_all < v.x) {
      if (octant <= 2) {
        if (y_min < u.y && v.y < y_max) {
          return obs;
        }
      } else {
        if (y_min < v.y && u.y < y_max) {
          return obs;
        }
      }
    }
  }

  Point dummy{-1, -1, -1, -1};
  return Obs(dummy, dummy);
}

// generate OASG for octant #octant
void OASG::SolveOctant(int octant)
{
  std::vector<Point> all_points;
  all_points.reserve(vertices_.size());
  for (auto& vertex : vertices_) {
    all_points.push_back(vertex);
  }

  for (auto obstacle : obstacles_) {
    all_points.push_back(obstacle.bottom_left_corner);
    all_points.push_back(obstacle.top_right_corner);
    all_points.push_back(obstacle.GetBottomRight());
    all_points.push_back(obstacle.GetTopLeft());
  }

  if (octant <= 2) {
    std::sort(all_points.begin(),
              all_points.end(),
              [](const Point& a, const Point& b) {
                return (a.x + a.y) < (b.x + b.y);
              });
  } else {
    std::sort(all_points.begin(),
              all_points.end(),
              [](const Point& a, const Point& b) {
                return (a.x - a.y) < (b.x - b.y);
              });
  }

  std::set<Point> active_in_octant;
  std::vector<Obs> bottom_or_top_boundaries;
  std::vector<Obs> left_boundaries;

  for (auto v : all_points) {
    std::vector<Point> subset_oct_sym_i
        = GetPointsInOct(v, active_in_octant, octant);
    Point dummy{-1, -1, -1, -1};
    Point nn = dummy;
    for (Point cand : subset_oct_sym_i) {
      if (nn == dummy || (cand.GetDist(v) < nn.GetDist(v))) {
        if (IntersectingObstacle(
                cand, v, bottom_or_top_boundaries, left_boundaries, octant)
            == Obs(dummy, dummy)) {
          nn = cand;
        }
      }
    }

    active_in_octant.insert(v);

    if (!(nn == dummy)) {
      g.AddEdge(nn, v);
    }

    // important bottom left corner?
    if (octant <= 2) {
      if (v.ind < 0 && v.orientation == 1) {
        int obs_ind = -v.ind - 1;
        bottom_or_top_boundaries.emplace_back(
            v, obstacles_[obs_ind].GetBottomRight());
        left_boundaries.emplace_back(v, obstacles_[obs_ind].GetTopLeft());
      }
    }

    // important top left corner?
    else {
      if (v.ind < 0 && v.orientation == 4) {
        int obs_ind = -v.ind - 1;
        Point top_right = obstacles_[obs_ind].top_right_corner;
        bottom_or_top_boundaries.emplace_back(v, top_right);

        Point bottom_left = obstacles_[obs_ind].bottom_left_corner;
        left_boundaries.emplace_back(bottom_left, v);
      }
    }
  }
}

void DSU::Init()
{
  for (int i = 0; i < num_vertices; i++) {
    par[i] = i;
    sz[i] = 1;
  }
}

int DSU::FindSet(int x)
{
  if (par[x] == x) {
    return x;
  }
  return par[x] = FindSet(par[x]);
}

bool DSU::UniteSets(int a, int b)
{
  int x = FindSet(a);
  int y = FindSet(b);
  if (x == y) {
    return false;
  }
  if (sz[y] > sz[x]) {
    std::swap(y, x);
  }
  par[y] = x;
  sz[x] += sz[y];
  return true;
}

void MTST::SPT(OASG G)
{
  Point dummy{-1, -1, -1, -1};
  std::map<Point, Point> par;

  std::map<Point, int> dist;
  std::priority_queue<std::pair<int, Point>> pq;

  for (auto& vertex : vertices_) {
    pq.push(std::make_pair(0, vertex));
    dist[vertex] = 0;
    par[vertex] = dummy;
  }

  while (!pq.empty()) {
    std::pair<int, Point> front = pq.top();
    Point u = front.second;
    pq.pop();
    for (Point v : G.g.adj[u]) {
      if (v.ind < 0) {
        if (!par.count(v)) {
          dist[v] = dist[u] + u.GetDist(v);
          par[v] = u;
        } else if (dist[v] > dist[u] + u.GetDist(v)) {
          dist[v] = dist[u] + u.GetDist(v);
          par[v] = u;
        }
      }
    }
  }

  for (auto& obstacle : obstacles_) {
    Point p1 = obstacle.bottom_left_corner;
    if (par.count(p1)) {
      edges.emplace_back(par[p1], p1);
    }

    Point p2 = obstacle.GetBottomRight();
    if (par.count(p2)) {
      edges.emplace_back(par[p2], p2);
    }

    Point p3 = obstacle.GetTopLeft();
    if (par.count(p3)) {
      edges.emplace_back(par[p3], p3);
    }

    Point p4 = obstacle.top_right_corner;
    if (par.count(p4)) {
      edges.emplace_back(par[p4], p4);
    }
  }
}

void MTST::MST(const OASG& G)
{
  std::map<Point, int> compress;
  int cur_cnt = 0;
  for (auto& vertex : vertices_) {
    compress[vertex] = cur_cnt++;
  }

  for (auto& obstacle : obstacles_) {
    compress[obstacle.bottom_left_corner] = cur_cnt++;
    compress[obstacle.GetBottomRight()] = cur_cnt++;
    compress[obstacle.GetTopLeft()] = cur_cnt++;
    compress[obstacle.top_right_corner] = cur_cnt++;
  }

  DSU mst(cur_cnt);
  mst.Init();
  for (const auto& edge : edges) {
    mst.UniteSets(compress[edge.first], compress[edge.second]);
    g.AddEdge(edge.first, edge.second);
  }

  std::vector<std::pair<int, std::pair<Point, Point>>> todo_edges;
  for (const auto& edges : G.g.adj) {
    Point u = edges.first;
    for (Point v : edges.second) {
      int dist = u.GetDist(v);
      todo_edges.emplace_back(dist, std::make_pair(u, v));
    }
  }
  std::sort(todo_edges.begin(), todo_edges.end());

  for (const auto& edge : todo_edges) {
    Point u = edge.second.first;
    Point v = edge.second.second;
    bool added = mst.UniteSets(compress[u], compress[v]);
    if (added) {
      g.AddEdge(u, v);
    }
  }
}

void OPMST::Init()
{
  std::map<Point, Point> alias;
  for (auto& obstacle : obstacles_) {
    Point p1 = obstacle.bottom_left_corner;
    Point p2 = obstacle.GetBottomRight();
    Point p3 = obstacle.GetTopLeft();
    Point p4 = obstacle.top_right_corner;

    for (auto& vertex : vertices_) {
      if (!alias.count(p1) || p1.GetDist(vertex) < p1.GetDist(alias[p1])) {
        alias[p1] = vertex;
      }
      if (!alias.count(p2) || p2.GetDist(vertex) < p2.GetDist(alias[p2])) {
        alias[p2] = vertex;
      }
      if (!alias.count(p3) || p3.GetDist(vertex) < p3.GetDist(alias[p3])) {
        alias[p3] = vertex;
      }
      if (!alias.count(p4) || p4.GetDist(vertex) < p4.GetDist(alias[p4])) {
        alias[p4] = vertex;
      }
    }
  }

  for (auto& edges : g.adj) {
    std::vector<Point> new_adj;
    for (Point pt : edges.second) {
      if (pt.ind >= 0) {
        new_adj.push_back(pt);
      } else {
        new_adj.push_back(alias[pt]);
      }
    }

    if (edges.first.ind < 0) {
      Point new_par = alias[edges.first];
      g.adj[new_par].insert(
          g.adj[new_par].end(), new_adj.begin(), new_adj.end());
    } else {
      edges.second = new_adj;
    }
  }

  Graph new_g;
  for (auto& edges : g.adj) {
    if (edges.first.ind >= 0) {
      new_g.adj.insert(edges);
    }
  }

  for (auto& edges : new_g.adj) {
    std::vector<Point> clean_adj;
    std::set<Point> seen;
    for (Point pt : edges.second) {
      if (!seen.count(pt) && !(pt == edges.first)) {
        clean_adj.push_back(pt);
        seen.insert(pt);
      }
    }
    edges.second = clean_adj;
  }

  g = new_g;
}

std::set<Point> Graph::BFS(Point root, Point stop)
{
  std::queue<Point> q;
  q.push(root);

  std::map<Point, bool> visited;
  visited[root] = true;

  while (!q.empty()) {
    Point u = q.front();
    q.pop();

    for (Point v : adj[u]) {
      if (visited.find(v) != visited.end() || v == stop) {
        continue;
      }
      q.push(v);
      visited[v] = true;
    }
  }

  std::set<Point> ret;
  for (auto p : visited) {
    ret.insert(p.first);
  }
  return ret;
}

// split adj by e(u, v)
std::vector<Graph> Graph::SplitGraph(Point a, Point b)
{
  Graph t1;
  Graph t2;

  std::set<Point> visited = BFS(a, b);
  for (auto& edges : adj) {
    Point u = edges.first;
    if (visited.find(u) != visited.end()) {
      t1.adj[u] = edges.second;
    } else {
      t2.adj[u] = edges.second;
    }
  }

  for (const auto& edges : t1.adj) {
    Point u = edges.first;
    std::vector<Point> edge;
    for (Point v : edges.second) {
      if (!(v == b)) {
        edge.push_back(v);
      }
    }
    t1.adj[u] = edge;
  }

  for (const auto& edges : t2.adj) {
    Point u = edges.first;
    std::vector<Point> edge;
    for (Point v : edges.second) {
      if (!(v == a)) {
        edge.push_back(v);
      }
    }
    t2.adj[u] = edge;
  }

  std::vector<Graph> ret = {t1, t2};
  return ret;
}

std::pair<std::pair<Point, Point>, Obs> Graph::GetEdgeAndObs()
{
  Point dummy = {-1, -1, -1, -1};
  for (auto& edges : adj) {
    Point u = edges.first;
    for (Point v : edges.second) {
      if (u.x <= v.x) {
        Obs blocking = IntersectingObstacle(
            u, v, (u.y <= v.y ? bottom_ : top_), left_, (u.y <= v.y ? 2 : 4));
        if (!(blocking == Obs(dummy, dummy))) {
          return {{u, v}, blocking};
        }
      }
    }
  }
  return {{dummy, dummy}, Obs(dummy, dummy)};
}

std::pair<int, int> Point::GetSlope(const Point& pt)
{
  int d_x = x - pt.x;
  int d_y = y - pt.y;
  return std::make_pair(d_x, d_y);
}

int Point::Cross(const Point& b) const
{
  return x * b.y - y * b.x;
}

bool CrossProductSorter(const Point& a, const Point& b)
{
  int c_1 = a.Cross(ref_);
  int c_2 = b.Cross(ref_);
  return (c_1 - c_2 != 0 ? c_1 > c_2 : ref_.GetDist(a) < ref_.GetDist(b));
}

std::vector<Point> Graph::GetIntersectingPoints(Point pt,
                                                Obs left,
                                                Obs top,
                                                Obs bottom)
{
  // ret = optimal travel path in ccw order
  std::vector<Point> ret;

  Point r_1 = {bottom.top_right_corner.x, bottom.top_right_corner.y, -1, -1};
  Point r_2 = {top.top_right_corner.x, top.top_right_corner.y, -1, -1};
  Obs right = Obs(r_1, r_2);

  // map intersection point to v in e(u, v)
  std::map<Point, Point> mapping;

  for (Point v : adj[pt]) {
    std::pair<int, int> m_1 = pt.GetSlope(v);

    // vertical line
    std::vector<Obs> all_obs;
    if (!m_1.first) {
      all_obs = {top, bottom};
      for (int i = 0; i < 2; i++) {
        if (pt.y <= v.y) {
          if (pt.y <= all_obs[i].bottom_left_corner.y
              && all_obs[i].bottom_left_corner.y <= v.y) {
            Point npt = {pt.x, all_obs[i].bottom_left_corner.y, 2, -1};
            ret.push_back(npt);
            mapping[npt] = v;
          }
        }

        else {
          if (v.y <= all_obs[i].bottom_left_corner.y
              && all_obs[i].bottom_left_corner.y <= pt.y) {
            Point npt = {pt.x, all_obs[i].bottom_left_corner.y, 2, -1};
            ret.push_back(npt);
            mapping[npt] = v;
          }
        }
      }
    }

    // horizontal line
    else if (!m_1.second) {
      all_obs = {left, right};
      for (int i = 0; i < 2; i++) {
        Point npt;
        if (pt.x <= v.x) {
          if (pt.x <= all_obs[i].bottom_left_corner.x
              && all_obs[i].bottom_left_corner.x <= v.x) {
            npt = {all_obs[i].bottom_left_corner.x, pt.y, 2, -1};
            ret.push_back(npt);
            mapping[npt] = v;
          }
        }

        else {
          if (v.x <= all_obs[i].bottom_left_corner.x
              && all_obs[i].bottom_left_corner.x <= pt.x) {
            npt = {all_obs[i].bottom_left_corner.x, pt.y, 2, -1};
            ret.push_back(npt);
            mapping[npt] = v;
          }
        }
      }
    }

    // good case
    else {
      all_obs = {left, right, top, bottom};
      int m = int(100 * (float(m_1.second) / float(m_1.first)));
      for (int i = 0; i < 3; i++) {
        Point npt;
        if (i == 0 || i == 1) {
          int y = m * (all_obs[i].bottom_left_corner.x - pt.x) + pt.y;
          if (all_obs[i].bottom_left_corner.y <= y
              && y <= all_obs[i].top_right_corner.y) {
            if (std::min(pt.x, v.x) <= all_obs[i].bottom_left_corner.x
                && all_obs[i].bottom_left_corner.x <= std::max(pt.x, v.x)) {
              npt = {all_obs[i].bottom_left_corner.x, y, 2, -1};
              ret.push_back(npt);
              mapping[npt] = v;
            }
          }
        }

        else {
          // scale new_x by m to avoid decimals
          int new_x = m * pt.x + (all_obs[i].bottom_left_corner.y - pt.y);
          if ((m * all_obs[i].bottom_left_corner.x <= new_x)
              && (new_x <= m * all_obs[i].top_right_corner.x)) {
            if (m * std::min(pt.x, v.x) <= new_x
                && new_x <= m * std::max(pt.x, v.x)) {
              npt = {(new_x / m), all_obs[i].bottom_left_corner.y, 2, -1};
              ret.push_back(npt);
              mapping[npt] = v;
            }
          }
        }
      }
    }
  }
  ret.push_back(bottom.bottom_left_corner);
  ret.push_back(bottom.top_right_corner);
  ret.push_back(top.top_right_corner);
  ret.push_back(top.bottom_left_corner);

  ref_ = pt;
  std::sort(ret.begin(), ret.end(), CrossProductSorter);

  Point dummy = {-1, -1, -1, -1};
  Point prev = dummy;
  std::vector<Point> longest_edge = {dummy, dummy};
  int max_dist = 0;
  int stop_idx = -1;

  for (size_t i = 0; i < ret.size(); i++) {
    Point p = ret[i];
    if (p.ind >= 0) {
      if (!(prev == dummy)) {
        int cur_dist = prev.GetDist(p);
        if (cur_dist > max_dist) {
          max_dist = cur_dist;
          longest_edge = {prev, p};
          stop_idx = static_cast<int>(i);
        }
      }
      prev = p;
    }
  }

  // #intersection > 1
  if (max_dist > 0) {
    std::vector<Point> tmp_ret;
    for (size_t i = stop_idx; i < ret.size(); i++) {
      tmp_ret.push_back(ret[i]);
    }
    for (size_t i = 0; i < stop_idx; i++) {
      tmp_ret.push_back(ret[i]);
      if (ret[i] == longest_edge[0]) {
        break;
      }
    }
    ret = tmp_ret;
  }

  else {
    ret = {prev};
  }

  // convert a_i back to n_i
  for (auto& p : ret) {
    if (mapping.find(p) != mapping.end()) {
      p = mapping[p];
    }
  }
  return ret;
}

std::vector<Point> Graph::PointOnObs(const std::vector<Point>& points)
{
  /* OBTree Solution */
  Point dummy = {-1, -1, -1, -1};
  std::vector<Point> ret;
  for (auto pt : points) {
    Obs blocking_obs = CheckSteinerNode(pt, overall_);
    if (!(blocking_obs == Obs(dummy, dummy))) {
      ret.push_back(pt);
      int obs_idx = -blocking_obs.bottom_left_corner.ind - 1;
      std::vector<Point> intersection_points = GetIntersectingPoints(
          pt, left_[obs_idx], top_[obs_idx], bottom_[obs_idx]);
      for (Point p : intersection_points) {
        ret.push_back(p);
      }
      return ret;
    }
  }
  return ret;
}

void Graph::Merge(const Graph& g, Point a, Point b)
{
  for (const auto& edges : g.adj) {
    Point u = edges.first;
    for (Point v : edges.second) {
      adj[u].push_back(v);
    }
  }
  AddEdge(a, b);
  ClearVars();
}

void Graph::ClearVars()
{
  pars.clear();
  depths.clear();
  sz.clear();
}

Graph OAST::RunFLUTE(std::vector<Point>& points)
{
  std::vector<int> x;
  std::vector<int> y;
  for (Point& pt : points) {
    x.push_back(pt.x);
    y.push_back(pt.y);
  }

  Graph g;
  stt::Tree flutetree = flt::flute(x, y, FLUTE_ACCURACY);
  for (int i = 0; i < 2 * flutetree.deg - 2; i++) {
    int par_idx = flutetree.branch[i].n;

    std::pair<int, int> par_coor = std::make_pair(flutetree.branch[par_idx].x,
                                                  flutetree.branch[par_idx].y);
    if (inds.find(par_coor) == inds.end()) {
      // steiner point
      inds[par_coor] = std::make_pair(nxt_unused++, 0);
    }

    std::pair<int, int> cur_coor
        = std::make_pair(flutetree.branch[i].x, flutetree.branch[i].y);
    if (inds.find(cur_coor) == inds.end()) {
      // steiner point
      inds[cur_coor] = std::make_pair(nxt_unused++, 0);
    }

    Point par = {par_coor.first,
                 par_coor.second,
                 inds[par_coor].first,
                 inds[par_coor].second};
    Point cur = {cur_coor.first,
                 cur_coor.second,
                 inds[cur_coor].first,
                 inds[cur_coor].second};
    if (!(par == cur)) {
      g.AddEdge(par, cur);
    }
  }
  return g;
}

Graph OAST::OA_FLUTE(std::vector<Point> points)
{
  Graph new_g = RunFLUTE(points);
  Graph ret;

  Point dummy = {-1, -1, -1, -1};

  bool blocked = false;
  std::pair<std::pair<Point, Point>, Obs> blocking_edge = new_g.GetEdgeAndObs();
  if (!(blocking_edge.second == Obs(dummy, dummy))) {
    Point u = blocking_edge.first.first;
    Point v = blocking_edge.first.second;
    Obs blocking_obs = blocking_edge.second;

    std::vector<Graph> new_graphs = new_g.SplitGraph(u, v);

    // create / refine T1
    new_graphs[0].AddEdge(blocking_obs.bottom_left_corner, u);
    std::vector<Point> n1 = new_graphs[0].GetPoints(1);
    Graph t1;
    if (n1.size() != 1) {
      t1 = OA_FLUTE(n1);
    } else {
      t1.adj[n1.front()] = {};
    }

    // create / refine T2
    new_graphs[1].AddEdge(blocking_obs.top_right_corner, v);
    std::vector<Point> n2 = new_graphs[1].GetPoints(1);
    Graph t2;
    if (n2.size() != 1) {
      t2 = OA_FLUTE(n2);
    } else {
      t2.adj[n2.front()] = {};
    }

    t1.Merge(
        t2, blocking_obs.bottom_left_corner, blocking_obs.top_right_corner);
    ret = t1;
    blocked = true;
  }

  bool steiner_point_on_obs = false;
  std::vector<Point> blocking_points_order = new_g.PointOnObs(points);
  if (!blocked && !blocking_points_order.empty()) {
    Point point_on_obs = blocking_points_order.front();

    std::vector<std::vector<Point>> subset_n;
    std::vector<Point> b4;
    for (size_t i = 1; i < blocking_points_order.size(); i++) {
      // corner point
      Point v = blocking_points_order[i];

      if (v.ind < 0) {
        b4.push_back(v);
      }

      else {
        // break edge and add points
        std::vector<Graph> break_graphs = new_g.SplitGraph(v, point_on_obs);
        subset_n.push_back(break_graphs[0].GetPoints(1));

        for (Point pt : b4) {
          if (std::find(subset_n[i - 1].begin(), subset_n[i - 1].end(), pt)
              == subset_n[i - 1].end()) {
            subset_n[i - 1].push_back(pt);
          }

          if (i > 1) {
            if (std::find(subset_n[i - 2].begin(), subset_n[i - 2].end(), pt)
                == subset_n[i - 2].end()) {
              subset_n[i - 2].push_back(pt);
            }
          }
        }
        b4.clear();
      }
    }

    for (auto& n_i : subset_n) {
      std::vector<Point> new_n_i;
      new_n_i.reserve(n_i.size());
      for (Point pt : n_i) {
        new_n_i.push_back(pt);
      }
      n_i = new_n_i;
    }

    Graph merged_g;
    for (auto& n_i : subset_n) {
      Graph tmp_g = OA_FLUTE(n_i);
      for (auto& edges : tmp_g.adj) {
        if (merged_g.adj.find(edges.first) != merged_g.adj.end()) {
          std::copy(tmp_g.adj[edges.first].begin(),
                    tmp_g.adj[edges.first].end(),
                    merged_g.adj[edges.first].end());
        } else {
          merged_g.adj[edges.first] = tmp_g.adj[edges.first];
        }
      }
    }
    ret = merged_g;
    steiner_point_on_obs = true;
  }

  if (!(blocked | steiner_point_on_obs)) {
    ret = new_g;
  }

  return ret;
}

Graph OAST::Partition(Graph oast)
{
  Point dummy = {-1, -1, -1, -1};
  Graph ret;

  bool blocked = false;
  std::pair<std::pair<Point, Point>, Obs> blocking_edge = oast.GetEdgeAndObs();
  if (!(blocking_edge.second == Obs(dummy, dummy))) {
    Point u = blocking_edge.first.first;
    Point v = blocking_edge.first.second;
    Obs blocking_obs = blocking_edge.second;

    std::vector<Graph> new_adjs = oast.SplitGraph(u, v);

    // create / refine T1
    Graph t1 = new_adjs[0];
    t1.AddEdge(u, blocking_obs.bottom_left_corner);
    Graph nt1 = Partition(t1);

    // create / refine T2
    Graph t2 = new_adjs[1];
    t2.AddEdge(v, blocking_obs.top_right_corner);
    Graph nt2 = Partition(t2);

    /* edges containing corner points may be destroyed during recursive calls;
     * add them back */
    if (nt1.adj.find(blocking_obs.bottom_left_corner) == nt1.adj.end()) {
      nt1.AddEdge(u, blocking_obs.bottom_left_corner);
    }

    if (nt2.adj.find(blocking_obs.top_right_corner) == nt2.adj.end()) {
      nt2.AddEdge(v, blocking_obs.top_right_corner);
    }

    nt1.Merge(
        nt2, blocking_obs.bottom_left_corner, blocking_obs.top_right_corner);
    ret = nt1;
    blocked = true;
  }

  bool refined = false;
  // arbitrarily root the tree to figure out subtree sizes
  oast.ClearVars();
  oast.DFS(oast.adj.begin()->first, dummy, 0);
  if (!blocked && static_cast<int>(oast.adj.size()) > 20) {
    std::pair<Point, Point> longest_edge = {dummy, dummy};

    int max_dist = 0;
    for (auto& edges : oast.adj) {
      Point u = edges.first;
      for (Point v : edges.second) {
        if (u.GetDist(v) > max_dist) {
          int subtree_sz_1 = std::min(oast.sz[u], oast.sz[v]);
          int subtree_sz_2 = oast.sz[oast.adj.begin()->first] - subtree_sz_1;
          if (subtree_sz_1 >= 2 && subtree_sz_2 >= 2) {
            max_dist = u.GetDist(v);
            longest_edge = {u, v};
          }
        }
      }
    }

    if (max_dist > 0) {
      std::vector<Graph> new_adjs
          = oast.SplitGraph(longest_edge.first, longest_edge.second);

      Graph t1 = new_adjs[0];
      Graph t2 = new_adjs[1];

      Graph nt1 = Partition(t1);
      nt1.Merge(Partition(t2), longest_edge.first, longest_edge.second);
      ret = nt1;
      refined = true;
    }
  }

  if (!(blocked | refined)) {
    std::vector<Point> n = oast.GetPoints(1);
    ret = (static_cast<int>(n.size()) > 1 ? OA_FLUTE(n) : oast);
  }

  return ret;
}

void OAST::Init()
{
  output_oast = Partition(input_opmst);
}

void Graph::DFS(Point cur, Point prev, int depth)
{
  pars[cur] = prev;
  depths[cur] = depth;
  sz[cur] = 1;
  for (Point nxt : adj[cur]) {
    if (!(nxt == prev)) {
      DFS(nxt, cur, depth + 1);
      sz[cur] += sz[nxt];
    }
  }
}

Tree OAST::ConstructTree(int drvr)
{
  Point dummy = {-1, -1, -1, -1};
  output_oast.ClearVars();

  // root tree at drvr index
  output_oast.DFS(vertices_[drvr], dummy, 0);

  // sort in terms of tree depth
  int wl = 0;
  std::vector<std::pair<int, Point>> order;
  for (const auto& info : output_oast.depths) {
    order.emplace_back(info.second, info.first);
    if (!(info.first == vertices_[drvr])) {
      wl += output_oast.pars[info.first].GetDist(info.first);
    }
  }
  // sort by depth to ensure every par has been seen
  std::sort(order.begin(), order.end());

  int num_points = static_cast<int>(order.size());
  Tree tree;
  tree.deg = num_points;
  tree.branch.resize(num_points);
  int cur_ind = 0;
  std::map<Point, int> new_inds;
  for (const auto& info : order) {
    Point cur = info.second;
    new_inds[cur] = cur_ind;
    tree.branch[cur_ind]
        = {cur.x, cur.y, (cur_ind != 0 ? new_inds[output_oast.pars[cur]] : 0)};
    cur_ind++;
  }
  tree.length = wl;
  return tree;
}

Tree RunFOARS(const std::vector<int>& x_pin,
              const std::vector<int>& y_pin,
              const std::vector<std::pair<int, int>>& x_obstacle,
              const std::vector<std::pair<int, int>>& y_obstacle,
              int drvr,
              utl::Logger* logger)
{
  int num_vertices = static_cast<int>(x_pin.size());
  for (int i = 0; i < num_vertices; i++) {
    int x = x_pin[i];
    int y = y_pin[i];
    vertices_.push_back(Point{x, y, i, 0});
    inds_[std::make_pair(x, y)] = std::make_pair(i, 0);
  }

  int num_obstacles = static_cast<int>(x_obstacle.size());
  for (int i = 1; i <= num_obstacles; i++) {
    int x_min = x_obstacle[i - 1].first;
    int x_max = x_obstacle[i - 1].second;

    int y_min = y_obstacle[i - 1].first;
    int y_max = y_obstacle[i - 1].second;

    Point bottom_left_corner{x_min, y_min, -i, 1};
    Point top_right_corner{x_max, y_max, -i, 3};

    inds_[std::make_pair(x_min, y_min)] = std::make_pair(-i, 1);
    inds_[std::make_pair(x_max, y_min)] = std::make_pair(-i, 2);
    inds_[std::make_pair(x_max, y_max)] = std::make_pair(-i, 3);
    inds_[std::make_pair(x_min, y_max)] = std::make_pair(-i, 4);

    obstacles_.emplace_back(bottom_left_corner, top_right_corner);
    top_.emplace_back(obstacles_[i - 1].GetTopLeft(),
                      obstacles_[i - 1].top_right_corner);
    bottom_.emplace_back(obstacles_[i - 1].bottom_left_corner,
                         obstacles_[i - 1].GetBottomRight());
    left_.emplace_back(obstacles_[i - 1].bottom_left_corner,
                       obstacles_[i - 1].GetTopLeft());
  }
  overall_ = CreateOBTree(obstacles_, num_obstacles);

  // create oasg
  OASG oasg(num_vertices, num_obstacles);
  for (int i = 1; i <= 4; i++) {
    oasg.SolveOctant(i);
  }

  // create mtst
  MTST mtst;
  mtst.SPT(oasg);
  mtst.MST(oasg);

  // create opmst
  OPMST opmst(num_vertices, num_obstacles, mtst.edges, mtst.g);
  opmst.Init();

  // create oast
  OAST oast(opmst.g);
  oast.Init();

  // return steiner tree
  return oast.ConstructTree(drvr);
}

}  // namespace foars

}  // namespace stt