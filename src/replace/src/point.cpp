#include "point.h"

namespace replace {

FloatPoint::FloatPoint() : x(0), y(0) {}
FloatPoint::FloatPoint(float inputX, float inputY) {
  x = inputX; 
  y = inputY;
}

IntPoint::IntPoint() : x(0), y(0) {}
IntPoint::IntPoint(int inputX, int inputY) {
  x = inputX;
  y = inputY;
}
}

