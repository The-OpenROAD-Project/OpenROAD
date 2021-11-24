set -e

BASE_DIR=$(dirname $0)

export PYTHONPATH=$BASE_DIR/../build/src/swig/python:$PYTHONPATH

echo "Running tests .."
echo ""
if [[ $1 = "parallel" ]]
then
    unittest-parallel -s $BASE_DIR/unitTestsPython/ -p 'Test*.py' -q
else
    files=$(find $BASE_DIR/unitTestsPython -name "Test*.py")
    for file in $files
    do
        name=$(echo $file | awk -F"/" '{print $NF}')
        echo "$name"
        echo ""
        python3 $BASE_DIR/unitTestsPython/$name
        echo ""
    done
fi