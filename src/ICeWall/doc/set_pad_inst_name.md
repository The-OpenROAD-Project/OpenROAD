## ICeWall set_pad_inst_name
### Synopsys
```
  % ICeWall set_pad_inst_name <format_string>
```
### Description
This can be optionally set to specify the name of padcell instances based upon the given format string. The format string is expected to include %s somewhere within the string, which will be replaced by the signal name associated with each pad instance.
### Examples
```
ICeWall set_pad_inst_name "%s"
```
