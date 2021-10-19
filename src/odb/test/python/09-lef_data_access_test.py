import opendbpy as odb
import os 

current_dir = os.path.dirname(os.path.realpath(__file__))
tests_dir = os.path.abspath(os.path.join(current_dir, os.pardir))
opendb_dir = os.path.abspath(os.path.join(tests_dir, os.pardir))
data_dir = os.path.join(tests_dir, "data")

db = odb.dbDatabase.create()
odb.read_lef(db, os.path.join(data_dir, "gscl45nm.lef"))
odb.read_def(db, os.path.join(data_dir, "design.def"))
tech = db.getTech()


# Basic LEF checks"
assert tech.getLefVersion() == 5.5, "Lef version mismatch"
assert tech.getLefVersionStr() == '5.5', "Lef version string mismatch"
assert tech.getManufacturingGrid() == 5, "Manufacturing grid size mismatch"
assert tech.getRoutingLayerCount() == 10, "Number of routing layer mismatch"
assert tech.getViaCount() == 14, "Number of vias mismatch"
assert tech.getLayerCount() == 22, "Number of layers mismatch"
assert tech.getLefUnits() == 2000, "Lef units mismatch"
units = tech.getLefUnits()

# Via rule checks
via_rules= tech.getViaGenerateRules()
assert len(via_rules) == 10, "Number of via rules mismatch"
via_rule = via_rules[0]
assert via_rule.getName() == 'M2_M1', "Via rule name mismatch"
assert via_rule.isDefault() == 0, "Via rule default mismatch"
assert via_rule.getViaLayerRuleCount() == 3, "Number of via rules for layer mismatch"
via_layer_rule = via_rule.getViaLayerRule(0)
lower_rule = via_rule.getViaLayerRule(0)
upper_rule = via_rule.getViaLayerRule(1)
cut_rule = via_rule.getViaLayerRule(2)

# Check layer names
assert lower_rule.getLayer().getName() == 'metal1', "Layer name from via rule mismatch"
assert upper_rule.getLayer().getName() == 'metal2', "Layer name from via rule mismatch"
assert cut_rule.getLayer().getName() == 'via1', "Layer name from via rule mismatch"

# Check via rule details
assert lower_rule.hasEnclosure() == 1, "lower enclosure mismatch"
assert lower_rule.hasRect() == 0, "lower has rectangle mismatch"
assert lower_rule.hasSpacing() == 0, "lower has spacing mismatch"
assert upper_rule.hasEnclosure() == 1, "lower enclosure mismatch"
assert upper_rule.hasRect() == 0, "lower has rectangle mismatch"
assert upper_rule.hasSpacing() == 0, "lower has spacing mismatch"
assert cut_rule.hasEnclosure() == 0, "lower enclosure mismatch"
assert cut_rule.hasRect() == 1, "lower has rectangle mismatch"
assert cut_rule.hasSpacing() == 1, "lower has spacing mismatch"


# Checks for number of vias
assert len(tech.getLayers()) == 22, "Number of layers mismatch"
layers = tech.getLayers()
layer =layers[2]
assert layer.getName() == 'metal1', "Layer name mismatch"
assert layer.getLowerLayer().getName() == 'contact', "Lower layer from layer mismatch"
assert layer.getUpperLayer().getName() == 'via1', "Upper layer from layer mismatch"
assert layer.hasAlias() == 0, "Layer hasAlias mismatch"
assert layer.hasArea() == 0, "Layer hasArea mismatch"
assert layer.hasDefaultAntennaRule() == 0, "Layer hasDefaultAntennaRule mismatch"
assert layer.hasMaxWidth() == 0, "Has max spacing"
assert layer.hasMinStep() == 0, "Has min spacing mismatch"
assert layer.hasOxide2AntennaRule() == 0, "has Oxide antenna rule mismatch"
assert layer.hasProtrusion() == 0, "has protrusion mismatch"
assert layer.hasV55SpacingRules() == 0, "Layer hasV55SpacingRules mismatch"

assert layer.getType() == 'ROUTING', "Routing mismatch"
assert layer.getDirection() == "HORIZONTAL", "Direction mismatch"
assert layer.getPitch() == 0.19*units, "pitch mismatch"
assert layer.getWidth() == 0.065*units, "width mismatch"
assert layer.getSpacing() == 0.065*units, "spacing mismatch"
assert layer.getResistance() == 0.38, "resistance mismatch"
assert layer.getCapacitance() == 0.0, "capacitance mismatch"
