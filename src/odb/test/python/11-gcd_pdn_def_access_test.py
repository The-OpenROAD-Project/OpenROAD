import opendbpy as odb
import os 

current_dir = os.path.dirname(os.path.realpath(__file__))
tests_dir = os.path.abspath(os.path.join(current_dir, os.pardir))
opendb_dir = os.path.abspath(os.path.join(tests_dir, os.pardir))
data_dir = os.path.join(tests_dir, "data")

db = odb.dbDatabase.create()
lib = odb.read_lef(db, os.path.join(data_dir, "Nangate45/NangateOpenCellLibrary.mod.lef"))
odb.read_def(db, os.path.join(data_dir, "gcd/gcd_pdn.def"))
chip = db.getChip()
block = chip.getBlock()
nets = block.getNets()
tech = db.getTech()

assert block.getName() == "gcd", "Block name mismatch"
units = block.getDefUnits()
assert units == 2000, "DEF units mismatch"

assert len(block.getChildren()) == 0, "Number of children mismatch"
assert len(block.getInsts()) == 0, "Number of instsances mismatch"
assert len(block.getBTerms()) == 0, "Number of B terms mismatch"
assert len(block.getObstructions())== 0, "Number of obstructions mismatch"
assert len(block.getBlockages()) == 0, "NUmber of blockages mismatch"
assert len(block.getNets()) == 2, "Number of nets mismatch"
assert len(block.getVias()) == 6, "Number of vias mismatch"
assert len(block.getRows()) == 112, "Number of rows mismatch"
bbox = block.getBBox()
assert bbox.xMin() == 20140, "Bbox xMin mismatch" 
assert bbox.yMin() == 22230, "Bbox xMan mismatch" 
assert bbox.xMax() == 180120, "Bbox yMin mismatch" 
assert bbox.yMax() == 179370, "Bbox yMax mismatch"
assert block.getGCellGrid() == None, "G cell grid"
die_area = block.getDieArea()
assert [die_area.xMin(), die_area.yMin(), die_area.xMax(), die_area.yMax()] == [0, 0, 200260, 201600]
assert len(block.getRegions()) == 0, "Number of regions mismatch"
assert len(block.getNonDefaultRules()) == 0, "Number of non default rules mismatch"
#assert block.getNumberOfScenarios() == 0, "Number of Scenarios Mismatch"


## Row checks
rows = block.getRows()
row = rows[0]
assert row.getName() == "ROW_1", "Rown name mismatch"
assert row.getSite().getName() == "FreePDK45_38x28_10R_NP_162NW_34O", "Site name mismatch"
assert row.getOrigin() == [20140, 22400]
assert row.getOrient() == "MX", "Row orientation"
assert row.getDirection() == "HORIZONTAL", "Row direction"
assert row.getSiteCount() == 421, "Site count mismatch"
assert row.getSpacing() == 380, "Site spacing"
assert row.getBBox().xMin() == 20140, "Row BBox XMin"
assert row.getBBox().yMin() == 22400, "Row BBox yMin"
assert row.getBBox().xMax() == 180120, "Row BBox XMax"
assert row.getBBox().yMax() == 25200, "Row BBox yMax"

## Net checks
nets = block.getNets() 
net = nets[0]
assert net.getName() == "VDD", "Net name mismatch"
assert net.isSpecial() == 1, "Net is special mismatch"
assert net.getWeight() == 1, "Net weight mismatch"
assert net.getTermCount() == 0, "Net term count mismatch"
assert net.getITermCount() == 0, "Net Iterm count mismatch"
assert net.getBTermCount() == 0, "Net Bterm count mismatch"
assert net.getSigType() == "POWER", "Net sig type mismatch"

# Need to fix
#assert len(net.getWire()) == 1, "Net wire mismatch"
assert len(net.getSWires()) == 1, "Net Swire mismatch"
# Need to fix
#assert len(net.getGlobalWire()) == 1, "Net global wire mismatch"
assert net.getNonDefaultRule() == None, "Net non default rule mismatch"


## Special Wire checks
swires = net.getSWires()
swire = swires[0]

assert swire.getBlock().getName() == "gcd", "Special wire block name mismatch"
assert swire.getNet().getName() == "VDD", "Special wire net mismatch"
assert swire.getWireType() == "ROUTED", "Special wire type mismatch "
assert swire.getShield() == None, "Special wire shield mismatch"
assert len(swire.getWires()) == 219, "Special wire number of wires mismatch"

wires = swire.getWires()
wire = wires[0]
assert wire.getTechLayer().getName() == "metal1", "Spacial Wire metal layer mismatch"
assert wire.isVia() == 0, "Special wire is via mismatch"
assert wire.getWidth() == 340, "Special wire width mismatch"
assert wire.getLength() == 159980, "Special wire length mismatch"
assert wire.getWireShapeType() == "FOLLOWPIN", "Special pin shape mismatch"
assert wire.getDir() == 1, "Special wire direction mismatch"

wire = wires[30]
assert wire.getTechLayer().getName() == "metal4", "Spacial Wire metal layer mismatch"
assert wire.isVia() == 0, "Special wire is via mismatch"
assert wire.getWidth() == 179200-22400, "Special wire width mismatch"
assert wire.getLength() == 960, "Special wire length mismatch"
assert wire.getWireShapeType() == "STRIPE", "Special pin shape mismatch"
assert wire.getDir() == 0, "Special wire direction mismatch"

wire = wires[34]
assert wire.isVia() == 1, "Special wire is via mismatch"
assert wire.getTechVia() == None, "Special wire tech via mismatch"
assert wire.getBlockVia().getName() == "via2_960x340", "Special wire block via mismatch"
assert wire.getViaXY() == [24140, 22400], "Special wire Via XY  mismatch"
assert len(wire.getViaBoxes(0)) == 5, "Special wire via box length mismatcj"
assert wire.getDir() == 1, "Special wire via direction mismatch"


vias = block.getVias()
via = vias[0]

assert via.getName() == "via1_960x340", "Via name mismatch"
assert via.getPattern() == "", "Via pattern mismatch"
#assert via.getGenerateRule().getName() == "Via1Array-0", "Via generate rule mismatch"
assert via.getTechVia() == None, "Via get tech via mismatch"
assert via.getBlockVia() == None, "Via get block via mismatch"
assert via.getViaParams().getXCutSize() == 140, "Via get params mismatch"
assert len(via.getBoxes()) == 5, "Via get boxes mismatch"
assert via.getTopLayer().getName() == "metal2", "Via top layer mismatch"
assert via.getBottomLayer().getName() == "metal1", "Via bottom layer mismatch"
assert via.isViaRotated() == 0, "Via is rotated mismatch"
assert via.getOrient() == "R0", "Via orientation mismatch"
