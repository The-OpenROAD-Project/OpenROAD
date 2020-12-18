// This is a very basic polygon API for tcl.  In C++ you should
// directly use the Boost Polygon classes.

%{
#include <boost/polygon/polygon.hpp>
#include <array>

using namespace boost::polygon::operators;

using Rectangle = boost::polygon::rectangle_data<int>;
using Polygon90 = boost::polygon::polygon_90_with_holes_data<int>;
using Polygon90Set = boost::polygon::polygon_90_set_data<int>;

Polygon90Set* newSetFromRect(int xLo, int yLo, int xHi, int yHi)
{
  using Pt = Polygon90::point_type;
  std::array<Pt, 4> pts
    = {Pt(xLo, yLo), Pt(xHi, yLo), Pt(xHi, yHi), Pt(xLo, yHi)};

  Polygon90 poly;
  poly.set(pts.begin(), pts.end());

  std::array<Polygon90, 1> arr{poly};
  return new Polygon90Set(boost::polygon::HORIZONTAL, arr.begin(), arr.end());
}
  
Polygon90Set* bloatSet(const Polygon90Set* set, int bloating)
{
  return new Polygon90Set(*set + bloating);
}

Polygon90Set* bloatSet(const Polygon90Set* set, int bloatX, int bloatY)
{
  Polygon90Set* result = new Polygon90Set(*set);
  bloat(*result, bloatX, bloatX, bloatY, bloatY);
  return result;
}

Polygon90Set* shrinkSet(const Polygon90Set* set, int shrinking)
{
  return new Polygon90Set(*set - shrinking);
}

Polygon90Set* shrinkSet(const Polygon90Set* set, int shrinkX, int shrinkY)
{
  Polygon90Set* result = new Polygon90Set(*set);
  shrink(*result, shrinkX, shrinkX, shrinkY, shrinkY);
  return result;
}

Polygon90Set* andSet(const Polygon90Set* set1, const Polygon90Set* set2)
{
  return new Polygon90Set(*set1 & *set2);
}

Polygon90Set* orSet(const Polygon90Set* set1, const Polygon90Set* set2)
{
  return new Polygon90Set(*set1 | *set2);
}

Polygon90Set* orSets(const std::vector<Polygon90Set>& sets)
{
  Polygon90Set* result = new Polygon90Set;
  for (const Polygon90Set& poly_set : sets) {
    *result |= poly_set;
  }
  return result;
}

Polygon90Set* subtractSet(const Polygon90Set* set1, const Polygon90Set* set2)
{
  return new Polygon90Set(*set1 - *set2);
}

std::vector<Polygon90> getPolygons(const Polygon90Set* set)
{
  std::vector<Polygon90> s;
  set->get(s);
  return s;
}

std::vector<odb::Rect> getRectangles(const Polygon90Set* set)
{
  std::vector<Rectangle> rects;
  set->get_rectangles(rects);

  // Convert from Boost rect to OpenDB rect
  std::vector<odb::Rect> result;
  result.reserve(rects.size());
  for (auto& r : rects) {
    result.emplace_back(xl(r), yl(r), xh(r), yh(r));
  }
  return result;
}

std::vector<Point> getPoints(const Polygon90* polygon)
{
  std::vector<Point> pts;
  for (auto& pt : *polygon) {
    pts.emplace_back(Point(pt.x(), pt.y()));
  }
  return pts;
}
%}

%template(Points) std::vector<odb::Point>;
%template(Rects) std::vector<odb::Rect>;
%template(Polygon90Set) std::vector<Polygon90>;
%template(Polygon90Sets) std::vector<Polygon90Set>;

// Simple constructor
%newobject newSetFromRect;
Polygon90Set* newSetFromRect(int xLo, int yLo, int xHi, int yHi);

// Query methods - return vectors for easy swig'ing
std::vector<odb::Point> getPoints(const Polygon90* polygon);
std::vector<Polygon90> getPolygons(const Polygon90Set* set);
std::vector<odb::Rect> getRectangles(const Polygon90Set* set);


%newobject bloatSet;
Polygon90Set* bloatSet(Polygon90Set* set, int bloating);

%newobject bloatSet;
Polygon90Set* bloatSet(Polygon90Set* set, int bloatX, int bloatY);

%newobject shrinkSet;
Polygon90Set* shrinkSet(Polygon90Set* set, int shrinking);

%newobject shrinkSet;
Polygon90Set* shrinkSet(Polygon90Set* set, int shrinkX, int shrinkY);

%newobject andSet;
Polygon90Set* andSet(Polygon90Set* set1, const Polygon90Set* set2);

%newobject orSet;
Polygon90Set* orSet(Polygon90Set* set1, const Polygon90Set* set2);

// It makese no sense to me that we have a vector by value and not
// pointer but swig seems to automatically add the pointer in the
// generated wrapper.
%newobject orSets;
Polygon90Set* orSets(const std::vector<Polygon90Set>& sets);

%newobject subtractSet;
Polygon90Set* subtractSet(Polygon90Set* set1, const Polygon90Set* set2);
