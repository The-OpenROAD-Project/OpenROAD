#!/bin/bash
#
# Bash wrapper script to read PDK files and generate the KLayout layer
# properties and technology file used the 6_final step of OpenROAD-flow-scripts
#
# Usage: gen_klayout_files.sh -v <virtuoso_tech_file> -l <virtuoso_layer_map>
#                             -t <tech_lef> -n <tech_name>
#                             -d <tech_description> -p <klayout_lyp>
#                             -o <klayout_lyt> [-m layer_name_mapper_ruby] [-h]
#
# where
#  virtuoso_tech_file: Cadence Virtuoso SKILL tech file
#  virtuoso_layer_map: Cadence Virtuoso layer mapping file (maps Cadence
#                      layer/purpose pairs to GDS II layer/datatype)
#  tech_lef: Cadence technology LEF with layer and via definitions
#  tech_name: Name to put in KLayout technology file
#  tech_description: Description to put in KLayout technology file
#  klayout_lyp: KLayout layer properties file (typically ending in .lyp) -
#               generated from Virtuoso tech file
#  klayout_lyt: Klayout technology file (typically ending in .lyt)
#  layer_name_mapper_ruby: Name of Ruby file containing custom layer name mapper
#                          Uses GenericLayerNameMapper.rb by default
#
# KLayout doesn't support passing script-specific command line arguments, so we
# translate this script's command line arguments into environment variables to
# pass to the KLayout Ruby scripts we call
#

#
# Prints the help usage
#
PrintHelp()
{
    echo "Usage: gen_klayout_files.sh -v <virtuoso_tech_file> -l <virtuoso_layer_map>"
    echo "                            -t <tech_lef> -n <tech_name> -d <tech_description>"
    echo "                            -p <klayout_lyp> -o <klayout_lyt>"
    echo "                            [-m <ruby_layer_name_mapper>"
}

#
# Clear the env variables, in case they are set.
#
# Could cache previous values and set them back to their previous values in the
# future
#
ClearEnvVars()
{
    unset VIRTUOSO_TECH_FILE
    unset VIRTUOSO_LAYER_MAP_FILE
    unset TECH_LEF
    unset KLAYOUT_LAYER_PROPERTIES_FILE
    unset KLAYOUT_TECH_FILE
    unset TECH_NAME
    unset TECH_DESC
    unset LAYER_NAME_MAPPER
}

#
# Parse args and set environment variables
#
GetOptionsAndSetEnvVars()
{
    export LAYER_NAME_MAPPER=GenericLayerNameMapper.rb
    local OPTIND
    while getopts "hv:l:t:n:d:p:o:m:" option; do
	case $option in
	    h)
		PrintHelp
		exit 1;;
	    v)
		export VIRTUOSO_TECH_FILE=$OPTARG
		;;
	    l)
		export VIRTUOSO_LAYER_MAP_FILE=$OPTARG
		;;
	    t)
		export TECH_LEF=$OPTARG
		;;
	    n)
		export TECH_NAME=$OPTARG
		;;
	    d)
		export TECH_DESC=$OPTARG
		;;
	    p)
		export KLAYOUT_LAYER_PROPERTIES_FILE=$OPTARG
		;;
	    o)
		export KLAYOUT_TECH_FILE=$OPTARG
		;;
	    m)
		export LAYER_NAME_MAPPER=$OPTARG
		;;
	    \?)
		echo "Error: Invalid option"
		exit 1;;
	esac
    done
    shift $((OPTIND-1))
}

#
# Check that all required env vars are set
#
CheckRequiredEnvVars()
{
    if [ -z "${VIRTUOSO_TECH_FILE}" ] || [ -z "${VIRTUOSO_LAYER_MAP_FILE}" ] ||
       [ -z "${TECH_LEF}" ] || [ -z "${KLAYOUT_LAYER_PROPERTIES_FILE}" ] ||
       [ -z "${KLAYOUT_TECH_FILE}" ] || [ -z  "${TECH_NAME}" ] ||
       [ -z  "${TECH_DESC}" ]; then
	echo "Error: Missing a required option"
	PrintHelp
	exit 1
    fi
}

#
# Echoes env vars and values
#
EchoEnvVars()
{
    echo "VIRTUOSO_TECH_FILE=${VIRTUOSO_TECH_FILE}"
    echo "VIRTUOSO_LAYER_MAP_FILE=${VIRTUOSO_LAYER_MAP_FILE}"
    echo "TECH_LEF=${TECH_LEF}"
    echo "KLAYOUT_LAYER_PROPERTIES_FILE=${KLAYOUT_LAYER_PROPERTIES_FILE}"
    echo "KLAYOUT_TECH_FILE=${KLAYOUT_TECH_FILE}"
    echo "TECH_NAME=${TECH_NAME}"
    echo "TECH_DESC=${TECH_DESC}"
    echo "LAYER_NAME_MAPPER=${LAYER_NAME_MAPPER}"
}

#
# Check that Ruby scripts that we depend on are in place
#
CheckRequiredKLayoutScripts()
{
    is_error=0
    klayout_file_path=~/.klayout/ruby
    script_list=("LEFViaData.rb" "KLayoutLayerMapGenerator.rb" "import_tf.rb" "KLayoutLayerPropertiesFileGenerator.rb")
    script_list+=("${LAYER_NAME_MAPPER}")
    for script_name in "${script_list[@]}"; do
	file_path=${klayout_file_path}/${script_name}
	if [ ! -e $file_path ]; then
	    echo "Error: Missing KLayout script: $file_path"
	    is_error=1
	fi
    done
    if [ $is_error -eq 1 ]; then
        echo "Info: Please install missing scripts using ORFS instructions at https://openroad-flow-scripts.readthedocs.io/en/latest/contrib/PlatformBringUp.html#klayout-properties-file"
	exit 1;
    fi
}

#
# Calls the KLayout scripts to generate the layer properties and tech files
#
CallKLayoutScripts()
{
    pwd=$(dirname "$0")

    $pwd/KLayoutLayerPropertiesFileGenerator.rb
    $pwd/KLayoutTechFileGenerator.rb
}

#
# Steps to execute
#
ClearEnvVars
GetOptionsAndSetEnvVars "$@"
CheckRequiredEnvVars
CheckRequiredKLayoutScripts
CallKLayoutScripts
ClearEnvVars

exit 0
