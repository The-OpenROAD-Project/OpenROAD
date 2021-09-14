# Graphical User Interface

The graphical user interface can be access by launching OpenROAD with ``-gui``.

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

## TCL functions

### Support

Determine is the GUI is active:

```
gui::enabled
```

Announce to the GUI that a design was loaded:

```
gui::design_created
```

To load the results of a DRC report:

```
gui::load_drc filename
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
- ``x, y`` new center of layout

To zoom out the layout:

```
gui::zoom_out
gui::zoom_out x y
```

Options description:
- ``x, y`` new center of layout

To move the layout to new area:

```
gui::center_at x y
```

Options description:
- ``x, y`` new center of layout

To change the resolution to a specific value:

```
gui::set_resolution resolution
```

Options description:
- ``resolution`` database units per pixel

### Selections

To add a single net to the selected items:

```
gui::selection_add_net name
```

Options description:
- ``name`` name of the net to add

To add several nets to the selected items:

```
gui::selection_add_nets name
```

Options description:
- ``name`` regular expression of the net names to add

To add a single instance to the selected items:

```
gui::selection_add_inst name
```

Options description:
- ``name`` name of the instance to add

To add several instances to the selected items:

```
gui::selection_add_insts name
```

Options description:
- ``name`` regular expression of the instance names to add

To clear the current set of selected items:

```
gui::clear_selections
```

### Highlighting

To highlight a net:

```
gui::highlight_net name
gui::highlight_net name highlight_group
```

Options description:
- ``name`` name of the net to highlight
- ``highlight_group`` group to add the highlighted net to, defaults to ``0``, valid groups are ``0 - 7``.

To highlight an instance:

```
gui::highlight_inst name
gui::highlight_inst name highlight_group
```

Options description:
- ``name`` name of the instance to highlight
- ``highlight_group`` group to add the highlighted instance to, defaults to ``0``, valid groups are ``0 - 7``.

To clear the highlight groups:

```
gui::clear_highlights
gui::clear_highlights highlight_group
```

Options description:
- ``highlight_group`` group to clear, defaults to ``0``, valid groups are ``-1 - 7``. Use ``-1`` to clear all groups.

### Save layout images

To save a picture of the currently visible layout use:

```
gui::save_image filename
gui::save_image filename x0 y0 x1 y1
```

Options description:
- ``filename`` path to save the image to.
- ``x0, y0`` first corner of the layout area (in microns) to be saved.
- ``x1, y1`` second corner of the layout area (in microns) to be saved.

### Rulers

To add a ruler to the layout:

a) either hold the ``Crtl`` button and use the right mouse button to place it visually.
To disable snapping for the ruler when adding, hold the ``Ctrl`` key, and to allow non-horizontal or vertical snapping when completing the ruler hold the ``Shift`` key.

b) or use the command:

```
gui::add_ruler x0 y0 x1 y1
gui::add_ruler x0 y0 x1 y1 label
gui::add_ruler x0 y0 x1 y1 label name
```

Returns: name of the newly created ruler

Options description: 
- ``x0, y0`` first end point of the ruler in microns.
- ``x1, y1`` second end point of the ruler in microns.
- ``label`` text label for the ruler.
- ``name`` name of the ruler

To remove a single ruler:

```
gui::delete_ruler name
```

Options description: 
- ``name`` name of the ruler

To remove all the rulers:

```
gui::clear_rulers
```

### GUI Controls

Control the visible and selected elements in the layout:

```
gui::set_display_controls name display_type value
```

Options description: 
- ``name`` is the name of the control. For example, for the power nets option this would be ``Signals/Power``.
- ``display_type`` is either ``visible`` or ``selectable``
- ``value`` is either ``true`` or ``false``

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
