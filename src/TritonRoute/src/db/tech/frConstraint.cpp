/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

//#include "frConstraint.h"
//
//using namespace fr;
//
//void frNonDefaultRule::setWidth(frCoord w, int z){
//    if (z >= widths_.size())
//        for (int i = widths_.size(); i <= z; i++)
//            widths_.push_back(-1);
//    widths_[z] = w;
//}
//void frNonDefaultRule::setSpacing(frCoord s, int z){
//    if (z >= spacings_.size())
//        for (int i = spacings_.size(); i <= z; i++)
//            spacings_.push_back(-1);
//    spacings_[z] = s;
//}
//
//void frNonDefaultRule::setWireExtension(frCoord we, int z){
//    if (z >= wireExtensions_.size())
//        for (int i = wireExtensions_.size(); i <= z; i++)
//            wireExtensions_.push_back(-1);
//    wireExtensions_[z] = we;
//}
//
//void frNonDefaultRule::addVia(frViaDef* via, int z){
//    if (z >= vias_.size())
//        for (int i = vias_.size(); i <= z; i++)
//            vias_.push_back(vector<frViaDef*>());
//    vias_[z].push_back(via);
//}
//
//void frNonDefaultRule::addViaRule(frViaRuleGenerate* via, int z){
//    if (z >= viasRules_.size())
//        for (int i = viasRules_.size(); i <= z; i++)
//            viasRules_.push_back(vector<frViaRuleGenerate*>());
//    viasRules_[z].push_back(via);
//}