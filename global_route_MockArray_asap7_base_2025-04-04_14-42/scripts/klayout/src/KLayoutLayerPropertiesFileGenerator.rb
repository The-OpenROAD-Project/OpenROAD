#!/usr/bin/env -S klayout -b -r
#
# Generates a KLayout layer properties (.lyp) file from the Virtuoso SKILL Tech
# File
#
# import_tf needs to be in ~/.klayout/ruby, not ~/.klayout/salt
#
# KLayout doesn't support script-specific command line options, so work around
# it with env vars
#
# Usage: KLayoutLayerPropertiesFileGenerator.rb
#

include RBA
require 'import_tf'

#
# Class to generate KLayout layer properties file from Virtuoso tech file
#
class KLayoutLayerPropertiesFileGenerator
    #
    # Initializer
    #
    # layout_view: layout view to import tech file into
    #
    def initialize()
        @layout_view = LayoutView.new
    end

    #
    # Reads the Virtuoso tech file
    #
    def read_virtuoso_tech_file(file_name)
        TechfileToKLayout.import_techfile(@layout_view, file_name)
    end

    #
    # Writes the layer properties file
    #
    def write_layer_properties_file(file_name)
        @layout_view.save_layer_props(file_name)
    end

    #
    # Standalone main driver
    #
    # Uses env vars to get arguments since KLayout doesn't support
    # script-specific arguments (e.g. it thinks that all arguments are for it)
    #
    def KLayoutLayerPropertiesFileGenerator.main
        input_file = ENV["VIRTUOSO_TECH_FILE"]
        output_file = ENV["KLAYOUT_LAYER_PROPERTIES_FILE"]

        if input_file && output_file
            rep = KLayoutLayerPropertiesFileGenerator.new()
            rep.read_virtuoso_tech_file(input_file)
            rep.write_layer_properties_file(output_file)
        else
            puts "Usage: KLayoutLayerPropertiesFileGenerator.rb"
            exit 1
        end
    end
end

#
# Only call the main driver if we're calling this as a script
#
if __FILE__ == $0
    KLayoutLayerPropertiesFileGenerator.main()
end
