// The Maze Routing Code has been cleaned
// See the working directory:  /home/fetzfs_projects/TritonRouteGPU/flex_gr_branch/OpenROAD/src/drt/src/gr


2025/06/19
1. Move the original FlexGR_init.cpp to FlexGR_init.cpp.delete
2. Flex_init.cpp -> Flex_init.cpp + FlexGRWorker_init.cpp
3. Change the GCell size to from M1 X 15 to M2 X 15

2025/07/21
// We reuse some of the codes from previous versions
0.1 FlexGR_init_gpu.cpp: prepare the initial congestion map and grid structure
1. please only use the code with _gpu postfix
2. Main function:  FlexGR_gpu.cpp
   GPU DB:  FlexGR_GPUDB.h  FlexGR_GPUDB.cpp  FlexGR_GPUDB.cu
3. initial routing: FlexGR_initRoute_gpu.cpp
4. layer assignment:  FlexGR_layerAssign_gpu.cpp  FlexGR_GPUDB_layerAssign.cu
5. 2D Maze Routing:
   FlexGRWorker_init_gpu.cpp
   FlexGRWorker_end_gpu.cpp
