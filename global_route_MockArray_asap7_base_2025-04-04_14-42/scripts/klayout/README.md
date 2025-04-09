# Collection of Ruby scripts to generate KLayout files required by ORFS

This directory contains a set of Ruby scripts that will generate the following KLayout files:

- Technology File
- Layer Properties File

using the following files as input:

- Cadence Virtuoso SKILL tech file
- Cadence Virtuoso layer mapping file
- Technology LEF file

## Setup
Copy the following files into ~/.klayout/ruby

- GenericLayerNameMapper.rb
- KLayoutLayerMapGenerator.rb
- KLayoutLayerPropertiesFileGenerator.rb

Also make sure that you have the import_tf.rb file from [tf_import](https://github.com/klayoutmatthias/tf_import) copied there as well. If you follow the instructions at [Guide to Integrate a New Platform into the OpenROAD Flow](https://openroad-flow-scripts.readthedocs.io/en/latest/contrib/PlatformBringUp.html#klayout-properties-file), it will be installed in ~/.klayout/salt/tf_import. Just copy it over to ~/.klayout/ruby, so that the scripts can find it.

## Usage
Then gen_klayout_files.sh is the main driver to generate the files. Its usage is:

```
Usage: gen_klayout_files.sh -v <virtuoso_tech_file> -l <virtuoso_layer_map>
                            -t <tech_lef> -n <tech_name>
                            -d <tech_description> -p <klayout_lyp>
                            -o <klayout_lyt> [-m layer_name_mapper_ruby] [-h]
```

where

- virtuoso_tech_file: Cadence Virtuoso SKILL tech file
- virtuoso_layer_map: Cadence Virtuoso layer mapping file (maps Cadence
                     layer/purpose pairs to GDS II layer/datatype)
- tech_lef: Cadence technology LEF with layer and via definitions
- tech_name: Name to put in KLayout technology file
- tech_description: Description to put in KLayout technology file
- klayout_lyp: KLayout layer properties file (typically ending in .lyp) -
              generated from Virtuoso tech file
- klayout_lyt: Klayout technology file (typically ending in .lyt)
- layer_name_mapper_ruby: Name of Ruby file containing custom layer name mapper
                         Uses GenericLayerNameMapper.rb by default

For example:

```
gen_klayout_files.sh -v process.tf -l process.layermap -d adv_process.lef \
                     -n adv_process -d "Really advanced process" -p output.lyp \
                     -o output.lyt -m AdvProcessLayerNameMapper.rb
```



