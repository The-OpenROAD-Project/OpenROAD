# Graphical User Interface

The graphical user interface can be access by launching OpenROAD with ``-gui`` or 
by opening it from the command-line with ``gui::show``.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Add Buttons to the Toolbar

This command creates toolbar button with name set using the
`-text` flag and accompanying logic in the `-script` flag.

Returns: name of the new button, either ``name`` or ``buttonX``.

```tcl
create_toolbar_button 
    [-name name]
    -text button_text
    -script tcl_script 
    [-echo]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `-name` | The name of the button, used when deleting the button. |
| `-text` | The text to put on the button. |
| `-script` | The tcl script to evaluate when the button is pressed. |
| `-echo` | This indicate that the commands in the ``tcl_script`` should be echoed in the log. |

### Remove Toolbar Button

To remove toolbar button: 

```tcl
gui::remove_toolbar_button 
    name
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `-name` | The name of the button, used when deleting the button. |

### Add items to the Menubar

This command add items to the menubar.
Returns: name of the new item, either ``name`` or ``actionX``.


```tcl
create_menu_item 
    [-name name]
    [-path menu_path]
    -text item_text
    -script tcl_script
    [-shortcut key_shortcut] 
    [-echo]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `-name` | (optional) name of the item, used when deleting the item.|
| `-path`|  (optional) Menu path to place the new item in (hierarchy is separated by /), defaults to "Custom Scripts", but this can also be "Tools" or "New menu/New submenu".|
| `-text` | The text to put on the item.|
| `-script` | The tcl script to evaluate when the button is pressed.|
| `-shortcut`| (optional) key shortcut to trigger this item.|
| `-echo` | (optional) indicate that the commands in the ``tcl_script`` should be echoed in the log. |

### Remove items from the Menubar

To remove menu item: 

```tcl
gui::remove_menu_item 
    name
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `-name` | name of the item, used when deleting the item.|

### Save Image

This command can be both be used when the GUI is active and not active
to save a screenshot with various options.

```tcl
save_image 
    [-resolution microns_per_pixel]
    [-area {x0 y0 x1 y1}]
    [-width width]
    [-display_option {option value}]
    filename
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `filename` | path to save the image to. |
| `-area` | x0, y0 - first corner of the layout area (in microns) to be saved, default is to save what is visible on the screen unless called when gui is not active and then it selected the whole block. x1, y1 - second corner of the layout area (in microns) to be saved, default is to save what is visible on the screen unless called when gui is not active and then it selected the whole block.|
| `-resolution`| resolution in microns per pixel to use when saving the image, default will match what the GUI has selected.|
| `-width`| width of the output image in pixels, default will be computed from the resolution. Cannot be used with ``-resolution``.|
| `-display_option`| specific setting for a display option to show or hide specific elements. For example, to hide metal1 ``-display_option {Layers/metal1 false}``, to show routing tracks ``-display_option {Tracks/Pref true}``, or to show everthing ``-display_option {* true}``.|

### Save Clocktree Image

This command saves the screenshot of clocktree given options 
to `filename`.

```tcl
save_clocktree_image 
    filename
    -clock clock_name
    [-width width]
    [-height height]
    [-scene scene]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
|`filename`| path to save the image to. |
|`-clock`| name of the clock to save the clocktree for. |
|`-scene`| name of the timing scene to save the clocktree for, default to the first scene defined. |
|`-height`| height of the image in pixels, defaults to the height of the GUI widget. |
|`-width`| width of the image in pixels, defaults to the width of the GUI widget. |

### Save Timing Histogram Image

This command saves the screenshot of timing histogram given options
to `filename`.

```tcl
save_histogram_image
    filename
    [-mode mode]
    [-width width]
    [-height height]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
|`filename`| path to save the image to. |
|`-mode`| chart mode to save, defaults to "setup". |
|`-height`| height of the image in pixels, defaults to 500px. |
|`-width`| width of the image in pixels, defaults to 500px. |

### Generate animated images

This command can be used to generate an animated gif.

When used with -start this command returns an integer key that can be
used to distinguish files if multiple are generated.  That key can be
provided when using -add or -end.  If only a single file is being used
the key can be ignored.

```tcl
save_animated_gif
    -start|-add|-end
    [-resolution microns_per_pixel]
    [-area {x0 y0 x1 y1}]
    [-width width]
    [-delay delay]
    [-key key]
    [filename]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `-start` | start a new animation. |
| `-add` | add a new frame to the animation. |
| `-end` | terminate the animtion and save file. |
| `filename` | path to save the animation to. |
| `-area` | x0, y0 - first corner of the layout area (in microns) to be saved, default is to save what is visible on the screen unless called when gui is not active and then it selected the whole block. x1, y1 - second corner of the layout area (in microns) to be saved, default is to save what is visible on the screen unless called when gui is not active and then it selected the whole block.|
| `-resolution`| resolution in microns per pixel to use when saving the image, default will match what the GUI has selected.|
| `-width`| width of the output image in pixels, default will be computed from the resolution. Cannot be used with ``-resolution``.|
| `-delay`| delay between frames in the GIF.|
| `-key`| used to distinguish multiple GIF files (returned by -add). Defaults to the most recent GIF.|

### Select Objects

This command selects object based on options.
Returns: number of objects selected.

```tcl
select 
    -type object_type
    [-name glob_pattern]
    [-filter attribute=value]
    [-case_insensitive]
    [-highlight group]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
|`-type`| name of the object type. For example, ``Inst`` for instances, ``Net`` for nets, and ``Marker`` for database markers.|
|`-name`| (optional) filter selection by the specified name. For example, to only select clk nets ``*clk*``. Use ``-case_insensitive`` to filter based on case insensitive instead of case sensitive.|
|`-filter`| (optional) filter selection based on the objects' properties. ``attribute`` represents the property's name and ``value`` the property's value. In case the property holds a collection (e. g. BTerms in a Net) or a table (e. g. Layers in a Generate Via Rule) ``value`` can be any element within those. A special case exists for checking whether a collection is empty or not by using the value ``CONNECTED``. This can be useful to select a specific group of elements (e. g. BTerms=CONNECTED will select only Nets connected to Input/Output Pins).|
|`-highlight`| (optional) add the selection to the specific highlighting group. Values can be 0 to 7. |

### Display Timing Cones

This command displays timing cones for a pin given options.

```tcl
display_timing_cone 
    pin
    [-fanin]
    [-fanout]
    [-off]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
|`pin` | name of the instance or block pin. |
|`-fanin`| (optional) display the fanin timing cone. |
|`-fanout`| (optional) display the fanout timing cone. |
|`-off`| (optional) remove the timing cone. |

### Focus Net

This command limits the drawing to specified net.

```tcl
focus_net 
    net
    [-remove]
    [-clear]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `pin` | name of the net. |
| `-remove` | (optional) removes the net from from the focus. |
| `-clear` | (optional) clears all nets from focus. |

## TCL functions

### Is GUI Enabled

Determine is the GUI is active:

```tcl
gui::enabled
```

### Trigger GUI to Load Design

Announce to the GUI that a design was loaded 
(note: this is only needed when the design was loaded through the odb API and not via ``read_def`` or ``read_db``):

```tcl
gui::design_created
```

### Load Database Markers Result

To select a marker category

```tcl
gui::select_marker_category 
    category
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `category` | Database marker category. |

### Show GUI

To open the GUI from the command-line (this command does not return until the GUI is closed):

```tcl
gui::show 
    script
    interactive
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `script` | TCL script to evaluate in the GUI. |
| `interactive` | Boolean if true, the GUI should open in an interactive session (default), or if false that the GUI would execute the script and return to the terminal.|

### Set GUI Title

To set the title of the main GUI window:

```tcl
gui::set_title title
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `title` | window title to use for the main GUI window |

### Hide GUI

To close the GUI and return to the command-line:

```tcl
gui::hide
```

### Minimize the GUI

To minimize the GUI window to an icon:

```tcl
gui::minimize
```

### Unminimize the GUI

To unminimize the GUI window from an icon:

```tcl
gui::unminimize
```

### Layout Fit

To fit the whole layout in the window:

```tcl
gui::fit
```

### Zoom to a specific region

To zoom in our out to a specific region:

```tcl
gui::zoom_to 
    x0 y0 x1 y1
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `x0, y0, x1, y1`| first and second corner of the layout area in microns.|

### Zoom In

To zoom in the layout:

```tcl
gui::zoom_in 
    x y
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `x, y` | new center of layout in microns.|

### Zoom Out

To zoom out the layout:

```tcl
gui::zoom_out 
    x y
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `x, y` |  new center of layout in microns.|

### Center At

To move the layout to new area:

```tcl
gui::center_at 
    x y
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `x, y` | new center of layout in microns.|

### Set Resolution

To change the resolution to a specific value:

```tcl
gui::set_resolution
    resolution
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `resolution` | database units per pixel. |

### Add a single net to selection

To add a single net to the selected items:

```tcl
gui::selection_add_net 
    name
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` | name of the net to add.|

### Add multiple nets to selection

To add several nets to the selected items using a regex:

```tcl
gui::selection_add_nets 
    name_regex
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name_regex`| regular expression of the net names to add.| 

### Add a single inst to selection

To add a single instance to the selected items:

```tcl
gui::selection_add_inst 
    name
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` | name of the instance to add. |

### Add multiple insts to selection

To add several instances to the selected items using a regex:

```tcl
gui::selection_add_insts 
    name_regex
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name_regex` | regular expression of the instance names to add. |

### Select at point or area

To add items at a specific point or in an area:

Example usage:
```
gui::select_at x y
gui::select_at x y append
gui::select_at x0 y0 x1 y1
gui::select_at x0 y0 x1 y1 append
```

```tcl
gui::select_at 
    x0 y0 x1 y1
    [append]

Or

gui::select_at
    x y 
    [append]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `x, y` | point in the layout area in microns. |
| `x0, y0, x1, y1`| first and second corner of the layout area in microns. |
| `append`| if ``true`` (the default value) append the new selections to the current selection list, else replace the selection list with the new selections. |

### Select next item from selection

To navigate through multiple selected items:
Returns: current index of the selected item.

```tcl
gui::select_next
```

### Select previous item from selection

To navigate through multiple selected items:
Returns: current index of the selected item.

```tcl
gui::select_previous 
```

### Clear Selection

To clear the current set of selected items:

```tcl
gui::clear_selections
```

### Get Selection Property

To get the properties for the current selection in the Inspector:

```tcl
gui::get_selection_property 
    name
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` | name of the property. For example, ``Type`` for object type or ``bbox`` for the bounding box of the object. |

### Animate Selection

To animate the current selection in the Inspector:

```tcl
gui::selection_animate 
    [repeat]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `repeat` | indicate how many times the animation should repeat, default value is 0 repeats. If the value is 0, the animation will repeat indefinitely.|

### Highlight Net

To highlight a net:

```tcl
gui::highlight_net 
    name
    [highlight_group]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` |  name of the net to highlight.|
| `highlight_group` | group to add the highlighted net to, defaults to ``0``, valid groups are ``0 - 7``. |

### Highlight Instance

To highlight an instance:

```tcl
gui::highlight_inst 
    name
    [highlight_group]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` | name of the instance to highlight. |
| `highlight_group` | group to add the highlighted instance to, defaults to ``0``, valid groups are ``0 - 7``. |

### Clear Highlight Groups

To clear the highlight groups:

```tcl
gui::clear_highlights
    [highlight_group]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `highlight_group` | group to clear, defaults to ``0``, valid groups are ``-1 - 7``. Use ``-1`` to clear all groups. |

### Add Ruler to Layout

To add a ruler to the layout:

1. either press ``k`` and use the mouse to place it visually.
To disable snapping for the ruler when adding, hold the ``Ctrl`` key, and to allow non-horizontal or vertical snapping when completing the ruler hold the ``Shift`` key.

2. or use the command:

Returns: name of the newly created ruler.

```tcl
gui::add_ruler 
    x0 y0 x1 y1
    [label]
    [name]
    [euclidian]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `x0, y0, x1, y1` | first and second end point of the ruler in microns. |
| `label` | text label for the ruler. |
| `name` | name of the ruler. |
| `euclidian` | ``1`` for euclidian ruler, and ``0`` for regular ruler. |

### Delete a single ruler

To remove a single ruler:

```tcl
gui::delete_ruler 
    name
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` | name of the ruler. |

### Clear All Rulers

To remove all the rulers:

```tcl
gui::clear_rulers
```

### Add Label to Layout

To add a label to the layout use the following command:

Returns: name of the newly created label.

```tcl
add_label -position {x y}
          [-anchor anchor]
          [-color color]
          [-size size]
          [-name name]
          text
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `-position` | point of the label in microns. |
| `-anchor` | anchor point for text, default is center. |
| `-color` | color to use for the label, default is white. |
| `size` | size of the label, default is determined by the default GUI font. |
| `name` | name of the label, one will be generated if not provided. |
| `text` | text for the label. |

### Delete a single label

To remove a single label:

```tcl
gui::delete_label
    name
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` | name of the label. |

### Clear All Labels

To remove all the labels:

```tcl
gui::clear_labels
```

### Display help

To display the help for a specific command or messasge.

```tcl
gui::show_help
    cmd_msg
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `cmd_msg`| command or message ID. |

### Set Heatmap

To control the settings in the heat maps:

The currently availble heat maps are:
- ``Pin``
- ``Power``
- ``Routing``
- ``Placement``
- ``IRDrop``
- ``RUDY`` [^RUDY]

These options can also be modified in the GUI by double-clicking the underlined display control for the heat map.

```tcl
gui::set_heatmap 
    name
    [option]
    [value]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` | is the name of the heatmap. |
| `option` | is the name of the option to modify. If option is ``rebuild`` the map will be destroyed and rebuilt. |
| `value` | is the new value for the specified option. This is not used when rebuilding map. |

### Dump Heatmap to file

To save the raw data from the heat maps ins a comma separated value (CSV) format:

```tcl
gui::dump_heatmap 
    name 
    filename
```

#### Options

| Switch Name | Description |
| ---- | ---- |
|`name` | is the name of the heatmap. |
|`filename` | path to the file to write the data to. |

[^RUDY]: RUDY means Rectangular Uniform wire DensitY, which can predict the routing density very rough and quickly. You can see this notion in [this paper](https://past.date-conference.com/proceedings-archive/2007/DATE07/PDFFILES/08.7_1.PDF) 


### Clocktree Selection

Select a clock in the clock viewer:

```tcl
gui::select_clockviewer_clock
    name
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` |  name of clock to select |

### GUI Display Controls

Control the visible and selected elements in the layout:

```tcl
gui::set_display_controls 
    name 
    [display_type] 
    [value]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` |  is the name of the control. For example, for the power nets option this would be ``Signals/Power`` or could be ``Layers/*`` to set the option for all the layers. |
| `display_type` | is either ``visible``, ``selectable``, ``color`` |
| `value` | is either ``true`` or ``false`` for ``visible`` or ``selectable`` or the name of the color |

### Check Display Controls

To check the visibility or selectability of elements in the layout:

```tcl
gui::check_display_controls 
    name 
    display_type 
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` | is the name of the control. For example, for the power nets option this would be ``Signals/Power`` or could be ``Layers/*`` to set the option for all the layers. |
| `display_type` | is either ``visible`` or ``selectable`` |

### Save Display Controls

When performing a batch operation changing the display controls settings, 
the following command can be used to save the current state of the display controls.

```tcl
gui::save_display_controls
```

### Restore Display Controls

This command restores display controls.

```tcl
gui::restore_display_controls
```

### Input Dialog

To request user input via the GUI:
Returns: a string with the input, or empty string if canceled.

```tcl
gui::input_dialog 
    title
    question
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `title` | is the title of the input message box. |
| `question` | is the text for the message box. |

### Pause script execution

Pause the execution of the script:

```tcl
gui::pause 
    [timeout]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `timeout` | is specified in milliseconds, if it is not provided the pause will last until the user presses the Continue button.|

### Show widget

To open a specific layout widget:

```tcl
gui::show_widget 
       name
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` | of the widget. For example, the display controls would be "Display Control". |

### Hide widget

To close a specific layout widget:

```tcl
gui::hide_widget 
    name
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` | of the widget. For example, the display controls would be "Display Control". |

### Select chart

To select a specific chart in the charts widget:

```tcl
gui::select_chart
    name
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `name` | of the chart. For example, "Endpoint Slack". |

### Update timing report

Update the paths in the Timing Report widget:

```tcl
gui::update_timing_report
```

### Show Worst Path

Update the paths in the Timing Report widget and select the path with the worst slack:

```tcl
gui::show_worst_path
    [-setup|-hold]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `-setup` | Select the path with the worst setup slack (default). |
| `-hold` | Select the path with the worst hold slack. |

### Clear Timing Path

Clear the selected timing path in the Timing Report widget:

```tcl
gui::clear_timing_path
```

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
