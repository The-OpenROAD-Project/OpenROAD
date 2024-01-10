# MPL2 Debug Messages

MPL2 debug messages are divided in:
- 4 groups according to HierRTLMP flow stages.
- 1 group for the special case in which bus planning is used.

## Groups

### Multilevel Autoclustering
- Group Name: `multilevel_autoclustering`
- Levels:
1. Overall steps of the stage.
2. Include in logs:
    * Macro signatures;
    * Connections of candidate clusters to be merged.

### Coarse Shaping
- Group Name: `coarse_shaping`
- Levels:
1. Overall steps of the stage;
2. Log clusters' tilings.

### Fine Shaping
- Group Name: `fine_shaping`
- Levels:
1. Overall steps of the stage;
2. Details of the shapes of each cluster's children.

### Hierarchical Macro Placement
- Group Name: `hierarchical_macro_placement`
- Levels:
1. Overall steps of the stage.
2. Include in logs:
    * Clusters' connections;
    * Simulated annealing results for both SoftMacro and HardMacro.

### Bus Planning
Special case for bus planning with a single level.
- Group Name: `bus_planning`

### Fine Shaping
- Group Name: `flipping`
- Levels:
1. Print the wire length before and after flipping
