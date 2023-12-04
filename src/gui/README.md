# Graphical User Interface

The graphical user interface can be access by launching OpenROAD with ``-gui`` or by opening it from the command-line with ``gui::show``.

## Commands

### Add buttons to the toolbar

```
create_toolbar_button [-name name]
                      -text button_text
                      -script tcl_script 
                      [-echo]
```

Returns: name of the new button, either ``name`` or ``buttonX``.

Options description:
- ``button_text``: The text to put on the button.
- ``tcl_script``: The tcl script to evaluate when the button is pressed.
- ``name``: (optional) name of the button, used when deleting the button.
- ``echo``: (optional) indicate that the commands in the ``tcl_script`` should be echoed in the log.

To remove the button: 

```
gui::remove_toolbar_button name
```

### Add items to the menubar

```
create_menu_item [-name name]
                 [-path menu_path]
                 -text item_text
                 -script tcl_script
                 [-shortcut key_shortcut] 
                 [-echo]
```

Returns: name of the new item, either ``name`` or ``actionX``.

Options description:
- ``item_text``: The text to put on the item.
- ``tcl_script``: The tcl script to evaluate when the button is pressed.
- ``name``: (optional) name of the item, used when deleting the item.
- ``menu_path``: (optional) Menu path to place the new item in (hierarchy is separated by /), defaults to "Custom Scripts", but this can also be "Tools" or "New menu/New submenu".
- ``key_shortcut``: (optional) key shortcut to trigger this item.
- ``echo``: (optional) indicate that the commands in the ``tcl_script`` should be echoed in the log.

To remove the item: 

```
gui::remove_menu_item name
```


### Save screenshot of layout

This command can be both be used when the GUI is active and not active.

```
save_image [-resolution microns_per_pixel]
           [-area {x0 y0 x1 y1}]
           [-width width]
           [-display_option {option value}]
           filename
```

Options description:
- ``filename`` path to save the image to.
- ``x0, y0`` first corner of the layout area (in microns) to be saved, default is to save what is visible on the screen unless called when gui is not active and then it selected the whole block.
- ``x1, y1`` second corner of the layout area (in microns) to be saved, default is to save what is visible on the screen unless called when gui is not active and then it selected the whole block.
- ``microns_per_pixel`` resolution in microns per pixel to use when saving the image, default will match what the GUI has selected.
- ``width`` width of the output image in pixels, default will be computed from the resolution. Cannot be used with ``-resolution``.
- ``option`` specific setting for a display option to show or hide specific elements. For example, to hide metal1 ``-display_option {Layers/metal1 false}``, to show routing tracks ``-display_option {Tracks/Pref true}``, or to show everthing ``-display_option {* true}``.

### Save screenshot of clock trees

```
save_clocktree_image filename
                     -clock clock_name
                     [-width width]
                     [-height height]
                     [-corner corner]
```

Options description:
- ``filename`` path to save the image to.
- ``-clock`` name of the clock to save the clocktree for.
- ``-corner`` name of the timing corner to save the clocktree for, default to the first corner defined.
- ``-height`` height of the image in pixels, defaults to the height of the GUI widget.
- ``-width`` width of the image in pixels, defaults to the width of the GUI widget.

### Selecting objects

```
select -type object_type
       [-name glob_pattern]
       [-filter attribute=value]
       [-case_insensitive]
       [-highlight group]
```

Returns: number of objects selected.

Options description:
- ``object_type``: name of the object type. For example, ``Inst`` for instances, ``Net`` for nets, and ``DRC`` for DRC violations.
- ``glob_pattern``: (optional) filter selection by the specified name. For example, to only select clk nets ``*clk*``. Use ``-case_insensitive`` to filter based on case insensitive instead of case sensitive.
- ``attribute=value``: (optional) filter selection based on the objects' properties. ``attribute`` represents the property's name and ``value`` the property's value. In case the property holds a collection (e. g. BTerms in a Net) or a table (e. g. Layers in a Generate Via Rule) ``value`` can be any element within those. A special case exists for checking whether a collection is empty or not by using the value ``CONNECTED``. This can be useful to select a specific group of elements (e. g. BTerms=CONNECTED will select only Nets connected to Input/Output Pins).
- ``group``: (optional) add the selection to the specific highlighting group. Values can be 0 to 7.

### Displaying timing cones

```
display_timing_cone pin
                    [-fanin]
                    [-fanout]
                    [-off]
```

Options description:
- ``pin``: name of the instance or block pin.
- ``fanin``: (optional) display the fanin timing cone.
- ``fanout``: (optional) display the fanout timing cone.
- ``off``: (optional) remove the timing cone.

### Limit drawing to specific nets

```
focus_net net
          [-remove]
          [-clear]
```

Options description:
- ``pin``: name of the net.
- ``remove``: (optional) removes the net from from the focus.
- ``clear``: (optional) clears all nets from focus.

## TCL functions

### Support

Determine is the GUI is active:

```
gui::enabled
```

Announce to the GUI that a design was loaded 
(note: this is only needed when the design was loaded through the odb API and not via ``read_def`` or ``read_db``):

```
gui::design_created
```

To load the results of a DRC report:

```
gui::load_drc filename
```

### Opening and closing

To open the GUI from the command-line (this command does not return until the GUI is closed):

```
gui::show
gui::show script
gui::show script interactive
```

Options description:
- ``script`` TCL script to evaluate in the GUI.
- ``interactive`` indicates if true the GUI should open in an interactive session (default), or if false that the GUI would execute the script and return to the terminal.

To close the GUI and return to the command-line:

```
gui::hide
```

### Layout navigation

To fit the whole layout in the window:

```
gui::fit
```

To zoom in our out to a specific region:

```
gui::zoom_to x0 y0 x1 y1
```

Options description:
- ``x0, y0`` first corner of the layout area in microns.
- ``x1, y1`` second corner of the layout area in microns.

To zoom in the layout:

```
gui::zoom_in
gui::zoom_in x y
```

Options description:
- ``x, y`` new center of layout.

To zoom out the layout:

```
gui::zoom_out
gui::zoom_out x y
```

Options description:
- ``x, y`` new center of layout.

To move the layout to new area:

```
gui::center_at x y
```

Options description:
- ``x, y`` new center of layout.

To change the resolution to a specific value:

```
gui::set_resolution resolution
```

Options description:
- ``resolution`` database units per pixel.

### Selections

To add a single net to the selected items:

```
gui::selection_add_net name
```

Options description:
- ``name`` name of the net to add.

To add several nets to the selected items:

```
gui::selection_add_nets name_regex
```

Options description:
- ``name_regex`` regular expression of the net names to add.

To add a single instance to the selected items:

```
gui::selection_add_inst name
```

Options description:
- ``name`` name of the instance to add.

To add several instances to the selected items:

```
gui::selection_add_insts name_regex
```

Options description:
- ``name_regex`` regular expression of the instance names to add.

To add items at a specific point or in an area:

```
gui::select_at x y
gui::select_at x y append
gui::select_at x0 y0 x1 y1
gui::select_at x0 y0 x1 y1 append
```

Options description:
- ``x, y`` point in the layout area in microns.
- ``x0, y0`` first corner of the layout area in microns.
- ``x1, y1`` second corner of the layout area in microns.
- ``append`` if ``true`` (the default value) append the new selections to the current selection list, else replace the selection list with the new selections.

To navigate through multiple selected items:

```
gui::select_next
gui::select_previous
```

Returns: current index of the selected item.

To clear the current set of selected items:

```
gui::clear_selections
```

To get the properties for the current selection in the Inspector:

```
gui::get_selection_property name
```

Options description:
- ``name`` name of the property. For example, ``Type`` for object type or ``bbox`` for the bounding box of the object.

To animate the current selection in the Inspector:

```
gui::selection_animate
gui::selection_animate repeat
```

Options description:
- ``repeat``: indicate how many times the animation should repeat, default value is 0 repeats. If the value is 0, the animation will repeat indefinitely.

### Highlighting

To highlight a net:

```
gui::highlight_net name
gui::highlight_net name highlight_group
```

Options description:
- ``name`` name of the net to highlight.
- ``highlight_group`` group to add the highlighted net to, defaults to ``0``, valid groups are ``0 - 7``.

To highlight an instance:

```
gui::highlight_inst name
gui::highlight_inst name highlight_group
```

Options description:
- ``name`` name of the instance to highlight.
- ``highlight_group`` group to add the highlighted instance to, defaults to ``0``, valid groups are ``0 - 7``.

To clear the highlight groups:

```
gui::clear_highlights
gui::clear_highlights highlight_group
```

Options description:
- ``highlight_group`` group to clear, defaults to ``0``, valid groups are ``-1 - 7``. Use ``-1`` to clear all groups.

### Rulers

To add a ruler to the layout:

1. either press ``k`` and use the mouse to place it visually.
To disable snapping for the ruler when adding, hold the ``Ctrl`` key, and to allow non-horizontal or vertical snapping when completing the ruler hold the ``Shift`` key.

2. or use the command:

```
gui::add_ruler x0 y0 x1 y1
gui::add_ruler x0 y0 x1 y1 label
gui::add_ruler x0 y0 x1 y1 label name
gui::add_ruler x0 y0 x1 y1 label name euclidian
```

Returns: name of the newly created ruler.

Options description: 
- ``x0, y0`` first end point of the ruler in microns.
- ``x1, y1`` second end point of the ruler in microns.
- ``label`` text label for the ruler.
- ``name`` name of the ruler.
- ``euclidian`` ``1`` for euclidian ruler, and ``0`` for regular ruler.

To remove a single ruler:

```
gui::delete_ruler name
```

Options description: 
- ``name`` name of the ruler.

To remove all the rulers:

```
gui::clear_rulers
```

### Heat Maps

The currently availble heat maps are:

- ``Power``
- ``Routing``
- ``Placement``
- ``IRDrop``
- ``RUDY`` [^RUDY]

To control the settings in the heat maps:

```
gui::set_heatmap name option
gui::set_heatmap name option value
```

Options description:
- ``name`` is the name of the heatmap.
- ``option`` is the name of the option to modify. If option is ``rebuild`` the map will be destroyed and rebuilt.
- ``value`` is the new value for the specified option. This is not used when rebuilding map.

These options can also be modified in the GUI by double-clicking the underlined display control for the heat map.


To save the raw data from the heat maps ins a comma separated value (CSV) format:

```
gui::dump_heatmap name filename
```

Options description: 
- ``name`` is the name of the heatmap.
- ``filename`` path to the file to write the data to.

[^RUDY]: RUDY means Rectangular Uniform wire DensitY, which can predict the routing density very rough and quickly. You can see this notion in [this paper](https://past.date-conference.com/proceedings-archive/2007/DATE07/PDFFILES/08.7_1.PDF) 



### GUI Display Controls

Control the visible and selected elements in the layout:

```
gui::set_display_controls name display_type value
```

Options description: 
- ``name`` is the name of the control. For example, for the power nets option this would be ``Signals/Power`` or could be ``Layers/*`` to set the option for all the layers.
- ``display_type`` is either ``visible`` or ``selectable``
- ``value`` is either ``true`` or ``false``

To check the visibility or selectability of elements in the layout:

```
gui::check_display_controls name display_type 
```

Options description: 
- ``name`` is the name of the control. For example, for the power nets option this would be ``Signals/Power`` or could be ``Layers/*`` to set the option for all the layers.
- ``display_type`` is either ``visible`` or ``selectable``


When performing a batch operation changing the display controls settings, the following commands can be used to save the current state of the display controls and restore them at the end.

```
gui::save_display_controls
gui::restore_display_controls
```

### GUI Controls

To request user input via the GUI:

```
gui::input_dialog title question
```

Returns: a string with the input, or empty string if canceled.

Options description: 
- ``title`` is the title of the input message box.
- ``question`` is the text for the message box.

Pause the execution of the script:

```
gui::pause 
gui::pause timeout
```

Options description: 
- ``timeout`` is specified in milliseconds, if it is not provided the pause will last until the user presses the Continue button.

To open or close a specific layout widget:

```
gui::show_widget name
gui::hide_widget name
```

Options description: 
- ``name`` of the widget. For example, the display controls would be "Display Control".

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
