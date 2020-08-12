#ifndef __REPLACE_COORDI__
#define __REPLACE_COORDI__

namespace replace {

class FloatPoint {
  public:
  float x;
  float y;
  FloatPoint();
  FloatPoint(float x, float y);
};

class IntPoint {
  public:
  int x;
  int y;
  IntPoint();
  IntPoint(int x, int y);
};
}

#endif
