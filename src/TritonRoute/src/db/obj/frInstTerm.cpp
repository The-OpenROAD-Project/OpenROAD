/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "frInstTerm.h"

std::string fr::frInstTerm::getFullName(){
    return term_->getName()+"."+inst_->getName();
}