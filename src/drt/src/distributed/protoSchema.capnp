struct ProtoPoint {
  x @0 :Int32;
  y @1 :Int32;
}

struct ProtoRect {
  begin @0 :ProtoPoint;
  end @1 :ProtoPoint;
}

struct ProtoSegStyle {
  beginExt @0 :UInt32;
  endExt @1 :UInt32;
  width @2 :UInt32;
  beginStyle @3 :ProtoEndStyle;
  endStyle @4 :ProtoEndStyle;
  enum ProtoEndStyle {
    truncateEndStyle @0;
    extendEndStyle @1;
    variableEndStyle @2;
  }
}
struct ProtoNet {
  fake @0 :Bool;
  special @1 :Bool;
  modified @2 :Bool;
  id @3 :Int32;
}
struct ProtoUpdate {
  net @0 :ProtoNet;
  orderInOwner @1 :Int32;
  type @2 :ProtoUpdateType;
  begin @3 :ProtoPoint;
  end @4 :ProtoPoint;
  offsetBox @5 :ProtoRect;
  layerNum @6 :Int32;
  bottomConnected @7 :Bool;
  topConnected @8 :Bool;
  tapered @9 :Bool;
  viaDefId @10 :Int32;
  shapeType @11 :ProtoShapeType;
  style @12 :ProtoSegStyle;

  enum ProtoUpdateType {
    addShape @0;
    addGuide @1;
    removeFromNet @2;
    removeFromBlock @3;
  }
  enum ProtoShapeType {
    pathSeg @0;
    patchWire @1;
    via @2;
    none @3;
  }
}

@0xad172d0705279b3b;