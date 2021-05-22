## ICeWall set_rdl_cover_file_name
### Synopsis
```
  % ICeWall set_rdl_cover_file_name <file_name>
```
### Description
Specify the name of the file to which the routing of the redistribution layer is to be written. If not specified, the default value is cover.def.
In the previous release, the openroad database did not support 45 degree lines used by RDL routing, and this cover.def allowed for the RDL to be added at the end of the flow, without being added to the database. Now that the database will allow 45 degree lines, and this command will be deprecated once ICeWall has been modified to write RDL into the database directly.
### Examples
```
ICeWall set_rdl_cover_file_name rdl.cover.def
```
