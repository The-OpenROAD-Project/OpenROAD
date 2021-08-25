set -e

BASE_DIR=$(dirname $0)

PYTHON_EXT="./$BASE_DIR/../../../build/src/openroad -python"

echo "Running tests .."
echo ""

files=$(find $BASE_DIR/unitTestsPython -name "Test*.py")
for file in $files
do
    name=$(echo $file | awk -F"/" '{print $NF}')
    echo "$name"
    echo ""
    $PYTHON_EXT $BASE_DIR/unitTestsPython/$name
    echo ""
done