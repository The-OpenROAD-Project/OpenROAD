#!/usr/bin/env ruby
#
# Extracts via layer stack information from the technology LEF file
#
# Usage: LEFViaData.rb -i tech_lef_file
#
#
# This file needs to be located in ~/.klayout/ruby
#

require 'optparse'

#
# Class to extract via layer stack information from technology LEF
#
class LEFViaData
    #
    # Initialization method
    #
    # via_map: stores the mapping from a key to the list of layers in the stack
    # start_via_re: regexp to find the start of the LEF VIA statement
    # via_layer_re: regexp to extract the layer name in the via layer stack
    #
    def initialize()
        @via_map = {}
        @start_via_re = /^\s*VIA\s+(\S+)(?i)(?:\s+DEFAULT)/
        @via_layer_re = /^\s*LAYER\s+(\S+)\s*\;/
    end

    #
    # Reads the tech LEF and creates the VIA map
    #
    def read_file(file_name)
        fh = File.open(file_name, chomp: true)
        if fh
            read(fh)
            fh.close
        else
            puts "Error: can't open " + file_name
        end
    end

    #
    # Reads the content from a stream and looks for the LEF VIA statement
    #
    def read(fh)
        fh.each_line do | line |
            result = line.match(@start_via_re)
            if result
                via_name = result[1]
                read_via(fh, via_name)
            end
        end
    end

    #
    # Reads the LEF VIA definition and adds the via layer stack to the via_map
    # They key to the via_map is the layer names with "_" in between
    # (e.g. M1_V1_M2). The value is the list of layer names.
    #
    def read_via(fh, via_name)
        layer_stack = []
        while (line = fh.gets)
            if line.match("^\s*END")
                #key = layer_stack.join("_")
                key = via_name
                @via_map[key] = layer_stack
                return
            end
            result = line.match(@via_layer_re)
            if result
                layer_name = result[1]
                layer_stack << layer_name
            end
        end
    end

    #
    # Accessor to get the via map
    #
    def get_map()
        return @via_map
    end

    #
    # Standalone main driver
    #
    def LEFViaData.main()
        options = {}
        OptionParser.new do |opts|
            opts.banner = "Usage: LEFViaData.rb -i tech_lef_file"
            opts.on("-iTECH_LEF_FILE", "--input_file=TECH_LEF_FILE") do |input_file|
                options[:input_file] = input_file
            end
        end.parse!

        if options[:input_file]
            rep = LEFViaData.new
            rep.read_file(options[:input_file])
            puts rep.get_map()
        end
    end
end

#
# Only call the main driver if we're calling this as a script
#
if __FILE__ == $0
    LEFViaData.main()
end
        
