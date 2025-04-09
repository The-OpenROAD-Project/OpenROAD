#!/usr/bin/env ruby
#
# Generates the KLayout layer map file from the Virtuoso Stream Layer Map File
# It's really here to map from the Virtuoso purpose name to KLayout's LEF
# purpose name
#
# Virtuoso Stream Layer Map File Format:
# layer_name purpose_name gds_layer gds_datatype
#
# KLayout Layer Map File Format:
# layer_name lef_purpose_name gds_layer gds_datatype
#
# Usage: KLayoutLayerMapGenerator.rb -i <input_file> -o <output_file>
#                                    [-m <layer_name_mapper_ruby_file>]
#
# layer_name_mapper is the path to a file that implements LayerNameMapper for
# custom layer name mapping
#
# File needs to be located in ~/.klayout/ruby
#

require 'optparse'

#
# Class to generate the layer map from the Virtuoso Stream Layer Map file
#
class KLayoutLayerMapGenerator
    #
    # Structs used to store data
    #
    VirtuosoLayerInfo = Struct.new(:cds_lpp, :gds)
    VirtuosoLayerPurposePair = Struct.new(:layer_name, :purpose_name)
    GDSLayerDataType = Struct.new(:layer, :datatype)

    #
    # Initializer
    #
    # layer_map: Stores mapping from layer name to VirtuosoLayerInfo object
    #
    def initialize(layer_name_mapper_file)
        @layer_map = {}
        # load in layer_name_mapper dynamically
        load layer_name_mapper_file
        @layer_name_mapper = LayerNameMapper.new()
    end

    #
    # Reads the Virtuoso Layer Map file
    #
    def read_virtuoso_layer_map_file(file_name)
        fh = File.open(file_name, chomp: true)
        if fh
            read(fh)
            fh.close
        else
            puts "Error: can't open " + file_name
        end
    end

    #
    # Reads the content from a stream
    #
    # For each layer in the layer mapping file, add an entry into layer_map.
    # The key is the mapped layer name and the value is a list of
    # VirtuosoLayerInfo objects. The value is a list since there can be multiple
    # entries for the same mapped layer name in the Virtuoso map
    #
    def read(fh)
        layer_re = /(\S+)\s+(\S+)\s+(\d+)\s+(\d+)([^#]*)\#\s*(\S+)?/
        fh.each_line do | line |
            if not line.match(/^#/)
                result = line.match(layer_re)
                if result
                    layer_name = result[1]
                    purpose_name = result[2]
                    gds_layer = result[3].to_i
                    gds_datatype = result[4].to_i
                    design_manual_layer_name = result[6]
                    key = map_layer_name(design_manual_layer_name, layer_name,
                                         purpose_name)
                    cds_lpp = VirtuosoLayerPurposePair.new(layer_name, purpose_name)
                    gds = GDSLayerDataType.new(gds_layer, gds_datatype)
                    layer_info = VirtuosoLayerInfo.new(cds_lpp, gds)
                    if @layer_map[key]
                        @layer_map[key] << layer_info
                    else
                        @layer_map[key] = [layer_info]
                    end
                else
                    if line != "\n"
                        puts "Skipping: " + line
                    end
                end
            end
        end
    end
    
    #
    # Maps layers to the desired name in the layer map
    #
    def map_layer_name(design_manual_layer_name, cds_layer_name, cds_purpose_name)
        return @layer_name_mapper.map_layer_name(design_manual_layer_name, cds_layer_name, cds_purpose_name)
    end

    #
    # Writes the layer map to a file
    #
    def write_layer_map_file(file_name)
        out_fh = File.open(file_name, "w") do |out_fh|
            write_layer_map(out_fh)
        end
    end

    #
    # Writes the layer map to a file handle
    #
    def write_layer_map(outfh)
        layer_map = get_map()
        layer_map.each { | layer_name, layer_list |
            layer_list.each { | layer_info |
                gds_info = layer_info.gds
                outfh.printf("%s %d %d\n", layer_name, gds_info.layer,
                             gds_info.datatype)
            }
        }
    end

    #
    # Accessor to return a hash sorted by the gds layer
    #
    def get_map
        sorted_hash = @layer_map.sort_by {|key, value| value[0].gds.layer }
        sorted_hash = Hash[sorted_hash]
        return sorted_hash
    end

    #
    # Standalone main driver
    #
    def KLayoutLayerMapGenerator.main
        options = {"layer_name_mapper": "GenericLayerNameMapper.rb"}
        OptionParser.new do |opts|
            opts.banner = "Usage: KLayoutLayerMapGenerator.rb -i input_file -o output_file"
            opts.on("-iINPUT_FILE", "--input_file=INPUT_FILE") do |input_file|
                options[:input_file] = input_file
            end
            opts.on("-oOUTPUT_FILE", "--output_file=OUTPUT_FILE") do |output_file|
                options[:output_file] = output_file
            end
            opts.on("-mLAYER_NAME_MAPPER", "--layer_name_mapper=LAYER_NAME_MAPPER") do |layer_name_mapper_ruby_file|
                options[:layer_name_mapper_ruby_file] = layer_name_mapper_ruby_file
            end
        end.parse!

        if options[:input_file] && options[:output_file]
            rep = KLayoutLayerMapGenerator.new(options[:layer_name_mapper_ruby_file])
            rep.read_virtuoso_layer_map_file(options[:input_file])
            rep.write_layer_map_file(options[:output_file])
        else
            puts "Usage: KLayoutLayerMapGenerator.rb -i input_file -o output_file [-m layer_name_mapper]"
            exit 1
        end
    end
end

#
# Only call the main driver if we're calling this as a script
#
if __FILE__ == $0
    KLayoutLayerMapGenerator.main()
end
