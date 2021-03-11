import opendbpy as odb
import os 

current_dir = os.path.dirname(os.path.realpath(__file__))
tests_dir = os.path.abspath(os.path.join(current_dir, os.pardir))
opendb_dir = os.path.abspath(os.path.join(tests_dir, os.pardir))
data_dir = os.path.join(tests_dir, "data")

db = odb.dbDatabase.create()
lib = odb.read_lef(db, os.path.join(data_dir, "Nangate45/NangateOpenCellLibrary.mod.lef"))
odb.read_def(db, os.path.join(data_dir, "gcd/floorplan.def"))
chip = db.getChip()
block = chip.getBlock()
nets = block.getNets()
tech = db.getTech()

### Block checks
assert block.getName() == "gcd", "Block name mismatch"
units = block.getDefUnits()
assert units == 2000, "DEF units mismatch"
assert len(block.getChildren()) == 0, "Number of children mismatch"
assert len(block.getInsts()) == 482, "Number of instsances mismatch"
assert len(block.getBTerms()) == 54, "Number of B terms mismatch"
assert len(block.getObstructions())== 0, "Number of obstructions mismatch"
assert len(block.getBlockages()) == 0, "NUmber of blockages mismatch"
assert len(block.getNets()) == 385, "Number of nets mismatch"
assert len(block.getVias()) == 0, "Number of vias mismatch"
assert len(block.getRows()) == 56, "Number of rows mismatch"
bbox = block.getBBox()
assert bbox.xMin() == 0, "Bbox xMin mismatch" 
assert bbox.xMax() == 200260, "Bbox xMan mismatch" 
assert bbox.yMin() == 0, "Bbox yMin mismatch" 
assert bbox.yMax() == 201600, "Bbox yMax mismatch"
assert block.getGCellGrid() == None, "G cell grid"
die_area = block.getDieArea()
assert [die_area.xMin(), die_area.yMin(), die_area.xMax(), die_area.yMax()] == [0, 0, 200260, 201600]
assert len(block.getRegions()) == 0, "Number of regions mismatch"
assert len(block.getNonDefaultRules()) == 0, "Number of non default rules mismatch"
#assert block.getNumberOfScenarios() == 0, "Number of Scenarios Mismatch"


## Row checks
rows = block.getRows()
row = rows[0]
assert row.getName() == "ROW_0", "Rown name mismatch"
assert row.getSite().getName() == "FreePDK45_38x28_10R_NP_162NW_34O", "Site name mismatch"
assert row.getOrigin() == [20140, 22400]
assert row.getOrient() == "MX", "Row orientation"
assert row.getDirection() == "HORIZONTAL", "Row direction"
assert row.getSiteCount() == 422, "Site count mismatch"
assert row.getSpacing() == 380, "Site spacing"
assert row.getBBox().xMin() == 20140, "Row BBox XMin"
assert row.getBBox().yMin() == 22400, "Row BBox yMin"
assert row.getBBox().xMax() == 180500, "Row BBox XMax"
assert row.getBBox().yMax() == 25200, "Row BBox yMax"


## Insts checks
insts = block.getInsts()
inst = insts[0]
assert inst.getName() == "_297_", "Instance name mismatch"
assert inst.getOrient() == "R0", "Instance Orientation mismatch"
assert [inst.getBBox().xMin(), inst.getBBox().yMin()] == [0,0], "Instance origin mismatch"
assert inst.getPlacementStatus() == "NONE", "Placement status mismatch"
assert inst.getMaster().getName() == "INV_X1", "Master cell mismatch"
assert len(inst.getITerms()) == 4, "NUmber of ITerms mismtach"
assert inst.getHalo() == None, "Instance Halo mismatch"


## Cell master checks
master = inst.getMaster()
assert master.getName() == "INV_X1", "Master name mismatch"
assert master.getOrigin() == [0,0], "Master origin mismatch"
assert master.getWidth() == 0.38*units, "Master Width mismatch"
assert master.getHeight() == 1.4*units, "master height mismatch"
assert master.getType() == "CORE", "Master type mismatch"
assert master.getLEQ() == None, "Master LEQ"
assert master.getEEQ() == None, "Master EEQ"
assert [master.getSymmetryX(), master.getSymmetryY(), 
    master.getSymmetryR90()] == [1,1,0], "Master symmetry mismatch"
assert len(master.getMTerms()) == 4, "Master M terms mismatch"
assert master.getLib().getName() == "NangateOpenCellLibrary.mod.lef"
assert len(master.getObstructions()) == 0, "Master obstruction mismatch"
assert [master.getPlacementBoundary().xMin(),
    master.getPlacementBoundary().yMin(), master.getPlacementBoundary().xMax(),
    master.getPlacementBoundary().yMax()]== [0,0,760,2800], "Placement bounday mismatch"
assert master.getMTermCount() == 4, "Number of M terms mismatch"
assert master.getSite().getName() == "FreePDK45_38x28_10R_NP_162NW_34O", "Master site mismatch"

## Net checks
nets = block.getNets() 
net = nets[0]
assert net.getName() == "clk", "Net name mismatch"
assert net.getWeight() == 1, "Net weight mismatch"
assert net.getTermCount() == 36, "Net term count mismatch"
assert net.getITermCount() == 35, "Net Iterm count mismatch"
assert net.getBTermCount() == 1, "Net Bterm count mismatch"
assert net.getSigType() == "SIGNAL", "Net sig type mismatch"
assert net.getWire() == None, "Net wire mismatch"
assert net.getSWires() == [], "Net Swire mismatch"
assert net.getGlobalWire() == None, "Net global wire mismatch"
assert net.getNonDefaultRule() == None, "Net non default rule mismatch"

