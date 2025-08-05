#include "Instance.h"

Macro::Macro(int idx, const Rsyn::Cell& cell, const Rsyn::PhysicalDesign& physicalDesign) {
    const DBU libDBU = physicalDesign.getDatabaseUnits(Rsyn::LIBRARY_DBU);
    const Rsyn::PhysicalCell& phyCell = physicalDesign.getPhysicalCell(cell);
    const Rsyn::PhysicalLibraryCell& phyLibCell = physicalDesign.getPhysicalLibraryCell(cell);
    auto transform = phyCell.getTransform();
    
    name = phyLibCell.getMacro()->name();
    orientation = transform.getOrientation();
    
    // Macro origin
    const DBUxy origin(
        static_cast<DBU>(std::round(phyLibCell.getMacro()->originX() * libDBU)),
        static_cast<DBU>(std::round(phyLibCell.getMacro()->originY() * libDBU))
    );
    // Shift transform bound to (0, 0)
    auto bounds = transform.getBounds();
    bounds.translate(-phyCell.getPosition());
    transform.setBounds(bounds);
    
    // Get pins 
    for (Rsyn::Pin pin : cell.allPins(false)) {
        int pinIndex = -1;
        if (!pin.isPowerOrGround()) {
            pinIndex = pins.size();
            pinIndices.emplace(pin.getName(), pinIndex);
            pins.emplace_back();
        }
        
        const Rsyn::PhysicalLibraryPin& phyLibPin = physicalDesign.getPhysicalLibraryPin(pin);
        const auto& phyPinGeo = phyLibPin.allPinGeometries()[0];
        for (const Rsyn::PhysicalPinLayer& phyPinLayer : phyPinGeo.allPinLayers()) {
            if (!phyPinLayer.hasRectangleBounds()) continue;
            int layerIndex = phyPinLayer.getLayer().getRelativeIndex();
            for (auto bounds : phyPinLayer.allBounds()) {
                bounds.translate(origin);
                bounds = transform.apply(bounds);
                BoxOnLayer box(layerIndex, bounds.getLower().x, bounds.getLower().y, bounds.getUpper().x, bounds.getUpper().y);
                if (pinIndex != -1) {
                    pins[pinIndex].push_back(box);
                }
                obstacles.push_back(box);
            }
        }
    }
    
    // libObs
    for (const Rsyn::PhysicalObstacle& phyObs : phyLibCell.allObstacles()) {
        if (phyObs.getLayer().getType() != Rsyn::PhysicalLayerType::ROUTING) continue;
        const int layerIndex = phyObs.getLayer().getRelativeIndex();
        for (auto bounds : phyObs.allBounds()) {
            bounds.translate(origin);
            bounds = transform.apply(bounds);
            obstacles.emplace_back(layerIndex, bounds.getLower().x, bounds.getLower().y, bounds.getUpper().x, bounds.getUpper().y);
        }
    }
}

int Macro::getPinIndex(std::string pinName) {
    auto p = pinIndices.find(pinName);
    if (p == pinIndices.end()) {
        log() << "Error: cannot find pin " << pinName << " in macro " << name << std::endl;
        return -1;
    } else {
        return p->second;
    }
}

Instance::Instance(int idx, std::string _name, const Rsyn::PhysicalPort& phyPort): index(idx), name(_name), type(PORT) {
    auto position = phyPort.getPosition();
    auto bounds = phyPort.getBounds();
    Rsyn::PhysicalTransform transform(Bounds(position, position), phyPort.getOrientation());
    bounds.translate(position);
    bounds = transform.apply(bounds);
    port = BoxOnLayer(phyPort.getLayer().getRelativeIndex(), getBoxFromRsynBounds(bounds));
}