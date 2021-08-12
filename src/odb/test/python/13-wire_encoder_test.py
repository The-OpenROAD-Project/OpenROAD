import opendbpy as odb
import os 

current_dir = os.path.dirname(os.path.realpath(__file__))
tests_dir = os.path.abspath(os.path.join(current_dir, os.pardir))
opendb_dir = os.path.abspath(os.path.join(tests_dir, os.pardir))
data_dir = os.path.join(tests_dir, "data")

db = odb.dbDatabase.create()
odb.read_lef(db, os.path.join(data_dir, "gscl45nm.lef"))
odb.read_def(db, os.path.join(data_dir, "design.def"))
chip = db.getChip()
tech = db.getTech()

vias = tech.getVias()
via1 = vias[0]
layer1 = via1.getBottomLayer()
via2 = vias[1]
via3 = vias[2]
block = chip.getBlock()
net = odb.dbNet_create(block, "w1")
wire = odb.dbWire_create(net)
wire_encoder =  odb.dbWireEncoder()

wire_encoder.begin(wire)
wire_encoder.newPath(layer1, "ROUTED")
wire_encoder.addPoint(2000,2000)

jid1 = wire_encoder.addPoint(10000,2000)
wire_encoder.addPoint(18000,2000)
wire_encoder.newPath(jid1)
wire_encoder.addTechVia(via1)

jid2 = wire_encoder.addPoint(10000,10000)
wire_encoder.addPoint(10000,18000)
wire_encoder.newPath(jid2)

jid3=wire_encoder.addTechVia(via2)
wire_encoder.end()

result = odb.write_def(block, os.path.join(opendb_dir, "build/wire_encoder.def"))
if(result!=1):
    exit("Write DEF failed")
