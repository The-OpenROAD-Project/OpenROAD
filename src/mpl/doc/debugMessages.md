# MPL2 Debug Messages

MPL2 debug messages are divided in:
- 5 groups according to HierRTLMP flow stages.
- 1 group for a post-process stage responsible for pushing the macros to the boundaries if possible.

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
1. Summary with the final costs of each penalty for cluster/macro placement.
2. Include locations chosen for each child/macro.
3. Include, before the summary, a list with the connections between clusters.

### Orientation Improvement
- Group Name: `flipping`
- Levels:
1. Print the wire length before and after flipping

### Boundary Push
- Group Name: `boundary_push`
- Levels:
1. Print name of the macro cluster currently being pushed, its distance to the close boundaries and a message if the move was not possible due to overlap.