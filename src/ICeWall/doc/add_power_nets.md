## ICeWall add_power_nets
### Synopsis
```
  % ICeWall add_power_nets <power_net_name>, ...
```
### Description
Define the nets to be used as power nets in the design, it is not required that these nets exist in the design database, they will be created as necessary.

Once the power net is defined, it can be used as the -signal argument for the ```ICeWall add_pad``` command to enable the addition of power pads to the design.

### Examples
```
ICeWall add_power_nets VDD DVDD_0 DVDD_1
```
