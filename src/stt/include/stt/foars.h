#include <map>
#include <set>
#include <utility>
#include <vector>

#include "SteinerTreeBuilder.h"

namespace stt {
namespace foars {

struct Point
{
  int x;
  int y;
  int ind;
  int orientation;

  bool operator<(const Point& a) const
  {
    if (ind == a.ind) {
      return orientation < a.orientation;
    }
    return ind < a.ind;
  }
  bool operator==(const Point& a) const
  {
    return (ind == a.ind && orientation == a.orientation);
  }
  int GetDist(const Point& pt);
  int Cross(const Point& b) const;
  std::pair<int, int> GetSlope(const Point& pt);
};

struct Obs
{
  Obs(Point a, Point b)
  {
    bottom_left_corner = a;
    top_right_corner = b;
  }

  bool operator==(const Obs& a) const
  {
    return (bottom_left_corner == a.bottom_left_corner
            && top_right_corner == a.top_right_corner);
  }
  // orientation follows ccw order with 1-based indexing
  Point GetBottomRight() const
  {
    return Point{
        top_right_corner.x, bottom_left_corner.y, bottom_left_corner.ind, 2};
  }
  Point GetTopLeft() const
  {
    return Point{
        bottom_left_corner.x, top_right_corner.y, bottom_left_corner.ind, 4};
  }

  Point bottom_left_corner;
  Point top_right_corner;
};

struct Graph;
struct OASG;
struct DSU;
struct MTST;
struct OPMST;
struct OAST;
struct OBTree;

Tree RunFOARS(const std::vector<int>& x_pin,
              const std::vector<int>& y_pin,
              const std::vector<std::pair<int, int>>& x_obstacle,
              const std::vector<std::pair<int, int>>& y_obstacle,
              int drvr,
              utl::Logger* logger);

}  // namespace foars

}  // namespace stt