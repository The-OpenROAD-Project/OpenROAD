#include "graph.h"


namespace MacroPlace {

////////////////////////////////////////////////
// Edge

Edge::Edge()
  : from_(nullptr), to_(nullptr), weight_(0) {}

Edge::Edge(Vertex* from, Vertex* to, int weight)
  : from_(from), to_(to), weight_(weight) {}

Vertex* 
Edge::from() const {
  return from_;
}

Vertex*
Edge::to() const {
  return to_;
}

int
Edge::weight() const {
  return weight_;
}

////////////////////////////////////////////////
// Vertex 

Vertex::Vertex()
  : vertexType_(VertexType::Uninitialized), ptr_(nullptr) {}

Vertex::Vertex(PinGroup* pinGroup)
  : Vertex() {
  setPinGroup(pinGroup);
  vertexType_ = VertexType::PinGroupType;
}

Vertex::Vertex(sta::Instance* inst, VertexType vertexType)
  : Vertex() {
  setInstance(inst);
  vertexType_ = vertexType;
}

// will be removed soon
Vertex::Vertex(void* ptr, VertexType vertexType)
  : Vertex() {
  ptr_ = ptr;
  vertexType_ = vertexType;
}

void* 
Vertex::ptr() const {
  return ptr_;
}

VertexType
Vertex::vertexType() const {
  return vertexType_;
}

const std::vector<int> &
Vertex::from() {
  return from_;
}

const std::vector<int> &
Vertex::to() {
  return to_;
}


PinGroup* 
Vertex::pinGroup() const {
  if( isPinGroup() ) {
    return static_cast<PinGroup*>(ptr_);
  }
  return nullptr; 
}

sta::Instance*
Vertex::instance() const {
  if( isInstance() ) {
    return static_cast<sta::Instance*>(ptr_);
  }
  return nullptr;
}

bool
Vertex::isPinGroup() const {
  switch(vertexType_) {
    case VertexType::PinGroupType:
      return true;
      break;
    default: 
      return false;
  }
  return false;
}

bool
Vertex::isInstance() const {
  switch(vertexType_) {
    case VertexType::MacroInstType:
    case VertexType::OtherInstType:
      return true;
      break;
    default: 
      return false;
  }
  return false;
}

void
Vertex::setPinGroup(PinGroup* pinGroup) {
  ptr_ = static_cast<void*>(pinGroup);
}

void
Vertex::setInstance(sta::Instance* inst) {
  ptr_ = static_cast<void*>(inst); 
}

////////////////////////////////////////////////
// PinGroup 

PinGroupLocation
PinGroup::pinGroupLocation() const {
  return pinGroupLocation_;
}

const std::vector<sta::Pin*>& 
PinGroup::pins() {
  return pins_;
}

const std::string
PinGroup::name() const {
  if( pinGroupLocation_ == PinGroupLocation::West ) {
    return "West";
  }
  else if( pinGroupLocation_ == PinGroupLocation::East ) {
    return "East";
  } 
  else if( pinGroupLocation_ == PinGroupLocation::North) {
    return "North";
  } 
  else if( pinGroupLocation_ == PinGroupLocation::South) {
    return "South";
  }
  return ""; 
}

void 
PinGroup::setPinGroupLocation(PinGroupLocation pgl) {
  pinGroupLocation_ = pgl; 
}

void
PinGroup::addPin(sta::Pin* pin) {
  pins_.push_back(pin);
}

}

