
set -e

BASE_DIR=$(dirname $0)
SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

files=$(find $BASE_DIR/../build/tests/cpp -maxdepth 1 -name "Test*")
for file in $files
do
    name=$(echo $file | awk -F"/" '{print $NF}')
    echo "$name"
    echo ""
    BASE_DIR=$SCRIPTPATH $BASE_DIR/../build/tests/cpp/$name
    echo ""
done
