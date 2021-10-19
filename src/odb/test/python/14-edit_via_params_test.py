import opendbpy as odb
import os 

current_dir = os.path.dirname(os.path.realpath(__file__))
tests_dir = os.path.abspath(os.path.join(current_dir, os.pardir))
opendb_dir = os.path.abspath(os.path.join(tests_dir, os.pardir))
data_dir = os.path.join(tests_dir, "data")

db = odb.dbDatabase.create()
db = odb.dbDatabase.create()
lib = odb.read_lef(db, os.path.join(data_dir, "Nangate45/NangateOpenCellLibrary.mod.lef"))
odb.read_def(db, os.path.join(data_dir, "gcd/floorplan.def"))
chip = db.getChip()
tech = db.getTech()
block = chip.getBlock()

via = odb.dbVia_create(block, "via1_960x340")
assert via.getName() == 'via1_960x340', "Via name mismatch"
assert via.hasParams() == 0, "Via has params"
params = via.getViaParams()
params.setBottomLayer(tech.findLayer('metal1'))
params.setCutLayer(tech.findLayer('via1'))
params.setTopLayer(tech.findLayer('metal2'))
params.setXCutSize(140)
params.setYCutSize(140)
params.setXCutSpacing(160)
params.setYCutSpacing(160)
params.setXBottomEnclosure(110)
params.setYBottomEnclosure(100)
params.setXTopEnclosure(110)
params.setYTopEnclosure(100)
params.setNumCutRows(1)
params.setNumCutCols(3)
via.setViaParams(params)


assert via.hasParams() == 1, "Via has params mismatch"
p = via.getViaParams()

assert p.getBottomLayer().getName() == 'metal1', "Via bottom layer name"
assert p.getTopLayer().getName() == 'metal2', "Via top layer name"
assert p.getCutLayer().getName() == 'via1', "Via cut layer name"
assert p.getXCutSize() == 140 and p.getYCutSize() == 140, "Via cut size"
assert p.getXCutSpacing() == 160 and p.getYCutSpacing() == 160, "Via cut spacing"
assert p.getXBottomEnclosure() == 110 and p.getYBottomEnclosure() == 100, "Via bottom enclosure"
assert p.getXTopEnclosure() == 110 and p.getYTopEnclosure() == 100, "Via top enclosure"
assert p.getNumCutRows() == 1 and p.getNumCutCols()==3, "Via number of rows and cols"


boxes = via.getBoxes()
layer_boxes = []
for box in boxes:
    name = box.getTechLayer().getName()
    layer_boxes.append([name, box])

count_metal1 = 0
count_metal2 = 0
count_via1 = 0

for layer, box in layer_boxes:
    if layer == 'metal1':
        count_metal1 = count_metal1 +1
        metal1_shape_width = box.xMax() - box.xMin()
        metal1_shape_height = box.yMax() - box.yMin()
    
    if layer == 'metal2':
        count_metal2 = count_metal2 +1
        metal2_shape_width = box.xMax() - box.xMin()
        metal2_shape_height = box.yMax() - box.yMin()

    if layer == 'via1':
        count_via1 = count_via1 + 1
        if count_via1 == 1:
            via_shape_width = box.xMax() - box.xMin()
            via_shape_height = box.yMax() - box.yMin()

assert count_metal1 == 1, "Number of metal1 shapes"
assert count_metal2 == 1, "Number of metal2 shapes"
assert count_via1 == 3, "Number of via shapes"

w_expected_metal1 = 3*140 + (3-1) * 160 + 2*110
h_expected_metal1 =  1*140 + (1-1) * 160 + 2*100

assert  metal1_shape_width == w_expected_metal1, "Metal1 width mismatch"
assert  metal1_shape_height == h_expected_metal1, "Metal1 height mismatch"

w_expected_metal2 = 3*140 + (3-1) * 160 + 2*110
h_expected_metal2 =  1*140 + (1-1) * 160 + 2*100

assert  metal2_shape_width == w_expected_metal2, "Metal2 width mismatch"
assert  metal2_shape_height == h_expected_metal2, "Metal2 height mismatch"

w_expected_via1 = 140
h_expected_via1 = 140

assert via_shape_width == w_expected_via1
assert via_shape_height == h_expected_via1
