mkdir -p /OpenROAD/build
cd /OpenROAD/build
cmake ..
make clean
make -j 4
