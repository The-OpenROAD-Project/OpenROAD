/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////


#include <iostream>
#include "TclInterface.h"
#include "Parameters.h"
#include "IOPlacementKernel.h"

namespace ioPlacer {

Parameters* parmsToIOPlacer = new Parameters();
IOPlacementKernel* ioPlacerKernel = new IOPlacementKernel(*parmsToIOPlacer);

void import_lef(const char* file){
        std::cout << " > Importing LEF file \"" << file << "\"\n";
        ioPlacerKernel->parseLef(file);
}

void import_def(const char* file){
        std::cout << " > Importing DEF file \"" << file << "\"\n";
        parmsToIOPlacer->setInputDefFile(file);
        ioPlacerKernel->parseDef(file);
}

void set_hor_metal_layer(int layer){
        parmsToIOPlacer->setHorizontalMetalLayer(layer);
}

int get_hor_metal_layer() {
        return parmsToIOPlacer->getHorizontalMetalLayer();
}

void set_ver_metal_layer(int layer){
       parmsToIOPlacer->setVerticalMetalLayer(layer); 
}

int get_ver_metal_layer(){
        return parmsToIOPlacer->getVerticalMetalLayer();
}

void set_num_slots(int numSlots){
        parmsToIOPlacer->setNumSlots( numSlots );
}

int get_num_slots(){
        return parmsToIOPlacer->getNumSlots();
}

void set_random_mode(int mode){
       parmsToIOPlacer->setRandomMode(mode); 
}

int get_random_mode(){
        return parmsToIOPlacer->getRandomMode();
}

void set_slots_factor(float factor){
        parmsToIOPlacer->setSlotsFactor(factor); 
}

float get_slots_factor(){
        return parmsToIOPlacer->getSlotsFactor();
}

void set_force_spread(bool force) {
        parmsToIOPlacer->setForceSpread(force);
}

bool get_force_spread() {
        return parmsToIOPlacer->getForceSpread();
}

void set_usage(float usage){
        parmsToIOPlacer->setUsage(usage); 
}

float get_usage() {
        return parmsToIOPlacer->getUsage();
}

void set_usage_factor(float usage){
        parmsToIOPlacer->setUsageFactor(usage);
}

float get_usage_factor(){
        return parmsToIOPlacer->getUsageFactor();       
}

void set_blockages_file(const char* file){
        parmsToIOPlacer->setBlockagesFile(file);
}

const char* get_blockages_file(){
        return parmsToIOPlacer->getBlockagesFile().c_str();
}

void add_blocked_area(long long int llx, long long int lly, long long int urx, long long int ury) {
        ioPlacerKernel->addBlockedArea(llx, lly, urx, ury);
}

void  set_hor_length(float length){
        parmsToIOPlacer->setHorizontalLength(length);
}

float get_hor_length(){
        return parmsToIOPlacer->getHorizontalLength();
}

void  set_ver_length_extend(float length){
        parmsToIOPlacer->setVerticalLengthExtend(length);
}

void  set_hor_length_extend(float length){
        parmsToIOPlacer->setHorizontalLengthExtend(length);
}

void  set_ver_length(float length){
        parmsToIOPlacer->setVerticalLength(length);
}

float get_ver_length(){
        return parmsToIOPlacer->getVerticalLength();
}

void  set_interactive_mode(bool enable){
        parmsToIOPlacer->setInteractiveMode(enable);
}

bool is_interactive_mode(){
       return parmsToIOPlacer->isInteractiveMode(); 
}

void print_all_parms(){
        parmsToIOPlacer->printAll();
}

void run_io_placement(){
        ioPlacerKernel->run();
}

void set_report_hpwl(bool report){
        parmsToIOPlacer->setReportHPWL(report);
}

bool get_report_hpwl(){
        return parmsToIOPlacer->getReportHPWL();
}

int compute_io_nets_hpwl(){
        return ioPlacerKernel->returnIONetsHPWL();
}

void export_def(const char* file){
        parmsToIOPlacer->setOutputDefFile(file);
        ioPlacerKernel->writeDEF();
}

void set_num_threads(int numThreads){;
        parmsToIOPlacer->setNumThreads(numThreads);
}

int get_num_threads(){
        return parmsToIOPlacer->getNumThreads();
}

void   set_rand_seed(double seed){
        parmsToIOPlacer->setRandSeed(seed);
}

double get_rand_seed(){
        return parmsToIOPlacer->getRandSeed();
}

void  set_hor_thick_multiplier(float length) {
        parmsToIOPlacer->setHorizontalThicknessMultiplier(length);
}

void  set_ver_thick_multiplier(float length) {
        parmsToIOPlacer->setVerticalThicknessMultiplier(length);
}

float  get_ver_thick_multiplier() {
        return parmsToIOPlacer->getVerticalThicknessMultiplier();
}

float  get_hor_thick_multiplier() {
        return parmsToIOPlacer->getHorizontalThicknessMultiplier();
}

float  get_hor_length_extend() {
        return parmsToIOPlacer->getHorizontalLengthExtend();
}

float  get_ver_length_extend() {
        return parmsToIOPlacer->getVerticalLengthExtend();
}

void set_boundaries_offset(int offset) {
        parmsToIOPlacer->setBoundariesOffset(offset);
}

void set_min_distance(int minDist) {
    parmsToIOPlacer->setMinDistance(minDist);
}

}
