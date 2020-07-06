// BSD 3-Clause License
//
// Copyright (c) 2020, MICL, DD-Lab, University of Michigan
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


#include <stdio.h>
#include <iostream>
#include <cstring>
#include "antennachecker/AntennaChecker.hh"
#include "sta/StaMain.hh"
#include "opendb/db.h"
#include "opendb/dbTypes.h"
#include "opendb/dbWireGraph.h"
#include <time.h>

namespace sta {
// Tcl files encoded into strings.
extern const char *antennachecker_tcl_inits[];
}

namespace antenna_checker {

using odb::dbSet;
using odb::dbNet;
using odb::dbITerm;
using odb::dbMTerm;
using odb::dbBTerm;
using odb::dbInst;
using odb::dbWire;
using odb::dbTechLayer;
using odb::dbWireType;
using odb::dbTechAntennaPinModel;
using odb::dbWireGraph;
using odb::dbTechVia;
using odb::dbVia;
using odb::dbBox;
using odb::dbTechLayerType;
using odb::dbViaParams;
using odb::dbViaParams;
using odb::dbTechLayerAntennaRule;

extern "C" {
extern int Antennachecker_Init(Tcl_Interp *interp);
}


AntennaChecker::AntennaChecker()
{
}

AntennaChecker::~AntennaChecker()
{
}

template <class valueType>
double
AntennaChecker::defdist( valueType value ) 
{
    double _dist_factor = 1.0 / (double) db_->getChip()->getBlock()->getDbUnitsPerMicron();
    return ((double) value) * _dist_factor;
}

dbNet*
AntennaChecker::get_net( std::string net_name )
{
    return  db_->getChip()->getBlock()->findNet(net_name.c_str());
}

void
AntennaChecker::load_antenna_rules()
{

    odb::dbTech* tech = db_->getTech();
    odb::dbSet<odb::dbTechLayer> tech_layers = tech->getLayers();

    odb::dbSet<odb::dbTechLayer>::iterator itr;
    for (itr = tech_layers.begin(); itr != tech_layers.end(); ++itr)
    {
        odb::dbTechLayer* tech_layer = (odb::dbTechLayer*) *itr;

        double metal_factor = 1.0;
        double diff_metal_factor = 1.0;

        double cut_factor = 1.0;
        double diff_cut_factor = 1.0;

        double side_metal_factor = 1.0;
        double diff_side_metal_factor = 1.0;

        double minus_diff_factor = 0.0;
        double plus_diff_factor = 0.0;
        double diff_metal_reduce_factor = 1.0;


        if ( tech_layer->hasDefaultAntennaRule() )
        {
            dbTechLayerAntennaRule* antenna_rule = tech_layer->getDefaultAntennaRule();

            odb::dbStringProperty* layer_prop = odb::dbStringProperty::find(tech_layer, "LEF57_ANTENNAGATEPLUSDIFF");
            if (layer_prop != nullptr)
            {
                std::string gate_plus_diff_info = layer_prop->getValue();
                int start = 0, end = 0;
                std::string gate_plus_diff_value = "";
                for( int i = 0; i < gate_plus_diff_info.size(); i++ )
                {
                    if ( gate_plus_diff_info.at(i) == ' ' )
                    {
                        gate_plus_diff_value = gate_plus_diff_info.substr(start, i-start-1);
                        start = i;
                    }
                }
                plus_diff_factor = std::stod(gate_plus_diff_value);
            }
	    if ( antenna_rule->isAreaFactorDiffUseOnly() )
            {
                diff_metal_factor = antenna_rule->getAreaFactor();

                diff_cut_factor = antenna_rule->getAreaFactor();
            }
            else
            {
                metal_factor = antenna_rule->getAreaFactor();
                diff_metal_factor = antenna_rule->getAreaFactor();

                cut_factor = antenna_rule->getAreaFactor();
                diff_cut_factor = antenna_rule->getAreaFactor();
            }
            if ( antenna_rule->isSideAreaFactorDiffUseOnly() )
            {
                diff_side_metal_factor = antenna_rule->getSideAreaFactor();
            }
            else
            {
                side_metal_factor = antenna_rule->getSideAreaFactor();
                diff_side_metal_factor = antenna_rule->getSideAreaFactor();
            }

            minus_diff_factor = antenna_rule->getAreaMinusDiffFactor();

        }

        ANTENNAmodel layer_antenna = {
            tech_layer,
            metal_factor,
            diff_metal_factor,
            cut_factor,
            diff_cut_factor,
            side_metal_factor,
            diff_side_metal_factor,
            minus_diff_factor,
            plus_diff_factor,
            diff_metal_reduce_factor
        };
        layer_info[tech_layer] = layer_antenna;
    }

}

std::pair<dbWireGraph::Node*, std::vector<dbWireGraph::Node*>> 
AntennaChecker::find_segment_root(std::pair<dbWireGraph::Node*, std::vector<dbWireGraph::Node*>> node_info, int wire_level )
{
    
    dbWireGraph::Node* node = node_info.first;
    std::vector<dbWireGraph::Node*> path_nodes = node_info.second;

    if ( !node->in_edge() ) 
        return node_info;
    
    path_nodes.push_back( node->in_edge()->source() ); 

    if ( node->in_edge()->type() == dbWireGraph::Edge::Type::VIA || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA )
    {
        auto new_root_info = find_segment_root( {node->in_edge()->source(), path_nodes}, wire_level );
        
        if ( node->in_edge()->source()->layer()->getRoutingLevel() > wire_level )
            return {node, new_root_info.second};

        if ( new_root_info.first->layer()->getRoutingLevel() == wire_level )
            return new_root_info;
        else
            return {node, new_root_info.second};
    }
    
    if ( node->in_edge()->type() == dbWireGraph::Edge::Type::SEGMENT || node->in_edge()->type() == dbWireGraph::Edge::Type::SHORT )
        return find_segment_root( { node->in_edge()->source(), path_nodes }, wire_level );
    
    return node_info;
}

dbWireGraph::Node * 
AntennaChecker::find_segment_start( dbWireGraph::Node * node )
{
    if( ( node->object() && strcmp( node->object()->getObjName(), "dbITerm" ) == 0 ) || !node->in_edge() )
        return node;
    else if ( node->in_edge()->type() == dbWireGraph::Edge::Type::VIA || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA )
        return node;
    else if ( node->in_edge()->type() == dbWireGraph::Edge::Type::SEGMENT || node->in_edge()->type() == dbWireGraph::Edge::Type::SHORT )
        return find_segment_start( node->in_edge()->source() );
    else
        return NULL;
}

bool
AntennaChecker::if_segment_root( dbWireGraph::Node * node, int wire_level )
{
    if( ( node->object() && strcmp( node->object()->getObjName(), "dbITerm" ) == 0 ) || !node->in_edge() )
        return true;
    else if ( node->in_edge()->type() == dbWireGraph::Edge::Type::VIA || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA )
    {
        if ( node->in_edge()->source()->layer()->getRoutingLevel() <= wire_level )
        {
            dbWireGraph::Node * current_root = node; 
            dbWireGraph::Node * new_root = find_segment_root( {node->in_edge()->source(), { node }}, wire_level ).first;
            if ( new_root->layer()->getRoutingLevel() == wire_level )
                return false;
            else
                return true;
        }
        else
            return true;
 }
    else
        return false;
}


void
AntennaChecker::find_wire_below_iterms( dbWireGraph::Node * node, double iterm_areas[2], int wire_level, std::set<dbITerm*>& iv, std::set<dbWireGraph::Node*>& nv )
{
    if ( check_iterm(node, iterm_areas) )
        iv.insert( dbITerm::getITerm( db_->getChip()->getBlock(), node->object()->getId() ) );

    nv.insert(node);

    if ( node->in_edge() && node->in_edge()->source()->layer()->getRoutingLevel() <= wire_level )
    {
        if ( (node->in_edge()->type() == dbWireGraph::Edge::Type::VIA || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA) && nv.find(node->in_edge()->source()) == nv.end() )
        {
            find_wire_below_iterms( find_segment_start( node->in_edge()->source() ), iterm_areas, wire_level, iv, nv);
        } else if ( (node->in_edge()->type() == dbWireGraph::Edge::Type::SEGMENT || node->in_edge()->type() == dbWireGraph::Edge::Type::SHORT ) && nv.find( node->in_edge()->source()) == nv.end() )
        {
            find_wire_below_iterms( node->in_edge()->source(), iterm_areas, wire_level, iv, nv );           
        }
    }

    dbWireGraph::Node::edge_iterator edge_itr;
    for ( edge_itr = node->begin(); edge_itr != node->end(); ++edge_itr )
    {
        if ( (*edge_itr)->type() == dbWireGraph::Edge::Type::VIA || (*edge_itr)->type() == dbWireGraph::Edge::Type::TECH_VIA )
        {
            if ( (*edge_itr)->target()->layer()->getRoutingLevel() <= wire_level && nv.find((*edge_itr)->target()) == nv.end())
            {
                find_wire_below_iterms( find_segment_start( (*edge_itr)->target() ), iterm_areas, wire_level, iv, nv );
            }
        }

        else if ( ((*edge_itr)->type() == dbWireGraph::Edge::Type::SEGMENT || (*edge_itr)->type() == dbWireGraph::Edge::Type::SHORT ) && nv.find((*edge_itr)->target()) == nv.end())
        {
            find_wire_below_iterms( (*edge_itr)->target(), iterm_areas, wire_level, iv, nv );
        }
    }
    return;
}

std::pair<double, double>
AntennaChecker::calculate_wire_area( dbWireGraph::Node * node, int wire_level, std::set<dbWireGraph::Node*>& nv, std::set<dbWireGraph::Node*>& level_nodes )
{
    double wire_area = 0;
    double side_wire_area = 0;

    double wire_width = defdist( node->layer()->getWidth() );

    uint wire_thickness_uint = 0;
    double wire_thickness = 0;
    node->layer()->getThickness(wire_thickness_uint);
    wire_thickness = defdist(wire_thickness_uint);

    int start_x, start_y;
    int end_x, end_y;
    node->xy(start_x, start_y);

    std::vector<std::pair<dbWireGraph::Edge*, std::string>> edge_vec;
    if (node->in_edge() != nullptr && nv.find(node->in_edge()->source()) == nv.end())
        edge_vec.push_back({node->in_edge(), "IN"});

    dbWireGraph::Node::edge_iterator edge_it;
    int out_edges_count = 0;

    for ( edge_it = node->begin(); edge_it != node->end(); edge_it++ )
    {
        if (nv.find((*edge_it)->source()) == nv.end())
        {
            out_edges_count++;
            edge_vec.push_back({*edge_it, "OUT"});
        }
    }

    nv.insert(node);

    for (auto edge_info: edge_vec)
    {
        dbWireGraph::Edge* edge = edge_info.first;
        std::string edge_io_type = edge_info.second;
        if (edge->type() == dbWireGraph::Edge::Type::VIA || edge->type() == dbWireGraph::Edge::Type::TECH_VIA)
        {
            if (edge_io_type.compare("IN") == 0)
            {
                wire_area += 0.5 * wire_width * wire_width;
                side_wire_area += defdist(wire_thickness_uint) * wire_width;

                if (edge->source()->layer()->getRoutingLevel() <= wire_level)
                {
                    std::pair<double, double> areas = calculate_wire_area(edge->source(), wire_level, nv, level_nodes);
                    wire_area += areas.first;
                    side_wire_area += areas.second;
                }
            }

            if (edge_io_type.compare("OUT") == 0)
            {
                if (out_edges_count == 1)
                {
                    wire_area += 0.5 * wire_width * wire_width;
                    side_wire_area += defdist(wire_thickness_uint) * wire_width;
                }

                if (edge->target()->layer()->getRoutingLevel() <= wire_level)
                {
                    std::pair<double, double> areas = calculate_wire_area(edge->target(), wire_level, nv, level_nodes);
                    wire_area += areas.first;
                    side_wire_area += areas.second;
                }
            }
        }

        if (edge->type() == dbWireGraph::Edge::Type::SEGMENT)
        {

            if (edge_io_type.compare("IN") == 0)
            {
                if (node->layer()->getRoutingLevel() == wire_level)
                {
                    level_nodes.insert(node);
                    edge->source()->xy(end_x, end_y);
                    wire_area += defdist( abs(end_x - start_x) + abs(end_y - start_y) ) * wire_width;
                    side_wire_area +=  ( defdist( abs(end_x - start_x) + abs(end_y - start_y) ) + wire_width ) * defdist(wire_thickness_uint) * 2;
                }

                std::pair<double, double> areas = calculate_wire_area(edge->source(), wire_level, nv, level_nodes);
                wire_area += areas.first;
                side_wire_area += areas.second;
            }


            if (edge_io_type.compare("OUT") == 0)
            {
                if (node->layer()->getRoutingLevel() == wire_level)
                {
                    level_nodes.insert(node);
                    edge->target()->xy(end_x, end_y);
                    wire_area += defdist( abs(end_x - start_x) + abs(end_y - start_y) ) * wire_width;
                    side_wire_area +=  ( defdist( abs(end_x - start_x) + abs(end_y - start_y) ) + wire_width ) * defdist(wire_thickness_uint) * 2;
                }

                std::pair<double, double> areas = calculate_wire_area(edge->target(), wire_level, nv, level_nodes);
                wire_area += areas.first;
                side_wire_area += areas.second;
            }
        }
    }
    return {wire_area, side_wire_area};
}

double
AntennaChecker::get_via_area( dbWireGraph::Edge * edge )
{
    double via_area = 0.0;
    if ( edge->type() == dbWireGraph::Edge::Type::TECH_VIA )
    {
        dbWireGraph::TechVia * tech_via_edge = ( dbWireGraph::TechVia * ) edge;
        dbTechVia * tech_via = tech_via_edge->via();
        dbSet<dbBox> tech_via_boxes = tech_via->getBoxes();
        dbSet<dbBox>::iterator box_itr;
        for ( box_itr = tech_via_boxes.begin(); box_itr != tech_via_boxes.end(); ++box_itr )
        {
            dbBox * box = *box_itr;
            if ( box->getTechLayer()->getType() == dbTechLayerType::CUT)
            {
                uint dx = box->getDX();
                uint dy = box->getDY();
                via_area = defdist( dx ) * defdist( dy );
            }
        }
    }
    else if ( edge->type() == dbWireGraph::Edge::Type::VIA )
    {
        dbWireGraph::Via * via_edge = ( dbWireGraph::Via * ) edge;
        dbVia * via = via_edge->via();
        dbSet<dbBox> via_boxes = via->getBoxes();
        dbSet<dbBox>::iterator box_itr;
        for ( box_itr = via_boxes.begin(); box_itr != via_boxes.end(); ++box_itr )
        {
            dbBox * box = *box_itr;
            if ( box->getTechLayer()->getType() == dbTechLayerType::CUT)
            {
                uint dx = box->getDX();
                uint dy = box->getDY();
                via_area = defdist( dx ) * defdist( dy );
            }
        }
    }
    return via_area;
}


dbTechLayer *
AntennaChecker::get_via_layer( dbWireGraph::Edge * edge )
{
    if ( edge->type() == dbWireGraph::Edge::Type::TECH_VIA )
    {
        dbWireGraph::TechVia * tech_via_edge = ( dbWireGraph::TechVia * ) edge;
        dbTechVia * tech_via = tech_via_edge->via();
        dbSet<dbBox> tech_via_boxes = tech_via->getBoxes();
        dbSet<dbBox>::iterator box_itr;
        for ( box_itr = tech_via_boxes.begin(); box_itr != tech_via_boxes.end(); ++box_itr )
        {
            dbBox * box = *box_itr;
            if ( box->getTechLayer()->getType() == dbTechLayerType::CUT )
                return box->getTechLayer();
        }
    }
    else if ( edge->type() == dbWireGraph::Edge::Type::VIA )
    {
        dbWireGraph::Via * via_edge = ( dbWireGraph::Via * ) edge;
        dbVia * via = via_edge->via();
        dbSet<dbBox> via_boxes = via->getBoxes();
        dbSet<dbBox>::iterator box_itr;
        for ( box_itr = via_boxes.begin(); box_itr != via_boxes.end(); ++box_itr )
        {
            dbBox * box = *box_itr;
            if ( box->getTechLayer()->getType() == dbTechLayerType::CUT )
                return box->getTechLayer();
        }
    }
    return nullptr;
}

std::string
AntennaChecker::get_via_name( dbWireGraph::Edge * edge )
{
    if ( edge->type() == dbWireGraph::Edge::Type::TECH_VIA )
    {
        dbWireGraph::TechVia * tech_via_edge = ( dbWireGraph::TechVia * ) edge;
        dbTechVia * tech_via = tech_via_edge->via();
        return tech_via->getName();
    }
    else if ( edge->type() == dbWireGraph::Edge::Type::VIA )
    {
        dbWireGraph::Via * via_edge = ( dbWireGraph::Via * ) edge;
        dbVia * via = via_edge->via();
        return via->getName();
    }
    return nullptr;
}

double
AntennaChecker::calculate_via_area( dbWireGraph::Node * node, int wire_level )
{
    double via_area = 0.0;
    if ( node->in_edge() && ( node->in_edge()->type() == dbWireGraph::Edge::Type::VIA || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA ))
        if ( node->in_edge()->source()->layer()->getRoutingLevel() > wire_level )
            via_area = via_area + get_via_area(node->in_edge());

    dbWireGraph::Node::edge_iterator edge_itr;
    for ( edge_itr = node->begin(); edge_itr != node->end(); ++edge_itr )
    {
        if ( (*edge_itr)->type() == dbWireGraph::Edge::Type::SEGMENT || (*edge_itr)->type() == dbWireGraph::Edge::Type::SHORT )
        {
            via_area = via_area + calculate_via_area( (*edge_itr)->target(), wire_level );
        }
        else if ( (*edge_itr)->type() == dbWireGraph::Edge::Type::VIA || (*edge_itr)->type() == dbWireGraph::Edge::Type::TECH_VIA )
        {
            if ( (*edge_itr)->target()->layer()->getRoutingLevel() > wire_level )
            {
                via_area = via_area + get_via_area((*edge_itr));
            }
            else
            {
                via_area = via_area + calculate_via_area( (*edge_itr)->target(), wire_level );
            }
        }
    }
    return via_area;
}

dbWireGraph::Edge *
AntennaChecker::find_via( dbWireGraph::Node * node, int wire_level )
{
    if ( node->in_edge() && ( node->in_edge()->type() == dbWireGraph::Edge::Type::VIA || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA ))
        if ( node->in_edge()->source()->layer()->getRoutingLevel() > wire_level )
        {
            return node->in_edge();
        }
    dbWireGraph::Node::edge_iterator edge_itr;
    for ( edge_itr = node->begin(); edge_itr != node->end(); ++edge_itr )
    {
        if ( (*edge_itr)->type() == dbWireGraph::Edge::Type::SEGMENT || (*edge_itr)->type() == dbWireGraph::Edge::Type::SHORT )
        {
            dbWireGraph::Edge * via = find_via( (*edge_itr)->target(), wire_level );
            if ( via )
                return via;
        }
        else if ( (*edge_itr)->type() == dbWireGraph::Edge::Type::VIA || (*edge_itr)->type() == dbWireGraph::Edge::Type::TECH_VIA )
        {
            if ( (*edge_itr)->target()->layer()->getRoutingLevel() > wire_level )
            {
                return (*edge_itr);
            }
            else
            {
                dbWireGraph::Edge * via = find_via( (*edge_itr)->target(), wire_level );
                if ( via )
                    return via;
            }
        }
    }
    return nullptr;
}


void
AntennaChecker::find_car_path( dbWireGraph::Node * node, int wire_level, dbWireGraph::Node * goal, std::vector<dbWireGraph::Node *> &current_path, std::vector<dbWireGraph::Node *> &path_found )
{
    current_path.push_back( node );

    if ( node == goal )
    {
        std::vector<dbWireGraph::Node *>::iterator current_itr;
        std::vector<dbWireGraph::Node *>::iterator found_itr;

        for ( current_itr = current_path.begin(); current_itr != current_path.end(); ++current_itr )
        {
            bool existed_node = 0;
            for ( found_itr = path_found.begin(); found_itr != path_found.end(); ++found_itr )
            {
                if ( (*current_itr) == (*found_itr) )
                {
                    existed_node = 1;
                    break;
                }
            }
            if ( existed_node == 0 )
                path_found.push_back( (*current_itr) );
        }
        current_path.pop_back();
        return;
    }
    else
    {
        if ( node->in_edge() && ( node->in_edge()->type() == dbWireGraph::Edge::Type::VIA || node->in_edge()->type() == dbWireGraph::Edge::Type::TECH_VIA ) )
            if ( node->in_edge()->source()->layer()->getRoutingLevel() < node->in_edge()->target()->layer()->getRoutingLevel() )
            {
                auto root_info = find_segment_root( {node->in_edge()->source(), { node->in_edge()->source() }}, node->in_edge()->source()->layer()->getRoutingLevel() );
                find_car_path( root_info.first, node->in_edge()->source()->layer()->getRoutingLevel(),  goal, current_path, path_found );
            }
        dbWireGraph::Node::edge_iterator edge_itr;
        for ( edge_itr = node->begin(); edge_itr != node->end(); ++edge_itr )
        {
            if ( (*edge_itr)->type() == dbWireGraph::Edge::Type::VIA || (*edge_itr)->type() == dbWireGraph::Edge::Type::TECH_VIA )
            {
                if ( (*edge_itr)->target()->layer()->getRoutingLevel() <= wire_level )
                    find_car_path( find_segment_start( (*edge_itr)->target() ), wire_level, goal, current_path, path_found );
            }
            else if ( (*edge_itr)->type() == dbWireGraph::Edge::Type::SEGMENT || (*edge_itr)->type() == dbWireGraph::Edge::Type::SHORT )
                find_car_path( (*edge_itr)->target(), wire_level,  goal, current_path, path_found );
        }
    }
    current_path.pop_back();
    return;
}


void
AntennaChecker::build_wire_PAR_table( std::vector<PARinfo>& PARtable, std::vector<std::pair<dbWireGraph::Node *, std::vector<dbWireGraph::Node*>>> wireroots_info )
{
    std::set<dbWireGraph::Node*> level_nodes = {};
    for ( auto root_itr = wireroots_info.begin(); root_itr != wireroots_info.end(); ++root_itr )
    {
        dbWireGraph::Node * wireroot = (*root_itr).first;
        
        if (level_nodes.find(wireroot) != level_nodes.end())
            continue;
        
        std::set<dbWireGraph::Node*> nv = {};
        std::pair<double, double> areas =  calculate_wire_area(wireroot, wireroot->layer()->getRoutingLevel(), nv, level_nodes);

	double wire_area = areas.first; 
        double side_wire_area = areas.second;
        double iterm_areas[2] = {0.0, 0.0};
        std::set<dbITerm*> iv = {};
        nv.clear();

        find_wire_below_iterms( wireroot, iterm_areas, wireroot->layer()->getRoutingLevel(), iv, nv );

        PARinfo new_par = { *root_itr, iv.size(), wire_area, side_wire_area, {iterm_areas[0], iterm_areas[1]}, 0.0, 0.0, 0.0, 0.0 };
        PARtable.push_back( new_par );
    }
    calculate_PAR_table(PARtable);
}


bool 
AntennaChecker::check_iterm(dbWireGraph::Node* node, double iterm_areas[2])
{
    if ( node->object() && strcmp( node->object()->getObjName(), "dbITerm" ) == 0 )
    {
        dbITerm* iterm = dbITerm::getITerm( db_->getChip()->getBlock(), node->object()->getId() );
        dbMTerm* mterm = iterm->getMTerm();
        std::string inst_name = iterm->getInst()->getConstName();
        
        if ( mterm->hasDefaultAntennaModel() )
        {
            dbTechAntennaPinModel * pinmodel = mterm->getDefaultAntennaModel();
            std::vector<std::pair<double, dbTechLayer*>> gate_area;
            pinmodel->getGateArea( gate_area );

            std::vector<std::pair<double, dbTechLayer*>>::iterator gate_area_iter;
            double max_gate_area = 0;
            for ( gate_area_iter = gate_area.begin(); gate_area_iter != gate_area.end();  gate_area_iter++ )
            {    
                max_gate_area = std::max(max_gate_area, (*gate_area_iter).first);
            }
            iterm_areas[0] = iterm_areas[0] + max_gate_area;
        }

        std::vector<std::pair<double, dbTechLayer*>> diff_area;
        mterm->getDiffArea( diff_area );
        
        std::vector<std::pair<double, dbTechLayer*>>::iterator diff_area_iter;
        double max_diff_area = 0;
        for ( diff_area_iter = diff_area.begin(); diff_area_iter != diff_area.end();  diff_area_iter++ )
        {
            max_diff_area = std::max(max_diff_area, (*diff_area_iter).first);
        }
        iterm_areas[1] = iterm_areas[1] + max_diff_area;
        return true;
    } else
        return false;

}

double
AntennaChecker::get_pwl_factor(dbTechLayerAntennaRule::pwl_pair pwl_info, double ref_val, double def)
{
    if ( pwl_info.indices.size() != 0 )
    {
        std::vector<double>::const_iterator indice_itr;
        std::vector<double>::const_iterator ratio_itr;
        if ( pwl_info.indices.size() == 1 )
        {
            indice_itr = pwl_info.indices.begin();
            ratio_itr = pwl_info.ratios.begin();
            return *ratio_itr; 
        }
        else
        {
            double pwl_info_indice;
            double pwl_info_ratio;
            for ( indice_itr = pwl_info.indices.begin(), ratio_itr = pwl_info.ratios.begin();
                  indice_itr != pwl_info.indices.end() && ratio_itr != pwl_info.ratios.end();
                  indice_itr++, ratio_itr++)
            {
                if ( indice_itr == pwl_info.indices.begin() && ratio_itr == pwl_info.ratios.begin() )
                {
                    pwl_info_indice = (*indice_itr);
                    pwl_info_ratio = (*ratio_itr);
                }

                if ( ref_val >= pwl_info_indice && ref_val < (*indice_itr) )
                {
                    return ((*ratio_itr) - pwl_info_ratio) / ((*indice_itr) - pwl_info_indice) * (ref_val - pwl_info_indice) + pwl_info_ratio;
                }
                else
                {
                    pwl_info_indice = (*indice_itr);
                    pwl_info_ratio = (*ratio_itr);
                }
            }
        }
    }
    else
        return def;

    return def;

}

void
AntennaChecker::calculate_PAR_table( std::vector<PARinfo>& PARtable )
{
    for ( std::vector<PARinfo>::iterator it = PARtable.begin(); it!=PARtable.end(); it++)
    {
        dbWireGraph::Node* wireroot = it->WirerootNode.first;
        odb::dbTechLayer* tech_layer = wireroot->layer();
        ANTENNAmodel am = layer_info[tech_layer];

        double metal_factor = am.metal_factor;
        double diff_metal_factor = am.diff_metal_factor;
        double side_metal_factor = am.side_metal_factor;
        double diff_side_metal_factor = am.diff_side_metal_factor;

        double minus_diff_factor = am.minus_diff_factor;
        double plus_diff_factor = am.plus_diff_factor;

        double diff_metal_reduce_factor = am.diff_metal_reduce_factor;

	if (tech_layer->hasDefaultAntennaRule())
        {   
	    dbTechLayerAntennaRule* antenna_rule = tech_layer->getDefaultAntennaRule(); 
  	    diff_metal_reduce_factor = get_pwl_factor(antenna_rule->getAreaDiffReduce(), it->iterm_areas[1], 1.0);    
    	}
        
        if ( it->iterm_areas[0] == 0 )
            continue;

        if ( it->iterm_areas[1] != 0 )
        {
            it->PAR_value = ( diff_metal_factor * it->wire_area ) / it->iterm_areas[0];
            it->PSR_value = ( diff_side_metal_factor * it->side_wire_area ) / it->iterm_areas[0];
            it->diff_PAR_value = ( diff_metal_factor * it->wire_area * diff_metal_reduce_factor - minus_diff_factor * it->iterm_areas[1] ) / ( it->iterm_areas[0] + plus_diff_factor * it->iterm_areas[1] );
            it->diff_PSR_value = ( diff_side_metal_factor * it->side_wire_area * diff_metal_reduce_factor - minus_diff_factor * it->iterm_areas[1] ) / ( it->iterm_areas[0] + plus_diff_factor * it->iterm_areas[1] );
        }
        else
        {
            it->PAR_value = ( metal_factor * it->wire_area ) / it->iterm_areas[0];
            it->PSR_value = ( side_metal_factor * it->side_wire_area ) / it->iterm_areas[0];
            it->diff_PAR_value = ( metal_factor * it->wire_area * diff_metal_reduce_factor ) / it->iterm_areas[0];
            it->diff_PSR_value = ( side_metal_factor * it->side_wire_area * diff_metal_reduce_factor ) / ( it->iterm_areas[0] );
        }
    }
}




void
AntennaChecker::build_wire_CAR_table( std::vector<ARinfo> &CARtable, std::vector<PARinfo> PARtable, std::vector<PARinfo> VIA_PARtable, std::vector<dbWireGraph::Node *> gate_iterms )
{
    std::vector<dbWireGraph::Node *>::iterator gate_itr;
    for( gate_itr = gate_iterms.begin(); gate_itr != gate_iterms.end(); ++gate_itr )
    {
        dbWireGraph::Node * gate = *gate_itr;
        dbTechLayer * layer = gate->layer();
        std::vector<PARinfo>::iterator ar_itr;
        
        for( ar_itr = PARtable.begin(); ar_itr != PARtable.end(); ++ar_itr )
        {
            dbWireGraph::Node * wireroot = ar_itr->WirerootNode.first;
            double par = ar_itr->PAR_value;
            double psr = ar_itr->PSR_value;
            double diff_par = ar_itr->diff_PAR_value;
            double diff_psr = ar_itr->diff_PSR_value;
            double diff_area = ar_itr->iterm_areas[1];
            long unsigned int num_iterms = ar_itr->num_iterms;
            double car = 0.0;
            double csr = 0.0;
            double diff_car = 0.0;
            double diff_csr = 0.0;
            std::vector<dbWireGraph::Node *> current_path;
            std::vector<dbWireGraph::Node *> path_found;
            std::vector<dbWireGraph::Node *> car_wireroots;

            find_car_path( wireroot, wireroot->layer()->getRoutingLevel(), gate, current_path, path_found );
            if( !path_found.empty() )
            {
                std::vector<dbWireGraph::Node *>::iterator path_itr;
                for( path_itr = path_found.begin(); path_itr != path_found.end(); ++path_itr )
                {
                    dbWireGraph::Node * node = *path_itr;
                    if ( if_segment_root( node, node->layer()->getRoutingLevel() ) )
                        car_wireroots.push_back( node );
                }
                
                std::vector<dbWireGraph::Node *>::iterator car_root_itr;
                for( car_root_itr = car_wireroots.begin(); car_root_itr != car_wireroots.end(); ++car_root_itr )
                {
                    dbWireGraph::Node * car_root = *car_root_itr;
                    std::vector<PARinfo>::iterator par_itr;
                    for( par_itr = PARtable.begin(); par_itr != PARtable.end(); ++par_itr )
                    {
                        if( par_itr->WirerootNode.first == car_root )
                        {
                            car = car + par_itr->PAR_value;
                            csr = csr + par_itr->PSR_value;
                            diff_car = diff_car + par_itr->diff_PAR_value;
                            diff_csr = diff_csr + par_itr->diff_PSR_value;
                            break;
                        }
                    }
                    dbTechLayer * wire_layer = wireroot->layer();
                    if ( wire_layer->hasDefaultAntennaRule() )
                    {
                        dbTechLayerAntennaRule* antenna_rule = wire_layer->getDefaultAntennaRule();
                        if ( antenna_rule->hasAntennaCumRoutingPlusCut() )
                        {
                            if ( car_root->layer()->getRoutingLevel() < wireroot->layer()->getRoutingLevel() )
                            {
                                std::vector<PARinfo>::iterator via_par_itr;
                                for( via_par_itr = VIA_PARtable.begin(); via_par_itr != VIA_PARtable.end(); ++via_par_itr )
                                {
                                    if( via_par_itr->WirerootNode.first == car_root )
                                    {
                                        car = car + via_par_itr->PAR_value;
                                        diff_car = diff_car + via_par_itr->diff_PAR_value;
                                        break;
                                    }
                                }
                            }
                        }
                    }     
                }

                ARinfo new_car = { wireroot, gate, false, par, psr, diff_par, diff_psr, car, csr, diff_car, diff_csr, diff_area };
                CARtable.push_back( new_car );
            }
        }
    }
}


void
AntennaChecker::build_VIA_PAR_table( std::vector<PARinfo> &VIA_PARtable, std::vector<std::pair<dbWireGraph::Node *, std::vector<dbWireGraph::Node*>>> wireroots_info )
{

    for ( auto root_itr = wireroots_info.begin(); root_itr != wireroots_info.end(); ++root_itr )
    {
        dbWireGraph::Node * wireroot = (*root_itr).first;
        double via_area = calculate_via_area( wireroot, wireroot->layer()->getRoutingLevel() );
        double iterm_areas[2] = {0.0, 0.0};
        std::set<dbITerm*> iv;
        std::set<dbWireGraph::Node*> nv;
        find_wire_below_iterms( wireroot, iterm_areas, wireroot->layer()->getRoutingLevel(), iv, nv );
        double par = 0.0;
        double diff_par = 0.0;

        double cut_factor = 1.0;
        double diff_cut_factor = 1.0;

        double minus_diff_factor = 0.0;
        double plus_diff_factor = 0.0;
        double diff_metal_reduce_factor = 1.0;

        if ( via_area != 0 && iterm_areas[0] != 0 )
        {

	    dbTechLayer* layer = get_via_layer( find_via( wireroot, wireroot->layer()->getRoutingLevel() ) );

            ANTENNAmodel am = layer_info[layer];
            minus_diff_factor = am.minus_diff_factor;
            plus_diff_factor = am.plus_diff_factor;
            diff_metal_reduce_factor = am.diff_metal_reduce_factor;
            if ( layer->hasDefaultAntennaRule() )
            {
                dbTechLayerAntennaRule* antenna_rule = layer->getDefaultAntennaRule();
                diff_metal_reduce_factor = get_pwl_factor(antenna_rule->getAreaDiffReduce() , iterm_areas[1], 1.0);
            }

            cut_factor = am.cut_factor;
            diff_cut_factor = am.diff_cut_factor;

            minus_diff_factor = am.minus_diff_factor;
            plus_diff_factor = am.plus_diff_factor;

            double plus_diff_factor = 5.0;
            if ( iterm_areas[1] != 0 )
            {
                par = ( diff_cut_factor * via_area ) / iterm_areas[0];
                diff_par = ( diff_cut_factor * via_area * diff_metal_reduce_factor - minus_diff_factor * iterm_areas[1] ) / ( iterm_areas[0] + plus_diff_factor * iterm_areas[1] );
            }
            else
            {
                par = ( cut_factor * via_area ) / iterm_areas[0];
                diff_par = ( cut_factor * via_area * diff_metal_reduce_factor - minus_diff_factor * iterm_areas[1] ) / ( iterm_areas[0] + plus_diff_factor * iterm_areas[1] );
            }
            PARinfo new_par = { *root_itr, iv.size(), 0.0, 0.0, {0.0, 0.0}, par, 0.0, diff_par, 0.0 };
            VIA_PARtable.push_back( new_par );
        }
    }
}

void
AntennaChecker::build_VIA_CAR_table( std::vector<ARinfo> &VIA_CARtable, std::vector<PARinfo> PARtable, std::vector<PARinfo> VIA_PARtable, std::vector<dbWireGraph::Node *> gate_iterms )
{
    std::vector<dbWireGraph::Node *>::iterator gate_itr;
    for( gate_itr = gate_iterms.begin(); gate_itr != gate_iterms.end(); ++gate_itr )
    {
        dbWireGraph::Node * gate = *gate_itr;
        dbTechLayer * layer = gate->layer();
        int x, y;
        gate->xy( x, y );

        std::vector<PARinfo>::iterator ar_itr;
        for( ar_itr = VIA_PARtable.begin(); ar_itr != VIA_PARtable.end(); ++ar_itr )
        {
            dbWireGraph::Node * wireroot = (ar_itr->WirerootNode).first;
            double par = ar_itr->PAR_value;
            double diff_par = ar_itr->diff_PAR_value;
            double diff_area = ar_itr->iterm_areas[1];
            double car = 0.0;
            double diff_car = 0.0;
            std::vector<dbWireGraph::Node *> current_path;
            std::vector<dbWireGraph::Node *> path_found;
            std::vector<dbWireGraph::Node *> car_wireroots;

            find_car_path( wireroot, wireroot->layer()->getRoutingLevel(), gate, current_path, path_found );
            if( !path_found.empty() )
            {
                std::vector<dbWireGraph::Node *>::iterator path_itr;
                for( path_itr = path_found.begin(); path_itr != path_found.end(); ++path_itr )
                {
                    dbWireGraph::Node * node = *path_itr;
                    int x, y;
                    node->xy( x, y );
                    if ( if_segment_root( node, node->layer()->getRoutingLevel() ) )
                        car_wireroots.push_back( node );
                }
                std::vector<dbWireGraph::Node *>::iterator car_root_itr;
                for( car_root_itr = car_wireroots.begin(); car_root_itr != car_wireroots.end(); ++car_root_itr )
                {
                    dbWireGraph::Node * car_root = *car_root_itr;
                    int x, y;
                    car_root->xy( x, y );
                    std::vector<PARinfo>::iterator par_itr;
                    std::vector<PARinfo>::iterator via_par_itr;
                    for( via_par_itr = VIA_PARtable.begin(); via_par_itr != VIA_PARtable.end(); ++via_par_itr )
                    {
                        if( (via_par_itr->WirerootNode).first == car_root )
                        {
                            car = car + via_par_itr->PAR_value;
                            diff_car = diff_car + via_par_itr->diff_PAR_value;
                            break;
                        }
                    }
                    dbTechLayer * via_layer = get_via_layer( find_via( wireroot, wireroot->layer()->getRoutingLevel() ) );
                    if ( via_layer->hasDefaultAntennaRule() )
                    {
                        dbTechLayerAntennaRule* antenna_rule = via_layer->getDefaultAntennaRule();
                        if ( antenna_rule->hasAntennaCumRoutingPlusCut() )
                        {
                            for( par_itr = PARtable.begin(); par_itr != PARtable.end(); ++par_itr )
                            {
                                if( par_itr->WirerootNode.first == car_root )
                                {
                                    car = car + par_itr->PAR_value;
                                    diff_car = diff_car + par_itr->diff_PAR_value;
                                    break;
                                }
                            }
                        }
                    }
                }

                ARinfo new_car = { wireroot, gate, false ,par, 0.0, diff_par, 0.0, car, 0.0, diff_car, 0.0, diff_area };
                VIA_CARtable.push_back( new_car );
            }
        }
    }
}


bool
AntennaChecker::check_wire_PAR( ARinfo AntennaRatio )
{
    dbTechLayer * layer = AntennaRatio.WirerootNode->layer();
    double par = AntennaRatio.PAR_value;
    double psr = AntennaRatio.PSR_value;
    double diff_par = AntennaRatio.diff_PAR_value;
    double diff_psr = AntennaRatio.diff_PSR_value;
    double diff_area = AntennaRatio.diff_area;

    bool if_violated = 0;
    
    if ( layer->hasDefaultAntennaRule() )
    {
        dbTechLayerAntennaRule* antenna_rule = layer->getDefaultAntennaRule();
        double PAR_ratio = antenna_rule->getPAR();
        if ( PAR_ratio != 0 )
        {
            fprintf( _out, "  PAR: %7.2f", par );
            if ( par > PAR_ratio )
            {
                fprintf( _out, "*" );
                if_violated = 1;
            }

            fprintf( _out, "  Ratio: %7.2f       (Area)\n", PAR_ratio );
        }
        else
        {
            dbTechLayerAntennaRule::pwl_pair diffPAR = antenna_rule->getDiffPAR();

            double diffPAR_PWL_ratio = get_pwl_factor(diffPAR, diff_area, 0);
            fprintf( _out, "  PAR: %7.2f", diff_par );
            if (diffPAR_PWL_ratio == 0)
                fprintf( _out, "  Ratio:    0.00       (Area)\n" );
            else
            {
                if ( diff_par > diffPAR_PWL_ratio )
                {
                    fprintf( _out, "*" );
                    if_violated = 1;
                }
                fprintf( _out, "  Ratio: %7.2f       (Area)\n", diffPAR_PWL_ratio );
            }

	}

        double PSR_ratio = antenna_rule->getPSR();
        if ( PSR_ratio != 0 )
        {
            fprintf( _out, "  PAR: %7.2f", psr );
            if ( psr > PSR_ratio )
            {
                fprintf( _out, "*" );
                if_violated = 1;
            }
            fprintf( _out, "  Ratio: %7.2f       (S.Area)\n", PSR_ratio );
        }
        else
        {
            dbTechLayerAntennaRule::pwl_pair diffPSR = antenna_rule->getDiffPSR();


            double diffPSR_PWL_ratio = get_pwl_factor(diffPSR, diff_area, 0.0);
            fprintf( _out, "  PAR: %7.2f", diff_psr );
            if (diffPSR_PWL_ratio == 0)
                fprintf( _out, "  Ratio:    0.00       (S.Area)\n" );
            else
            {
                if ( diff_psr > diffPSR_PWL_ratio )
                {
                    fprintf( _out, "*" );
                    if_violated = 1;
                }
                fprintf( _out, "  Ratio: %7.2f       (S.Area)\n", diffPSR_PWL_ratio );
            }
	}
    }

    return if_violated;
}


bool
AntennaChecker::check_wire_CAR( ARinfo AntennaRatio )
{
    dbTechLayer * layer = AntennaRatio.WirerootNode->layer();
    double car = AntennaRatio.CAR_value;
    double csr = AntennaRatio.CSR_value;
    double diff_car = AntennaRatio.diff_CAR_value;
    double diff_csr = AntennaRatio.diff_CSR_value;
    double diff_area = AntennaRatio.diff_area;

    bool if_violated = 0;
    if ( layer->hasDefaultAntennaRule() )
    {
        dbTechLayerAntennaRule* antenna_rule = layer->getDefaultAntennaRule();
        double CAR_ratio = antenna_rule->getCAR();
        if ( CAR_ratio != 0 )
        {
            fprintf( _out, "  CAR: %7.2f", car );
            if ( car > CAR_ratio )
            {
                fprintf( _out, "*" );
                if_violated = 1;
            }
            fprintf( _out, "  Ratio: %7.2f       (C.Area)\n", CAR_ratio );            
        }
        else
        {
            dbTechLayerAntennaRule::pwl_pair diffCAR = antenna_rule->getDiffCAR();
             
	    double diffCAR_PWL_ratio = get_pwl_factor(diffCAR, diff_area, 0);
            fprintf( _out, "  CAR: %7.2f", car );
            if (diffCAR_PWL_ratio == 0)
                fprintf( _out, "  Ratio:    0.00       (C.Area)\n" );
            else
            {
                if ( car > diffCAR_PWL_ratio )
                {
                    fprintf( _out, "*" );
                    if_violated = 1;
                }
                fprintf( _out, "  Ratio: %7.2f       (C.Area)\n", diffCAR_PWL_ratio );
            }
	}


	double CSR_ratio = antenna_rule->getCSR();	 
        if ( CSR_ratio != 0 )
        {
            fprintf( _out, "  CAR: %7.2f", csr );
            if ( csr > CSR_ratio )
            {
                fprintf( _out, "*" );
                if_violated = 1;
            }
            fprintf( _out, "  Ratio: %7.2f       (C.S.Area)\n", CSR_ratio );
        }
        else
        {
            dbTechLayerAntennaRule::pwl_pair diffCSR = antenna_rule->getDiffCSR();


            double diffCSR_PWL_ratio = get_pwl_factor(diffCSR, diff_area, 0.0);
            fprintf( _out, "  CAR: %7.2f", diff_csr );
            if (diffCSR_PWL_ratio == 0)
                fprintf( _out, "  Ratio:    0.00       (C.S.Area)\n" );
            else
            {
                if ( diff_csr > diffCSR_PWL_ratio )
                {
                    fprintf( _out, "*" );
                    if_violated = 1;
                }
                fprintf( _out, "  Ratio: %7.2f       (C.S.Area)\n", diffCSR_PWL_ratio );
            }

        }
    }
    return if_violated;
}

bool
AntennaChecker::check_VIA_PAR( ARinfo AntennaRatio )
{
    dbTechLayer * layer = get_via_layer( find_via( AntennaRatio.WirerootNode, AntennaRatio.WirerootNode->layer()->getRoutingLevel() ) );
    double par = AntennaRatio.PAR_value;
    double diff_par = AntennaRatio.diff_PAR_value;
    double diff_area = AntennaRatio.diff_area;

    bool if_violated = 0;
    if ( layer->hasDefaultAntennaRule() )
    {
        dbTechLayerAntennaRule* antenna_rule = layer->getDefaultAntennaRule();
        double PAR_ratio = antenna_rule->getPAR();
        if ( PAR_ratio != 0 )
        {
            fprintf( _out, "  PAR: %7.2f", par );
            if ( par > PAR_ratio )
            {
                fprintf( _out, "*" );
                if_violated = 1;
            }
            fprintf( _out, "  Ratio: %7.2f       (Area)\n", PAR_ratio );            
        }
        else
        {
            dbTechLayerAntennaRule::pwl_pair diffPAR = antenna_rule->getDiffPAR();
	
            double diffPAR_PWL_ratio = get_pwl_factor(diffPAR, diff_area, 0);
            fprintf( _out, "  PAR: %7.2f", par );
            if (diffPAR_PWL_ratio == 0)
                fprintf( _out, "  Ratio:    0.00       (Area)\n" );
            else
            {
                if ( par > diffPAR_PWL_ratio )
                {
                    fprintf( _out, "*" );
                    if_violated = 1;
                }
                fprintf( _out, "  Ratio: %7.2f       (Area)\n", diffPAR_PWL_ratio );
            }	                
	}
    }
    return if_violated;
}


bool
AntennaChecker::check_VIA_CAR( ARinfo AntennaRatio )
{
    dbTechLayer * layer = get_via_layer( find_via( AntennaRatio.WirerootNode, AntennaRatio.WirerootNode->layer()->getRoutingLevel() ) );
    double car = AntennaRatio.CAR_value;
    double diff_car = AntennaRatio.diff_CAR_value;
    double diff_area = AntennaRatio.diff_area;

    bool if_violated = 0;
    if ( layer->hasDefaultAntennaRule() )
    {
        dbTechLayerAntennaRule* antenna_rule = layer->getDefaultAntennaRule();
        double CAR_ratio = antenna_rule->getCAR();
        if ( CAR_ratio != 0 )
        {
            fprintf( _out, "  CAR: %7.2f", car );
            if ( car > CAR_ratio )
            {
                fprintf( _out, "*" );
                if_violated = 1;
            }
            fprintf( _out, "  Ratio: %7.2f       (C.Area)\n", CAR_ratio );            
        }
        else
        {
            dbTechLayerAntennaRule::pwl_pair diffCAR = antenna_rule->getDiffCAR();

            double diffCAR_PWL_ratio = get_pwl_factor(diffCAR, diff_area, 0);
            fprintf( _out, "  CAR: %7.2f", car );
            if (diffCAR_PWL_ratio == 0)
                fprintf( _out, "  Ratio:    0.00       (C.Area)\n" );
            else
            {
                if ( car > diffCAR_PWL_ratio )
                {
                    fprintf( _out, "*" );
                    if_violated = 1;
                }
                fprintf( _out, "  Ratio: %7.2f       (C.Area)\n", diffCAR_PWL_ratio );
            }
	}
    }
    return if_violated;
}


std::vector<int>
AntennaChecker::GetAntennaRatio()
{
    std::string bname = db_->getChip()->getBlock()->getName();

    std::string output_file_name = bname + ".ac.rpt";
    _out = fopen( output_file_name.c_str(), "w" );

    if ( _out == nullptr )
    {
        odb::notice(0,"Cannot open report file (%s) for writing\n", output_file_name.c_str() );
        return {0, 0, 0};
    }

    dbSet<dbNet> nets = db_->getChip()->getBlock()->getNets();
    if ( nets.size() == 0 )
        return {0, 0, 0};
        
    dbSet<dbNet>::iterator net_itr;
    int num_total_net = 0;
    int num_violated_net = 0;
    int num_violated_pins = 0;
    for( net_itr = nets.begin(); net_itr != nets.end(); ++net_itr )
    {
        dbNet * net = *net_itr;
        if ( net->isSpecial() )
            continue;
        num_total_net++;
        std::string nname = net->getConstName();
        fprintf( _out, "\nNet - %s\n", nname.c_str() );
        dbWire * wire = net->getWire();
        dbWireGraph graph;
        if ( wire )
        {
            graph.decode( wire );
            dbWireGraph::node_iterator node_itr;
            dbWireGraph::edge_iterator edge_itr;

            // print_graph_info(graph);

            std::vector<std::pair<dbWireGraph::Node *, std::vector<dbWireGraph::Node*>>> wireroots_info;
            std::vector<dbWireGraph::Node *> gate_iterms;

            for ( node_itr = graph.begin_nodes(); node_itr != graph.end_nodes(); ++node_itr )
            {
                
                dbWireGraph::Node * node = *node_itr;
                
                auto wireroot_info = find_segment_root( { node, { node } }, node->layer()->getRoutingLevel() );
                dbWireGraph::Node* wireroot = wireroot_info.first;
                 
                if ( wireroot)
                {

                    bool find_root = 0;
                    std::vector<std::pair<dbWireGraph::Node *, int>>::iterator root_itr;
                    for(auto root_itr = wireroots_info.begin(); root_itr != wireroots_info.end(); ++root_itr )
                    {
                        if ( find_root )
                            break;
                        else
                        {
                            if ( (*root_itr).first == wireroot )
                                find_root = 1;
                        }
                    }
                    if ( !find_root )
                        wireroots_info.push_back( wireroot_info );
                }
                if( node->object() && strcmp( node->object()->getObjName(), "dbITerm" ) == 0 )
                {
                    dbITerm * iterm = dbITerm::getITerm( db_->getChip()->getBlock(), node->object()->getId() );
                    dbMTerm * mterm = iterm->getMTerm();
                    if ( strcmp( mterm->getIoType().getString(), "INPUT" ) == 0 )
                        if ( mterm->hasDefaultAntennaModel() )
                            gate_iterms.push_back( node );
                }
            }

            std::vector<PARinfo> PARtable;
            build_wire_PAR_table( PARtable, wireroots_info );

            std::vector<PARinfo> VIA_PARtable;
            build_VIA_PAR_table( VIA_PARtable, wireroots_info );
            
            std::vector<ARinfo> CARtable;
            build_wire_CAR_table( CARtable, PARtable, VIA_PARtable, gate_iterms );

            std::vector<ARinfo> VIA_CARtable;
            build_VIA_CAR_table( VIA_CARtable, PARtable, VIA_PARtable, gate_iterms );

            bool if_violated_wire = 0;
            bool if_violated_VIA = 0;

            std::vector<dbWireGraph::Node *>::iterator gate_itr;
            for( gate_itr = gate_iterms.begin(); gate_itr != gate_iterms.end(); ++gate_itr )
            {
                dbWireGraph::Node * gate = *gate_itr;
                dbTechLayer * layer = gate->layer();
                
    	    dbITerm * iterm = dbITerm::getITerm( db_->getChip()->getBlock(), gate->object()->getId() );
                dbMTerm * mterm = iterm->getMTerm();
                fprintf(_out, "  %s  (%s)  %s\n", iterm->getInst()->getConstName(), mterm->getMaster()->getConstName(), mterm->getConstName());

                for( auto ar: CARtable )
                {
                    if( ar.GateNode == gate )
                    {
                        fprintf( _out, "[1]  %s:\n", ar.WirerootNode->layer()->getConstName());
                        bool wire_PAR_violation = check_wire_PAR( ar );
                        bool wire_CAR_violation = check_wire_CAR( ar );
                        if ( wire_PAR_violation || wire_CAR_violation )
                            if_violated_wire = 1;

                        fprintf( _out, "\n");
                    }
                }

                for( auto via_ar: VIA_CARtable )
                {
                    if( via_ar.GateNode == gate )
                    {
                        dbWireGraph::Edge * via = find_via( via_ar.WirerootNode, via_ar.WirerootNode->layer()->getRoutingLevel() );
                        fprintf( _out, "[1]  %s:\n", get_via_name( via ).c_str());
                        bool VIA_PAR_violation = check_VIA_PAR( via_ar );
                        bool VIA_CAR_violation = check_VIA_CAR( via_ar );
                        if ( VIA_PAR_violation || VIA_CAR_violation )
                            if_violated_VIA = 1;
                        fprintf( _out, "\n");
                    }
                }
            }

            if( if_violated_wire || if_violated_VIA )
            {
                num_violated_net++;
                num_violated_pins += gate_iterms.size();
            }
        }
    }    
    fprintf(_out, "Number of pins violated: %d\nNumber of nets violated: %d\nTotal number of unspecial nets: %d\n", num_violated_pins, num_violated_net, num_total_net );
    return {num_violated_pins, num_violated_net, num_total_net };
}


void
AntennaChecker::check_antennas()
{
    //clock_t start, end;
    //start = clock();
    std::string bname = db_->getChip()->getBlock()->getName();
    printf( "Checking antenna...\n" );
    printf( "Design name: %s\n", bname.c_str());
    std::vector<int> nets_info = GetAntennaRatio();
    if ( nets_info[2] != 0 )
        printf( "Number of pins violated: %d\nNumber of nets violated: %d\nTotal number of nets: %d\n", nets_info[0], nets_info[1], nets_info[2] );
    else
        printf( "Design error!\n" );
    printf("Gotta verbose %s\n", verbose_ ? "true" : "false");
    //end = clock();
    //std::cout << "Runtime: " << ( double )( end - start ) / CLOCKS_PER_SEC << std::endl;   
}


void
AntennaChecker::find_wireroot_iterms( dbWireGraph::Node * node, int wire_level, std::vector<dbITerm *>& gates )
{
    double iterm_areas[2] = {0.0, 0.0};
    std::set<dbITerm *> iv;
    std::set<dbWireGraph::Node *> nv;

    find_wire_below_iterms(node, iterm_areas, wire_level, iv, nv);
    gates.assign(iv.begin(), iv.end());

}


std::vector<std::pair<double, std::vector<dbITerm *>>>
AntennaChecker::PAR_max_wire_length( dbNet * net, int route_level )
{
    
    std::vector<std::pair<double, std::vector<dbITerm *>>> par_wires;
    if ( net->isSpecial() )
        return par_wires;
    dbWire * wire = net->getWire();
    if ( wire != nullptr )
    {
        dbWireGraph graph;
        graph.decode( wire );

        dbWireGraph::node_iterator node_itr;
        dbWireGraph::edge_iterator edge_itr;

        std::vector<dbWireGraph::Node *> wireroots;
	auto wireroots_info = get_wireroots(graph);
	
	std::set<dbWireGraph::Node*> level_nodes;
        for ( auto root_itr: wireroots_info )
        {
            dbWireGraph::Node* wireroot = root_itr.first;
            odb::dbTechLayer* tech_layer = wireroot->layer();
	    if( level_nodes.find(wireroot) == level_nodes.end() && tech_layer->getRoutingLevel() == route_level )
            {
                double max_length = 0;
                std::set<dbWireGraph::Node*> nv;
		std::pair<double, double> areas = calculate_wire_area( wireroot, tech_layer->getRoutingLevel(), nv, level_nodes );
		double wire_area = areas.first;
                double iterm_areas[2] = {0.0, 0.0};
                std::set<dbITerm*> iv;
            	nv.clear();
            	find_wire_below_iterms( wireroot, iterm_areas, tech_layer->getRoutingLevel(), iv, nv );
                double wire_width = defdist( tech_layer->getWidth() );

		ANTENNAmodel am = layer_info[tech_layer];
                double metal_factor = am.metal_factor;
                double diff_metal_factor = am.diff_metal_factor;
                double side_metal_factor = am.side_metal_factor;
                double diff_side_metal_factor = am.diff_side_metal_factor;

                double minus_diff_factor = am.minus_diff_factor;
                double plus_diff_factor = am.plus_diff_factor;
                double diff_metal_reduce_factor = am.diff_metal_reduce_factor;
		
                if ( iterm_areas[0] != 0 && tech_layer->hasDefaultAntennaRule() )
                {
                    dbTechLayerAntennaRule* antenna_rule = tech_layer->getDefaultAntennaRule();
		    dbTechLayerAntennaRule::pwl_pair diff_metal_reduce_factor_pwl = antenna_rule->getAreaDiffReduce();
                    diff_metal_reduce_factor = get_pwl_factor(diff_metal_reduce_factor_pwl, iterm_areas[1], 1.0);

                    double PAR_ratio = antenna_rule->getPAR();
                    if ( PAR_ratio != 0 )
                    {
                        if ( iterm_areas[1] != 0 )
                            max_length = ( PAR_ratio * iterm_areas[0] - diff_metal_factor * wire_area ) / wire_width;
                        else
                            max_length = ( PAR_ratio * iterm_areas[0] - metal_factor * wire_area ) / wire_width;
                    }
                    else
                    {
                        dbTechLayerAntennaRule::pwl_pair diffPAR = antenna_rule->getDiffPAR();
                        double diffPAR_ratio = get_pwl_factor(diffPAR, iterm_areas[1], 0.0);
			if ( iterm_areas[1] != 0 )
                       	    max_length = ( diffPAR_ratio * ( iterm_areas[0] + plus_diff_factor * iterm_areas[1] ) - ( diff_metal_factor * wire_area * diff_metal_reduce_factor - minus_diff_factor * iterm_areas[1] ) ) / wire_width;
                        else
                            max_length = ( diffPAR_ratio * ( iterm_areas[0] + plus_diff_factor * iterm_areas[1] ) - ( metal_factor * wire_area * diff_metal_reduce_factor - minus_diff_factor * iterm_areas[1] ) ) / wire_width;
                    }
                    if ( max_length != 0 )
                    {
                        std::vector<dbITerm *> gates;
                        find_wireroot_iterms( wireroot, wireroot->layer()->getRoutingLevel(), gates );
                        std::pair<double, std::vector<dbITerm *>> par_wire = std::make_pair( max_length, gates );
                        par_wires.push_back( par_wire );
                    }
                }
            }
        }
    }
    return par_wires;
}


void 
AntennaChecker::check_par_max_length()
{
    std::string bname = db_->getChip()->getBlock()->getName();
    dbSet<dbNet> nets = db_->getChip()->getBlock()->getNets();
    if ( nets.size() != 0 )
    {
        dbSet<dbNet>::iterator net_itr;
	for( net_itr = nets.begin(); net_itr != nets.end(); ++net_itr )
        {
            dbNet * net = *net_itr;
	    if ( net->isSpecial() )
                continue;
	    if ( std::string(net->getConstName()).compare(net_name_) == 0 )
            {
                std::vector<std::pair<double, std::vector<dbITerm *>>> par_max_length_wires = PAR_max_wire_length( net, route_level_ );
		for ( auto par_wire: par_max_length_wires) 
                {
                    printf( "Net name: %s, Routing Level: %d, Max Length for PAR: %f\n", net_name_.c_str(), route_level_, par_wire.first );
                }
            }
        }
    }
}

std::vector<std::pair<dbWireGraph::Node *, std::vector<dbWireGraph::Node*>>>
AntennaChecker::get_wireroots(dbWireGraph graph)
{
    std::vector<std::pair<dbWireGraph::Node *, std::vector<dbWireGraph::Node*>>> wireroots_info;
    dbWireGraph::node_iterator node_itr;

    for ( node_itr = graph.begin_nodes(); node_itr != graph.end_nodes(); ++node_itr )
    {
        dbWireGraph::Node * node = *node_itr;
        auto wireroot_info = find_segment_root( { node, { node } }, node->layer()->getRoutingLevel() );
        dbWireGraph::Node * wireroot = wireroot_info.first;
        if ( wireroot)
        {
            bool find_root = 0;
            std::vector<std::pair<dbWireGraph::Node *, int>>::iterator root_itr;
            for( auto root_itr = wireroots_info.begin(); root_itr != wireroots_info.end(); ++root_itr )
            {
                if ( find_root )
                    break;
                else
                {
                    if ( (*root_itr).first == wireroot )
                        find_root = 1;
                }
            }
            if ( !find_root )
                wireroots_info.push_back( wireroot_info );
        }
    }

    return wireroots_info;
}


std::vector<std::pair<int, std::vector<dbITerm *>>>
AntennaChecker::get_net_antenna_violations( dbNet * net )
{
    std::vector<std::pair<int, std::vector<dbITerm *>>> antenna_violations;
    if ( net->isSpecial() )
        return antenna_violations;
    dbWire * wire = net->getWire();
    dbWireGraph graph;
    if ( wire )
    {
        graph.decode( wire );

	auto wireroots_info = get_wireroots(graph);
	
        std::vector<PARinfo> PARtable;
        build_wire_PAR_table( PARtable, wireroots_info );
	
        std::vector<PARinfo>::iterator par_itr;
        
        for( par_itr = PARtable.begin(); par_itr != PARtable.end(); ++par_itr )
        {
	    dbTechLayer * layer = (*par_itr).WirerootNode.first->layer();
            double par = (*par_itr).PAR_value;
            double psr = (*par_itr).PSR_value;
  	    double diff_par = (*par_itr).diff_PAR_value;
            double diff_psr = (*par_itr).diff_PSR_value;
 	    double diff_area = (*par_itr).iterm_areas[1];
	    
            bool wire_PAR_violation = 0;
            if ( layer->hasDefaultAntennaRule() )
            {
                dbTechLayerAntennaRule* antenna_rule = layer->getDefaultAntennaRule();
		double PAR_ratio = antenna_rule->getPAR();
                if ( PAR_ratio != 0 )
                {
                    if ( par > PAR_ratio )
                        wire_PAR_violation = 1;
                }
                else
                {
                    dbTechLayerAntennaRule::pwl_pair diffPAR = antenna_rule->getDiffPAR();
                    double diffPAR_ratio = get_pwl_factor(diffPAR, diff_area, 0.0);
		    if (diffPAR_ratio != 0 && diff_par > diffPAR_ratio)
			wire_PAR_violation = 1;
                }

        	double PSR_ratio = antenna_rule->getPSR();
        	if ( PSR_ratio != 0 )
        	{
        	    if ( psr > PSR_ratio )
        	        wire_PAR_violation = 1;
        	}
        	else
        	{
        	    dbTechLayerAntennaRule::pwl_pair diffPSR = antenna_rule->getDiffPSR();
        	    double diffPSR_ratio = get_pwl_factor(diffPSR, diff_area, 0.0);
        	    if ( diffPSR_ratio != 0 && diff_psr > diffPSR_ratio )
        	            wire_PAR_violation = 1;
        	}

	    }

            if ( wire_PAR_violation )
            {
                std::vector<dbITerm *> gates;
                find_wireroot_iterms( (*par_itr).WirerootNode.first, layer->getRoutingLevel(), gates );
                std::pair<int, std::vector<dbITerm *>> antenna_violation = std::make_pair( layer->getRoutingLevel(), gates );
                antenna_violations.push_back( antenna_violation );
            }
        }
    }
    return antenna_violations;
}




std::vector<std::pair<double, std::vector<dbITerm *>>>
AntennaChecker::get_violated_wire_length( dbNet * net, int routing_level )
{

    std::vector<std::pair<double, std::vector<dbITerm *>>> violated_wires;
    if ( net->isSpecial() || net->getWire() == nullptr)
        return violated_wires;
    dbWire * wire = net->getWire();
        
    dbWireGraph graph;
    graph.decode( wire );

    auto wireroots_info = get_wireroots(graph);
    
    std::set<dbWireGraph::Node*> level_nodes;
    for ( auto root_itr = wireroots_info.begin(); root_itr != wireroots_info.end(); ++root_itr )
    {
        dbWireGraph::Node* wireroot = root_itr->first;
        odb::dbTechLayer* tech_layer = wireroot->layer();
        if (level_nodes.find(wireroot) == level_nodes.end()
            && tech_layer->getRoutingLevel() == routing_level )
        {
            std::set<dbWireGraph::Node*> nv;
            auto areas = calculate_wire_area( wireroot, tech_layer->getRoutingLevel(), nv, level_nodes );
            double wire_area = areas.first; 
            double iterm_areas[2] = {0.0, 0.0};
            
            std::set<dbITerm*> iv;
            nv.clear();
            find_wire_below_iterms( wireroot, iterm_areas, tech_layer->getRoutingLevel(), iv, nv );
            if (iterm_areas[0] == 0)
                continue;

            double wire_width = defdist( tech_layer->getWidth() );

            ANTENNAmodel am = layer_info[tech_layer];
            double metal_factor = am.metal_factor;
            double diff_metal_factor = am.diff_metal_factor;
            double side_metal_factor = am.side_metal_factor;
            double diff_side_metal_factor = am.diff_side_metal_factor;

            double minus_diff_factor = am.minus_diff_factor;
            double plus_diff_factor = am.plus_diff_factor;
            double diff_metal_reduce_factor = am.diff_metal_reduce_factor;
            
            if (wireroot->layer()->hasDefaultAntennaRule())
            {
                dbTechLayerAntennaRule* antenna_rule = tech_layer->getDefaultAntennaRule();
                diff_metal_reduce_factor = get_pwl_factor(antenna_rule->getAreaDiffReduce(), iterm_areas[1], 1.0);

                double par = 0;
                double diff_par = 0;

                if ( iterm_areas[1] != 0 )
                {
                    par = ( diff_metal_factor * wire_area ) / iterm_areas[0];
                    diff_par = ( diff_metal_factor * wire_area * diff_metal_reduce_factor - minus_diff_factor * iterm_areas[1] ) / ( iterm_areas[0] + plus_diff_factor * iterm_areas[1] );
                }
                else
                {
                    par = ( metal_factor * wire_area ) / iterm_areas[0];
                    diff_par = ( metal_factor * wire_area * diff_metal_reduce_factor ) / iterm_areas[0];
                }    

                double cut_length = 0;
                double PAR_ratio = antenna_rule->getPAR();
                if ( PAR_ratio != 0 )
                {
                    if ( par > PAR_ratio )
                    {
                        if ( iterm_areas[1] != 0 )
                            cut_length = ( ( par - PAR_ratio ) * iterm_areas[0] - diff_metal_factor * wire_area ) / wire_width;
                        else
                            cut_length = ( ( par - PAR_ratio ) * iterm_areas[0] - metal_factor * wire_area ) / wire_width;
                    }    

                }
                else
                {
                    dbTechLayerAntennaRule::pwl_pair diffPAR = antenna_rule->getDiffPAR();
                    double diffPAR_ratio =  get_pwl_factor(diffPAR, iterm_areas[1], 0.0);
                    if (iterm_areas[1] != 0)
                        cut_length = ( (diff_par - diffPAR_ratio ) * ( iterm_areas[0] + plus_diff_factor * iterm_areas[1] ) - ( diff_metal_factor * wire_area * diff_metal_reduce_factor - minus_diff_factor * iterm_areas[1] ) ) / wire_width;
                    else
                        cut_length = ( (diff_par - diffPAR_ratio ) * ( iterm_areas[0] + plus_diff_factor * iterm_areas[1] ) - ( metal_factor * wire_area * diff_metal_reduce_factor - minus_diff_factor * iterm_areas[1] ) ) / wire_width;
                    
                }
            
                if ( cut_length != 0 )
                {
                    std::vector<dbITerm *> gates;
                    find_wireroot_iterms( wireroot, routing_level, gates );
                    std::pair<double, std::vector<dbITerm *>> violated_wire = std::make_pair( cut_length, gates );
                    violated_wires.push_back( violated_wire );
                }
            }
        }
    }
    return violated_wires;
}

}
