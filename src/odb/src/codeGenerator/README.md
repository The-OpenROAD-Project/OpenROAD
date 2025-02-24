# Automatic Code Generator

This is an automatic code generation tool for OpenDB objects and Iterators. To test the tool you can use the following command

``` shell
python3 gen.py
```

Empty sections are removed by default from the output.  If you need to add someting to a section that is currently empty, you can run the generator with --keep_empty to preserve them.  Once the section is filled in, the flag can be dropped and the code regnerated to remove the remaining empty sections.
