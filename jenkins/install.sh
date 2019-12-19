rm -rf /OpenROAD/build
mkdir -p /OpenROAD/build
cd /OpenROAD/build
cmake ..
make -j 4
