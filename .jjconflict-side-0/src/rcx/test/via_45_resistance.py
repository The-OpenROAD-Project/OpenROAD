import helpers  # only for setting logging


def set_resistance(tech):
    layer = tech.getDB().getTech().findLayer
    layer("via1").setResistance(5)
    layer("via2").setResistance(5)
    layer("via3").setResistance(5)
    layer("via4").setResistance(3)
    layer("via5").setResistance(3)
    layer("via6").setResistance(3)
    layer("via7").setResistance(1)
    layer("via8").setResistance(1)
    layer("via9").setResistance(0.5)
