#!/usr/bin/env ruby

require 'stringio'
require 'test/unit'
require_relative '../src/KLayoutLayerMapGenerator'

#
# Unit test for KLayoutLayerMapGenerator
#
class KLayoutLayerMapGeneratorTest < Test::Unit::TestCase
    #
    # Sets up KLayoutLayerMapGenerator object
    #
    def setup
        current_dir = File.dirname(__FILE__)
        layer_name_map_file = File.join(current_dir, "..", "src",
                                        "GenericLayerNameMapper.rb")
        @rep = KLayoutLayerMapGenerator.new(layer_name_map_file)
    end

    #
    # Tests basic operation
    #
    def test_basic
        data_str = <<~LM_TEXT
# Bogus sample layer map
M1               drawing            99       0                                                               # M1
M1               net                99       23                                                              # M1_NET
M1               net2               99       23                                                              # M1_NET
1Layer           drawing            123      99                                                              # 1Layer
LM_TEXT

        data_stream = StringIO.new(data_str)
        @rep.read(data_stream)
        layer_map = @rep.get_map()
        assert(layer_map)
        assert(layer_map.length == 3)
        # Test M1 entry
        layer_list = layer_map["M1"]
        assert(layer_list.length == 1)
        entry = layer_list[0]
        assert(entry)
        assert(entry.cds_lpp.layer_name == "M1")
        assert(entry.cds_lpp.purpose_name == "drawing")
        assert(entry.gds.layer == 99)
        assert(entry.gds.datatype == 0)
        # Test M1 NET entry
        layer_list = layer_map["M1_NET"]
        assert(layer_list.length == 2)
        layer_list.each do | entry |
            assert(entry)
            assert(entry.cds_lpp.layer_name == "M1")
            assert(entry.cds_lpp.purpose_name.start_with?("net"))
            assert(entry.gds.layer == 99)
            assert(entry.gds.datatype == 23)
        end
        # Test 1Layer entry
        layer_list = layer_map["1Layer"]
        assert(layer_list.length == 1)
        entry = layer_list[0]
        assert(entry.cds_lpp.layer_name == "1Layer")
        assert(entry.cds_lpp.purpose_name == "drawing")
        assert(entry.gds.layer == 123)
        assert(entry.gds.datatype == 99)
    end
end
