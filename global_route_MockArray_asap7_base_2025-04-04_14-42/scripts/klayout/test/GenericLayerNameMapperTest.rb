#!/usr/bin/env ruby

require 'test/unit'
require_relative '../src/GenericLayerNameMapper'

#
# Unit test for GenericLayerNameMapper
#
class GenericLayerNameMapperTest < Test::Unit::TestCase
    #
    # Sets up GenericLayerNameMapper object
    #
    def setup
        @rep = LayerNameMapper.new()
    end

    #
    # Tests basic operation
    #
    def test_basic
        test_data = [
            [ "M1", "M1", "drawing", "M1" ],
            [ "", "E1", "drawing", "E1" ],
        ]
        test_data.each do | test_case |
            design_layer_name = test_case[0]
            cds_layer_name = test_case[1]
            cds_purpose_name = test_case[2]
            exp_layer_name = test_case[3]
            mapped_name = @rep.map_layer_name(design_layer_name, cds_layer_name,
                                              cds_purpose_name)
            assert(mapped_name == exp_layer_name,
                   sprintf("got %s expecting %s", mapped_name, exp_layer_name))
        end
    end
end
