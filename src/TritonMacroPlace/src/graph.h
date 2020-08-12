#ifndef __MACRO_NETLIST_GRAPH__
#define __MACRO_NETLIST_GRAPH__ 

#include <iostream>
#include <string>
#include <vector>

namespace sta { 
class Pin; 
class Instance;
}

namespace MacroPlace{

class MacroCircuit;
class Vertex;

class Edge {
  public:
    Vertex* from() const;
    Vertex* to() const;
    int weight() const;
    
    Edge();
    Edge(Vertex* from, Vertex* to, int weight);

  private:
    Vertex* from_;
    Vertex* to_;
    int weight_;

};

class PinGroup;

// pin, instMacro, instOther, nonInit
enum VertexType {
  PinGroupType, MacroInstType, OtherInstType, Uninitialized
};

class Vertex {
  public: 
    Vertex();

    // PinGroup is input then VertexType must be PinGroupType
    Vertex(PinGroup* pinGroup);

    // need to specify between macro or other
    Vertex(sta::Instance* inst, VertexType vertexType);

    // Will be removed soon
    Vertex(void* ptr, VertexType vertexType);
    void* ptr() const;

    VertexType vertexType() const;
    const std::vector<int>& from();
    const std::vector<int>& to();

    PinGroup* pinGroup() const;
    sta::Instance* instance() const;

    bool isPinGroup() const;
    bool isInstance() const;

    void setPinGroup(PinGroup* pinGroup);
    void setInstance(sta::Instance* inst);


  private:
    VertexType vertexType_;
    std::vector<int> from_;
    std::vector<int> to_;

    // This can be either PinGroup / sta::Instance*,
    // based on vertexType
    void* ptr_;
};

enum PinGroupLocation {
  West, East, North, South
};

class PinGroup {
  public:
    PinGroupLocation pinGroupLocation() const;
    const std::vector<sta::Pin*>& pins();
    const std::string name() const;

    void setPinGroupLocation(PinGroupLocation);
    void addPin(sta::Pin* pin);

  private:
    PinGroupLocation pinGroupLocation_;
    std::vector<sta::Pin*> pins_;
};


}

#endif
