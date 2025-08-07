# dbSta Pin Checker Commands
namespace eval dbSta {
    # Check if pin is valid (returns 1/0)
    proc is_pin_valid {pin_name} {
        return [ord::lef_checker::isPinValid $pin_name]
    }
    
    # Check all pins in library (returns error count)
    proc check_pin_alignment {lib_name} {
        return [ord::lef_checker::checkPinAlignment $lib_name]
    }
    
    # Get manufacturing grid in DBU
    proc get_manufacturing_grid {} {
        return [ord::lef_checker::getManufacturingGrid]
    }
    
    package provide dbSta 1.0
}
