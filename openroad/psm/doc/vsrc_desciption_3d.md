# 3D voltage source location file description

The `<file_name>.loc` file contains voltage source location, size, voltage value along with chiplet name:

```
<x_coordinate>, <y_coordinate>, <source_size>, <voltage>, <chiplet_instance_name>
```

The description is in continuation from 2D `Vsrc_description.md` file.
For 3D `.loc` files, chiplet name is added. The name is its instance name from the `.3dbx` file.

## Example

```
26.57, 27.615, 1.0, 1.1, chipA
26.57, 67.615, 1.0, 1.1, chipA
82.57, 27.615, 1.0, 1.1, chipA
82.57, 67.615, 1.0, 1.1, chipA
```
