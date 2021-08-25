import odb
import os


def createSimpleDB():
    db = odb.dbDatabase.create()
    tech = odb.dbTech.create(db)
    L1 = odb.dbTechLayer_create(tech, 'L1', 'ROUTING')
    lib = odb.dbLib.create(db, "lib")
    odb.dbChip.create(db)
    #Creating Master and2 and or2
    and2 = createMaster2X1(lib, 'and2', 1000, 1000, 'a', 'b', 'o')
    or2  = createMaster2X1(lib, 'or2', 500, 500, 'a', 'b', 'o')
    return db, lib


def createMultiLayerDB():
    db = odb.dbDatabase.create()
    tech = odb.dbTech.create(db)
    
    m1 = odb.dbTechLayer_create(tech, "M1", 'ROUTING')
    m1.setWidth(2000)
    m2 = odb.dbTechLayer_create(tech, "M2", 'ROUTING')
    m2.setWidth(2000)
    m3 = odb.dbTechLayer_create(tech, "M3", 'ROUTING')
    m3.setWidth(2000)

    v12 = odb.dbTechVia_create(tech, "VIA12")
    odb.dbBox_create(v12, m1, 0, 0, 2000, 2000)
    odb.dbBox_create(v12, m2, 0, 0, 2000, 2000)

    v23 = odb.dbTechVia_create(tech, "VIA23")
    odb.dbBox_create(v23, m2, 0, 0, 2000, 2000)
    odb.dbBox_create(v23, m3, 0, 0, 2000, 2000)

    return db, tech, m1, m2, m3, v12, v23


    
#logical expr OUT = (IN1&IN2)
#
#     (n1)   +-----
# IN1--------|a    \    (n3)
#     (n2)   | (i1)o|-----------OUT
# IN2--------|b    /            
#            +-----             
def create1LevelBlock(db, lib, parent):
    blockName = '1LevelBlock'
    block = odb.dbBlock_create(parent, blockName, ',')
    #Creating Master and2 and instance inst
    and2 = lib.findMaster('and2')
    inst = odb.dbInst.create(block, and2, "inst")
    #creating our nets
    n1 = odb.dbNet.create(block, "n1")
    n2 = odb.dbNet.create(block, "n2")
    n3 = odb.dbNet.create(block, "n3")
    IN1 = odb.dbBTerm.create(n1, 'IN1')
    IN1.setIoType('INPUT')
    IN2 = odb.dbBTerm.create(n2, 'IN2')
    IN2.setIoType('INPUT')
    OUT = odb.dbBTerm.create(n3, 'OUT')
    OUT.setIoType('OUTPUT')
    #connecting nets
    odb.dbITerm.connect(inst, n1, inst.getMaster().findMTerm('a'))
    odb.dbITerm.connect(inst, n2, inst.getMaster().findMTerm('b'))
    odb.dbITerm.connect(inst, n3, inst.getMaster().findMTerm('o'))
    return block



#logical expr OUT = (IN1&IN2) | (IN3&IN4)
#     (n1)   +-----
# IN1--------|a    \    (n5)
#     (n2)   | (i1)o|-----------+
# IN2--------|b    /            |       +-------
#            +-----             +--------\a     \    (n7)
#                                         ) (i3)o|---------------OUT
#     (n3)   +-----             +--------/b     /
# IN3--------|a    \    (n6)    |       +-------
#     (n4)   | (i2)o|-----------+
# IN4--------|b    /
#            +-----
def create2LevelBlock(db, lib, parent):
    
    blockName = '2LevelBlock'
    block = odb.dbBlock_create(parent, blockName, ',')
    
    and2 = lib.findMaster('and2')
    or2 = lib.findMaster('or2')
    #creating instances
    i1 = odb.dbInst.create(block, and2, "i1")
    i2 = odb.dbInst.create(block, and2, "i2")
    i3 = odb.dbInst.create(block, or2, "i3")
    #creating nets and block terms
    n1 = odb.dbNet.create(block, "n1")
    n2 = odb.dbNet.create(block, "n2")
    n3 = odb.dbNet.create(block, "n3")
    n4 = odb.dbNet.create(block, "n4")
    n5 = odb.dbNet.create(block, "n5")
    n6 = odb.dbNet.create(block, "n6")
    n7 = odb.dbNet.create(block, "n7")
    
    IN1 = odb.dbBTerm.create(n1, 'IN1')
    IN1.setIoType('INPUT')
    IN2 = odb.dbBTerm.create(n2, 'IN2')
    IN2.setIoType('INPUT')
    IN3 = odb.dbBTerm.create(n3, 'IN3')
    IN3.setIoType('INPUT')
    IN4 = odb.dbBTerm.create(n4, 'IN4')
    IN4.setIoType('INPUT')
    OUT = odb.dbBTerm.create(n7, 'OUT')
    OUT.setIoType('OUTPUT')
    #connecting nets
    odb.dbITerm.connect(i1, n1, i1.getMaster().findMTerm('a'))
    odb.dbITerm.connect(i1, n2, i1.getMaster().findMTerm('b'))
    odb.dbITerm.connect(i1, n5, i1.getMaster().findMTerm('o'))
    
    odb.dbITerm.connect(i2, n3, i2.getMaster().findMTerm('a'))
    odb.dbITerm.connect(i2, n4, i2.getMaster().findMTerm('b'))
    odb.dbITerm.connect(i2, n6, i2.getMaster().findMTerm('o'))
    
    odb.dbITerm.connect(i3, n5, i3.getMaster().findMTerm('a'))
    odb.dbITerm.connect(i3, n6, i3.getMaster().findMTerm('b'))
    odb.dbITerm.connect(i3, n7, i3.getMaster().findMTerm('o'))
    
    P1 = odb.dbBPin_create(IN1)
    P2 = odb.dbBPin_create(IN2)
    P3 = odb.dbBPin_create(IN3)
    P4 = odb.dbBPin_create(IN4)
    P5 = odb.dbBPin_create(OUT)
    
    return block

#  +-----
#  |in1   \    
#      out|
#  |in2  /            
#  +-----  
def createMaster2X1(lib, name, width, height, in1, in2, out):
    master = odb.dbMaster_create(lib, name)
    master.setWidth(width)
    master.setHeight(height)
    master.setType('CORE')
    odb.dbMTerm.create(master, in1, 'INPUT')
    odb.dbMTerm.create(master, in2, 'INPUT')
    odb.dbMTerm.create(master, out, 'OUTPUT')
    master.setFrozen()
    return master
def createMaster3X1(lib, name, width, height, in1, in2, in3, out):
    master = odb.dbMaster_create(lib, name)
    master.setWidth(width)
    master.setHeight(height)
    master.setType('CORE')
    odb.dbMTerm.create(master, in1, 'INPUT')
    odb.dbMTerm.create(master, in2, 'INPUT')
    odb.dbMTerm.create(master, in3, 'INPUT')
    odb.dbMTerm.create(master, out, 'OUTPUT')
    master.setFrozen()
    return master
