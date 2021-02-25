import opendbpy as odb
import helper
import odbUnitTest


class TestWireCodec(odbUnitTest.TestCase):
    #This Function is called before each of the test cases defined below
    def setUp(self):
        self.db, self.tech, self.m1, self.m2, self.m3, self.v12, self.v23 = helper.createMultiLayerDB()
        self.chip  = odb.dbChip_create(self.db)
        self.block = odb.dbBlock_create(self.chip, "chip")
        self.net   = odb.dbNet_create(self.block, "net")
        self.wire  = odb.dbWire_create(self.net)
        self.pathsEnums = ["PATH", "JUNCTION", "SHORT", "VWIRE", "POINT", "POINT_EXT", "VIA", "TECH_VIA", "RECT", "ITERM", "BTERM", "RULE", "END_DECODE"]


    #this function is called after each of the test cases
    def tearDown(self):
        self.db.destroy(self.db)

    def test_decoder(self):
        encoder = odb.dbWireEncoder()
        encoder.begin(self.wire)
        encoder.newPath(self.m1, "ROUTED")
        encoder.addPoint(2000, 2000)
        j1 = encoder.addPoint(10000, 2000)
        encoder.addPoint(18000, 2000)
        encoder.newPath(j1)
        encoder.addTechVia(self.v12)
        j2 = encoder.addPoint(10000, 10000)
        encoder.addPoint(10000, 18000)
        encoder.newPath(j2)
        j3 = encoder.addTechVia(self.v12)
        encoder.addPoint(23000, 10000, 4000)
        encoder.newPath(j3)
        encoder.addPoint(3000, 10000)
        encoder.addTechVia(self.v12)
        encoder.addTechVia(self.v23)
        encoder.addPoint(3000, 10000, 4000)
        encoder.addPoint(3000, 18000, 6000)
        encoder.end()

        

        decoder = odb.dbWireDecoder()
        decoder.begin(self.wire)


        # Encoding started with a path
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.PATH
        

        # Check first point
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.POINT
        point = decoder.getPoint()
        assert point == [2000, 2000]


        # Check second point
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.POINT
        point = decoder.getPoint()
        assert point == [10000, 2000]


        # Check third point
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.POINT
        point = decoder.getPoint()
        assert point == [18000, 2000]


        # Check first junction id
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.JUNCTION
        jid = decoder.getJunctionValue()
        assert jid == j1


        # Check junction point
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.POINT
        point = decoder.getPoint()
        assert point == [10000, 2000]


        # Check tech via
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.TECH_VIA
        tchVia = decoder.getTechVia()
        assert tchVia.getName() == self.v12.getName()


        # Check next point
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.POINT
        point = decoder.getPoint()
        assert point == [10000, 10000]


        # Check next point
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.POINT
        point = decoder.getPoint()
        assert point == [10000, 18000]

        # Check second junction id
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.JUNCTION
        jid = decoder.getJunctionValue()
        assert jid == j2


        # Check junction point
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.POINT
        point = decoder.getPoint()
        assert point == [10000, 10000]


        # Check tech via
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.TECH_VIA
        tchVia = decoder.getTechVia()
        assert tchVia.getName() == self.v12.getName()


        # Check next point
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.POINT_EXT
        point = decoder.getPoint_ext()
        assert point == [23000, 10000, 4000]

        
        # Check third junction id
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.JUNCTION
        jid = decoder.getJunctionValue()
        assert jid == j3


        # Check junction point
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.POINT
        point = decoder.getPoint()
        assert point == [10000, 10000]

        # Check next point
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.POINT
        point = decoder.getPoint()
        assert point == [3000, 10000]

        # Check tech via
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.TECH_VIA
        tchVia = decoder.getTechVia()
        assert tchVia.getName() == self.v12.getName()

        # Check tech via
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.TECH_VIA
        tchVia = decoder.getTechVia()
        assert tchVia.getName() == self.v23.getName()

        # Check next point
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.POINT_EXT
        point = decoder.getPoint_ext()
        assert point == [3000, 10000, 4000]

        # Check next point
        nextOp = decoder.next()
        assert nextOp == odb.dbWireDecoder.POINT_EXT
        point = decoder.getPoint_ext()
        assert point == [3000, 18000, 6000]



if __name__=='__main__':
    odbUnitTest.mainParallel(TestWireCodec)