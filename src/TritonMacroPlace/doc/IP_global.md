# Global IP configuration parameters. 
#### By default, these values are used for all IPs, unless specified in local configuration file. 
  * All Units in um (micrometer)
  * The values in this file do not correspond to any foundry enablement

1. Global halo Width for vertical edge of macros. 
    * This is halo width value to be used for each IP along the vertical edge. This is essential to ensure proper routing to macro pins.

          set ::HALO_WIDTH_V 2

2. Global halo Width for horizontal edge of macros. 
    * This is halo width value to be used for each IP along the horizontal edge. This is essential to ensure proper routing to macro pins.

          set ::HALO_WIDTH_H 2
  
3. Global channel width for vertical edge of macros. 
    * This is channel width value to be used for each IP along the vertical edge. This is essential for buffer placement and PDN connectivity of these cells.

          set ::CHANNEL_WIDTH_V 25
  
4. Global channel width for horizontal edge of macros.
    * This is channel width value to be used for each IP along the horizontal edge. This is essential for buffer placement and PDN connectivity of these cells.

          set ::CHANNEL_WIDTH_H 25

#### Example
* [example_IP_global.cfg](example_IP_global.cfg)
