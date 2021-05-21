## ICeWall set_core_area
### Synopsys
```
  % ICeWall set_core_area <minX> <minY> <maxX> <maxY>
```
### Description
Specify the co-ordinates of the chip core. This area is reserved for stdcell placements. Placement of padcells should be made outside this core area, but within the specified die area
### Examples
```
ICeWall set_core_area 180.012 180.096 2819.964 2819.712
```
