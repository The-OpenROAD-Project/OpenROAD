#!/usr/bin/env ruby
#
# Generic LayerNameMapper
#

require 'optparse'

#
# Class to map layer names (default)
#
class LayerNameMapper
    #
    # Initializer
    #
    def initialize()
    end

    #
    # Maps layers to the desired name in the layer map
    #
    def map_layer_name(design_manual_layer_name, cds_layer_name, cds_purpose_name)
        if design_manual_layer_name && design_manual_layer_name.length > 0
            return design_manual_layer_name
        end
        return cds_layer_name
    end
    
    #
    # Standalone main driver
    #
    def LayerNameMapper.main()
        options = {}
        OptionParser.new do |opts|
            opts.banner = "Usage: LayerNameMapper.rb -d design_manual_layer_name -l cds_layer_name -p cds_purpose_name"
            opts.on("-dDESIGN_MANUAL_LAYER_NAME", "--design_manual_layer_name=DESIGN_MANUAL_LAYER_NAME") do |design_manual_layer_name|
                options[:design_manual_layer_name] = design_manual_layer_name
            end
            opts.on("-lCDS_LAYER_NAME", "--layer_name=CDS_LAYER_NAME") do |cds_layer_name|
                options[:cds_layer_name] = cds_layer_name
            end
            opts.on("-pCDS_PURPOSE_NAME", "--purpose_name=CDS_PURPOSE_NAME") do |cds_purpose_name|
                options[:cds_purpose_name] = cds_purpose_name
            end
        end.parse!
        if options[:design_manual_layer_name] && options[:cds_layer_name] &&
           options[:cds_purpose_name]
            rep = LayerNameMapper.new()
            puts rep.map_layer_name(options[:design_manual_layer_name],
                                    options[:cds_layer_name],
                                    options[:cds_purpose_name])
        end
    end
end


#
# Only call the main driver if we're calling this as a script
#
if __FILE__ == $0
    LayerNameMapper.main()
end
