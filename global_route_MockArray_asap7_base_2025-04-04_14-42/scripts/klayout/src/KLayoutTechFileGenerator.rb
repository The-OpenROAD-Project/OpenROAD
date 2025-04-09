#!/usr/bin/env -S klayout -b -r
#
# Generates a KLayout technology file (.lyt) from the Virtuoso and Tech LEF
# files
#
# The Ruby files LEFViaData.rb and KLayoutLayerMapGenerator need to be in
# ~/.klayout/ruby
#
# KLayout doesn't support script-specific command line options, so work around
# it with env vars
#
# Usage: KLayoutTechFileGenerator.rb
#

include RBA
require 'LEFViaData'
require 'KLayoutLayerMapGenerator'

#
# Class to generate the KLayout tech file
#
class KLayoutTechFileGenerator
    #
    # Initializer
    #
    # tech: KLayout technology object
    #
    def initialize(name, description, layer_properties_file)
        @tech = Technology.new()
        @tech.name = name
        @tech.description = description
        @tech.layer_properties_file = layer_properties_file
    end

    #
    # Iterates through the Virtuoso layer map and creates the layer map that
    # gets written to the tech file.
    #
    # layer map maps layer_name to a string "layer_name:gds_layer/gds_datatype"
    #
    # Caveats
    #  1) KLayout only writes out one mapping per layer, so we only add the
    #     first occurrence of the layer into the map. This is usually datatype
    #     == 0
    #
    def get_layer_map(sorted_layer_map)
        layer_map = LayerMap.new()
        sorted_layer_map.each do |layer_name, layer_list|
            is_first = true
            layer_list.each do |layer_info|
                gds_info = layer_info.gds
                if layer_name.match?(/\A\d/)
                    # single quote escaping required for layers that start
                    # with a digit
                    layer_name = "'" + layer_name + "'"
                end
                map_str = sprintf("%s:%d/%d", layer_name, gds_info.layer,
                                  gds_info.datatype)
                if is_first
                    layer_map.map(map_str)
                    is_first = false
                end
            end
        end
        return layer_map
    end

    #
    # Adds the via layer stack to the connectivity object
    #
    def add_via_connectivity(lef_file, sorted_layer_map)
        via_data = LEFViaData.new()
        via_data.read_file(lef_file)
        connectivity = @tech.component("connectivity")
        via_data.get_map().each do | via_name, layer_stack |
            lower_layer = layer_stack[0]
            via_layer = layer_stack[1]
            upper_layer = layer_stack[2]
            connection = NetTracerConnectivity.new
            connection.name = via_name
            connection.connection(lower_layer, via_layer, upper_layer)
            connectivity.add(connection)
        end
        # add symbolic mappings based on layers having multiple mappings
        sorted_layer_map.each do | layer_name, layer_list |
            if layer_list.length > 1
                formatted_layer_list = []
                layer_list.each do | layer_info |
                    gds_info = layer_info.gds
                    gds_data = sprintf("%d/%d", gds_info.layer,
                                       gds_info.datatype)
                    formatted_layer_list << gds_data
                end
                expr = formatted_layer_list.join("+")
                connection = NetTracerConnectivity.new
                connection.symbol(layer_name, expr)
                connectivity.add(connection)
            end
        end
    end

    #
    # Returns the Virtuoso layer map
    #
    def get_virtuoso_layer_map(file_name, layer_name_mapper)
        layer_map_gen = KLayoutLayerMapGenerator.new(layer_name_mapper)
        layer_map_gen.read_virtuoso_layer_map_file(file_name)
        sorted_layer_map = layer_map_gen.get_map()
        return sorted_layer_map
    end

    #
    # Adds the layer map to the technology object
    #
    def add_layer_map(layer_map)
        # Gets a copy of the reader options
        reader_opts = @tech.load_layout_options
        lefdef_config = reader_opts.lefdef_config
        lefdef_config.layer_map = layer_map
        # Add a placeholder LEF, so that the technology file has a lef-files
        # element. ORFS keys off this later and replaces the element with the
        # real list of LEF files during during 6_final
        lefdef_config.lef_files = [ "placeholder.lef" ]
        # Store the updated reader options back on the tech object
        @tech.load_layout_options = reader_opts
    end

    #
    # Writes the tech file
    #
    def save_tech(file_name)
        @tech.save(file_name)
    end
    
    #
    # Standalone main driver
    #
    # Uses env vars to get arguments since KLayout doesn't support
    # script-specific arguments (e.g. it thinks that all arguments are for it)
    #
    def KLayoutTechFileGenerator.main()
        name = ENV["TECH_NAME"]
        desc = ENV["TECH_DESC"]
        layer_map_file = ENV["VIRTUOSO_LAYER_MAP_FILE"]
        lef_file = ENV["TECH_LEF"]
        lyp_file = ENV["KLAYOUT_LAYER_PROPERTIES_FILE"]
        lyt_file = ENV["KLAYOUT_TECH_FILE"]
        layer_name_mapper = ENV["LAYER_NAME_MAPPER"]
        rep = KLayoutTechFileGenerator.new(name, desc, lyp_file)
        sorted_layer_map = rep.get_virtuoso_layer_map(layer_map_file,
                                                      layer_name_mapper)
        layer_map = rep.get_layer_map(sorted_layer_map)
        rep.add_layer_map(layer_map)
        rep.add_via_connectivity(lef_file, sorted_layer_map)
        rep.save_tech(lyt_file)
    end
end

#
# Only call the main driver if we're calling this as a script
#
if __FILE__ == $0
    KLayoutTechFileGenerator.main()
end

    
