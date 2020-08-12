# Local IP configuration parameters
* All Units in um (micrometer)
* If specified, these IP-specific parameter values override global values specified in IP_global.cfg file. 
* For IPs without any local (IP-specific) parameters, global values are used for macro placement.
* The values in this file do not correspond to any foundry enablement.

1. Specify IP master name (IP LEF Name example: XYZ_1024XXX) for each parameter.

        set ::parameter IP_REFERENCE value

2. IP-specific halo Width for vertical edge of macros. This is halo width value (placement blockage) to be used for specified IP along the vertical edge. This is essential to ensure proper routing to macro pins.

        set ::HALO_WIDTH_V XYZ_1024XXX 2

3. IP-specific halo Width for horizontal edge of macros. This is halo width value (placement blockage) to be used for specified IP along the horizontal edge. This is essential to ensure proper routing to macro pins.

        set ::HALO_WIDTH_H XYZ_1024XXX 2

4. IP-specific channel width for vertical edge of macros. This is channel width value to be used for specified IP along the vertical edge. This is essential for buffer placement and PDN connectivity of these cells.

        set ::CHANNEL_WIDTH_V XYZ_1024XXX 30 

5. IP-specific channel width for horizontal edge of macros. This is channel width value to be used for specified IP along the horizontal edge. This is essential for buffer placement and PDN connectivity of these cells.

        set ::CHANNEL_WIDTH_H XYZ_1024XXX 25

#### Example
* [example_IP_local.cfg](example_IP_local.cfg)
