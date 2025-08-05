#pragma once
#include "global.h"
#include "GeoTypes.h"

struct PinReference {
    int instanceIndex;
    int pinIndex; // pin index inside the instance
    
    PinReference(int instIdx, int pIdx): instanceIndex(instIdx), pinIndex(pIdx) {}
};

class Net {
public:
    Net(int idx, std::string _name): index(idx), name(_name) {};
    void addPinRef(int instanceIndex, int pinIndex) {
        pinRefs.emplace_back(instanceIndex, pinIndex);
    }
    
    int getIndex() const { return index; }
    std::string getName() const { return name; }
    const vector<PinReference>& getAllPinRefs() const { return pinRefs; }
    
private:
    int index;
    std::string name;
    vector<PinReference> pinRefs;
};