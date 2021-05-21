## ICeWall add_ground_nets
### Synopsys
```
  % ICeWall add_ground_nets <ground_net_name>, ...
```
### Description
Define the nets to be used as ground nets in the design, it is not required that these nets exist in the design database, they will be created as necessary.

Once a ground net is defined, it can be used as the -signal argument for the ```ICeWall add_pad``` command to enable the addition of ground pads to the design.

### Examples
```
ICeWall add_ground_nets VSS DVSS_0 DVSS_1
```
