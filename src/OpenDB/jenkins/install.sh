mkdir -p /OpenDB/build
cd /OpenDB/build
cmake -DBUILD_PYTHON=ON -DBUILD_TCL=ON ..
make