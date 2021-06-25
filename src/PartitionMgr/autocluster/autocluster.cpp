#include "autocluster.h"
#include "opendb/db.h"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "MLPart.h"
#include "utl/Logger.h"


#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>
#include <queue>
#include <cmath>
#include <tuple>
#include <sys/stat.h>

using utl::PAR;


namespace par {
    using std::string;
    using std::cout;
    using std::endl;
    using std::vector;
    using std::unordered_map;
    using std::max;
    using std::min;
    using std::to_string;
    using std::find;
    using std::floor;
    using std::ceil;
    using std::ofstream;
    using std::queue;
    using std::pair;
    using std::map;
    using std::tuple;

    using odb::dbBlock;
    using odb::dbBox;
    using odb::Rect;
    using odb::dbDatabase;
    using odb::dbMaster;
    using odb::dbMTerm;
    using odb::dbSet;
    using odb::dbSigType;
    using odb::dbInst;
    using odb::dbStringProperty;
    using odb::dbMPin;

    using sta::Instance;
    using sta::LeafInstanceIterator;
    using sta::Cell;
    using sta::InstanceChildIterator;
    using sta::Pin;
    using sta::PinSeq;
    using sta::Net;
    using sta::NetIterator;
    using sta::NetConnectedPinIterator;
    using sta::NetTermIterator;
    using sta::NetPinIterator;
    using sta::InstancePinIterator;
    using sta::PortDirection;
    using sta::Term;
    using sta::LibertyCell;
    using sta::NetConnectedPinIterator;
    using sta::LibertyCellPortIterator;
    using sta::LibertyPort;

    // ******************************************************************************
    // Class Cluster
    // ******************************************************************************
    float Cluster::calculateArea(ord::dbVerilogNetwork *network)
    {
        float area = 0.0;
        for(int i = 0; i < _inst_vec.size(); i++) {
            LibertyCell *liberty_cell = network->libertyCell(_inst_vec[i]);
            area += liberty_cell->area();
        }

        for(int i = 0; i < _macro_vec.size(); i++) {
            LibertyCell *liberty_cell = network->libertyCell(_macro_vec[i]);
            area += liberty_cell->area();
        }

        return area;
    }



    // ******************************************************************************
    // Class autoclusterMgr
    // ******************************************************************************
    //
    //  traverseLogicalHierarchy
    //  Recursive function to collect the design metrics (number of instances, hard macros, area)
    //  in the logical hierarchy
    //
    Metric autoclusterMgr::traverseLogicalHierarchy(sta::Instance* inst)
    {
        float area = 0.0;
        unsigned int num_inst = 0;
        unsigned int num_macro = 0;

        InstanceChildIterator *child_iter = _network->childIterator(inst);
        while(child_iter->hasNext())
        {
            Instance* child = child_iter->next();
            if(_network->isHierarchical(child)) {
                Metric metric = traverseLogicalHierarchy(child);
                area += metric.area;
                num_inst += metric.num_inst;
                num_macro += metric.num_macro;
            } else {
                LibertyCell *liberty_cell = _network->libertyCell(child);
                area += liberty_cell->area();
                if(liberty_cell->isBuffer()) {
                    _num_buffer += 1;
                    _area_buffer += liberty_cell->area();
                    _buffer_map[child] = ++_buffer_id;
                }

                Cell *cell = _network->cell(child);
                const char* cell_name = _network->name(cell);
                dbMaster *master = _db->findMaster(cell_name);
                if(master->isBlock())
                    num_macro += 1;
                else
                    num_inst += 1;
            }
        }
        Metric metric = Metric(area, num_macro, num_inst);
        _logical_cluster_map[inst] = metric;
        return metric;
    }

    bool isConnectedNet(pair<Net*, Net*>& p1, pair<Net*, Net*>& p2) 
    {
        if(p1.first != nullptr) {
            if(p1.first == p2.first)
                return true;
            else if(p1.first == p2.second)
                return true;
        }

        if(p1.second != nullptr) {
            if(p1.second == p2.first) 
                return true;
            else if(p1.second == p2.second)
                return true;
        }

        return false;
    }

    void appendNet(vector<Net*>& vec, pair<Net*, Net*> &p) 
    {
        if(p.first != nullptr) {
            if(find(vec.begin(), vec.end(), p.first) == vec.end()) 
                vec.push_back(p.first);
        }

        if(p.second != nullptr) {
            if(find(vec.begin(), vec.end(), p.second) == vec.end())
                vec.push_back(p.second);
        }
    }



    //
    // Handle Buffer transparency for handling net connection across buffers
    //  RV? Is this handling chain of buffers?
    //  TODO: handle inverter pairs
    //
    void autoclusterMgr::getBufferNet()
    {
        vector<pair<Net*, Net*> > buffer_net;
        for(int i = 0; i <= _buffer_id; i++) {
            buffer_net.push_back(pair<Net*, Net*>(nullptr, nullptr));
        }
        getBufferNetUtil(_network->topInstance(), buffer_net);
    
        int* class_array = new int[_buffer_id+1];
        for(int i = 0; i <= _buffer_id; i++)
            class_array[i] = i;

        int unique_id = 0;

        for(int i = 0; i <= _buffer_id; i++) {
            if(class_array[i] == i)
                class_array[i] = unique_id++;

            for(int j = i + 1; j <= _buffer_id; j++) {
                if(isConnectedNet(buffer_net[i], buffer_net[j]))
                    class_array[j] = class_array[i];
            }
        }
        
        for(int i = 0; i < unique_id; i++) {
            vector<Net*> vec_net;
            _buffer_net_vec.push_back(vec_net);
        }
        
        for(int i = 0; i<= _buffer_id; i++) {
            appendNet(_buffer_net_vec[class_array[i]], buffer_net[i]);
        }
    }



    void autoclusterMgr::getBufferNetUtil(Instance* inst, vector<pair<Net*, Net*> >& buffer_net)
    {
        bool is_top = (inst == _network->topInstance());
        NetIterator *net_iter = _network->netIterator(inst);
        while(net_iter->hasNext()) {
            Net *net = net_iter->next();
            bool with_buffer = false;
            if(is_top || !hasTerminals(net)) {
                NetConnectedPinIterator *pin_iter = _network->connectedPinIterator(net);
                while(pin_iter->hasNext()) {
                    Pin *pin = pin_iter->next();
                    if(_network->isLeaf(pin)) {
                        Instance *child_inst = _network->instance(pin);
                        LibertyCell *liberty_cell = _network->libertyCell(child_inst);
                        if(liberty_cell->isBuffer()) {
                            with_buffer = true;
                            int buffer_id = _buffer_map[child_inst];

                            if(buffer_net[buffer_id].first == nullptr) 
                                buffer_net[buffer_id].first = net;
                            else if(buffer_net[buffer_id].second == nullptr)
                                buffer_net[buffer_id].second = net;
                            else
                                _logger->error(PAR, 401, "Buffer Net has more than two net connection...");
                        }
                    }
                }

                if(with_buffer == true) {
                    _buffer_net_list.push_back(net);
                }
            }
        }

        delete net_iter;

        InstanceChildIterator *child_iter = _network->childIterator(inst);
        while(child_iter->hasNext()) {
            Instance* child = child_iter->next();
            getBufferNetUtil(child, buffer_net);
        }

        delete child_iter;
    }

    

    //
    //  Create a bundled model for external pins.  Group boundary pins into bundles
    //  Currently creates 3 groups per side
    //  TODO
    //     1) Make it customizable  number of bins per side
    //     2) Use area to determine bundles
    //     3) handle rectilinear boundaries and area pins (3D connections)
    //
    void autoclusterMgr::createBundledIO()
    {
        // Get the floorplan information


        Rect die_box;
        _block->getDieArea(die_box);
        
        _floorplan_lx = die_box.xMin();
        _floorplan_ly = die_box.yMin();
        _floorplan_ux = die_box.xMax();
        _floorplan_uy = die_box.yMax();

        // Map all the BTerms to bundledIOs: "L", "R", "B", "T"
        for(auto term : _block->getBTerms()) 
        {
            std::string bterm_name = term->getName();
            int lx = INT_MAX;
            int ly = INT_MAX;
            int ux = 0;
            int uy = 0;
            for(auto pin : term->getBPins()) {
                for(auto box : pin->getBoxes()) {
                    lx = min(lx, box->xMin());
                    ly = min(ly, box->yMin());
                    ux = max(ux, box->xMax());
                    uy = max(uy, box->yMax());
                }
            }

            if(lx == _floorplan_lx && (uy <= _floorplan_uy / 3)) 
                _bterm_map[bterm_name] = string("L1");
            else if(lx == _floorplan_lx && (ly >=  _floorplan_uy * 2 / 3)) 
                _bterm_map[bterm_name] = string("L3");
            else if(lx == _floorplan_lx) 
                _bterm_map[bterm_name] = string("L");
            else if(ux == _floorplan_ux && (uy <= _floorplan_uy / 3))
                _bterm_map[bterm_name] = string("R1");
            else if(ux == _floorplan_ux && (ly >= _floorplan_uy * 2 / 3))
                _bterm_map[bterm_name] = string("R3");
            else if(ux == _floorplan_ux)
                _bterm_map[bterm_name] = string("R");
            else if(ly == _floorplan_ly && (ux <= _floorplan_ux / 3))
                _bterm_map[bterm_name] = string("B1");
            else if(ly == _floorplan_ly && (lx >= _floorplan_ux * 2 / 3))
                _bterm_map[bterm_name] = string("B3");
            else if(ly == _floorplan_ly)
                _bterm_map[bterm_name] = string("B");
            else if(uy == _floorplan_uy && (ux <= _floorplan_ux / 3))
                _bterm_map[bterm_name] = string("T1");
            else if(uy == _floorplan_uy && (lx >= _floorplan_ux * 2 / 3))
                _bterm_map[bterm_name] = string("T3");
            else if(uy == _floorplan_uy)
                _bterm_map[bterm_name] = string("T");
            else
                _logger->error(PAR, 400, "Floorplan has not been initialized? Pin location error.");
        }
        
        // verify the correctness of  BTerm mapping
        //
        // RV Check??
        //
        unordered_map<string, string>::iterator term_map_iter;
        int num_L = 0;
        int num_L1 = 0;
        int num_L3 = 0;
        int num_R = 0;
        int num_R1 = 0;
        int num_R3 = 0;
        int num_B = 0;
        int num_B1 = 0;
        int num_B3 = 0;
        int num_T = 0;
        int num_T1 = 0;
        int num_T3 = 0;
        for(term_map_iter = _bterm_map.begin(); term_map_iter != _bterm_map.end(); term_map_iter++)
        {
            if(term_map_iter->second == string("L")) 
                num_L++;
            else if(term_map_iter->second == string("L1")) 
                num_L1++;
            else if(term_map_iter->second == string("L3")) 
                num_L3++;
            else if(term_map_iter->second == string("R"))
                num_R++;
            else if(term_map_iter->second == string("R1"))
                num_R1++;
            else if(term_map_iter->second == string("R3"))
                num_R3++;
            else if(term_map_iter->second == string("B"))
                num_B++;
            else if(term_map_iter->second == string("B1"))
                num_B1++;
            else if(term_map_iter->second == string("B3"))
                num_B3++;
            else if(term_map_iter->second == string("T1"))
                num_T1++;
            else if(term_map_iter->second == string("T3"))
                num_T3++;
            else
                num_T++;
        }
    }
        
    void autoclusterMgr::createCluster(int& cluster_id)
    {
        // This function will only be called by top instance
        Instance* inst = _network->topInstance();
        Metric metric = _logical_cluster_map[inst];
        bool is_hier = false;
        if(metric.num_macro > _max_num_macro || metric.num_inst > _max_num_inst)
        {
            InstanceChildIterator *child_iter = _network->childIterator(inst);
            vector<Instance*> glue_inst_vec;
            while(child_iter->hasNext())
            {
                Instance* child = child_iter->next();
                if(_network->isHierarchical(child)) {
                    createClusterUtil(child, cluster_id);
                    is_hier = true;
                }
                else
                    glue_inst_vec.push_back(child);
            }

            // Create cluster for glue logic
            if(glue_inst_vec.size() >= 1) {
                string name = "top";
                if(!is_hier)
                    name = name + "_glue_logic";
                Cluster* cluster = new Cluster(++cluster_id, !is_hier, name);
                vector<Instance*>::iterator vec_iter;
                for(vec_iter = glue_inst_vec.begin(); vec_iter != glue_inst_vec.end(); vec_iter++)
                {
                    LibertyCell *liberty_cell = _network->libertyCell(*vec_iter);
                    if(liberty_cell->isBuffer() == true) 
                        continue;

                    Cell *cell = _network->cell(*vec_iter);
                    const char* cell_name = _network->name(cell);
                    dbMaster *master = _db->findMaster(cell_name);
                    if(master->isBlock())
                        cluster->addMacro(*vec_iter);
                    else
                        cluster->addInst(*vec_iter);
                    _inst_map[*vec_iter] = cluster_id;
                }
                _cluster_map[cluster_id] = cluster;

                if(cluster->getNumInst() >= _min_num_inst || cluster->getNumMacro() >= _min_num_macro) {
                    _cluster_list.push_back(cluster);
                } else {
                    _merge_cluster_list.push_back(cluster);
                }
            }
        } else {
            // This no need to do any clustering
            Cluster* cluster = new Cluster(++cluster_id, true, string("top_instance"));
            _cluster_map[cluster_id] = cluster;
            cluster->specifyTopInst(inst);
            cluster->addLogicalModule(string("top_instance"));
            LeafInstanceIterator *leaf_iter = _network->leafInstanceIterator(inst);
            while(leaf_iter->hasNext())
            {
                Instance* leaf_inst = leaf_iter->next();
                LibertyCell *liberty_cell = _network->libertyCell(leaf_inst);
                if(liberty_cell->isBuffer() == false) {
                    Cell *cell = _network->cell(leaf_inst);
                    const char* cell_name = _network->name(cell);
                    dbMaster *master = _db->findMaster(cell_name);
                    if(master->isBlock())
                        cluster->addMacro(leaf_inst);
                    else
                        cluster->addInst(leaf_inst);
                    
                    _inst_map[leaf_inst] = cluster_id;
                } 
            }
            _cluster_list.push_back(cluster);
        }
    }
    
    
    void autoclusterMgr::createClusterUtil(Instance* inst, int& cluster_id)
    {
        Metric metric = _logical_cluster_map[inst];
        Cluster* cluster = new Cluster(++cluster_id, true, string(_network->pathName(inst)));
        cluster->specifyTopInst(inst);
        cluster->addLogicalModule(string(_network->pathName(inst)));
        _cluster_map[cluster_id] = cluster;
        LeafInstanceIterator *leaf_iter = _network->leafInstanceIterator(inst);
        while(leaf_iter->hasNext())
        {
            Instance* leaf_inst = leaf_iter->next();
            LibertyCell *liberty_cell = _network->libertyCell(leaf_inst);
            if(liberty_cell->isBuffer() == false) {
                Cell *cell = _network->cell(leaf_inst);
                const char* cell_name = _network->name(cell);
                dbMaster *master = _db->findMaster(cell_name);
                if(master->isBlock())
                    cluster->addMacro(leaf_inst);
                else
                    cluster->addInst(leaf_inst);
            
                _inst_map[leaf_inst] = cluster_id;
            }
        }
        
        if(cluster->getNumMacro() >= _max_num_macro || cluster->getNumInst() >= _max_num_inst) {
            _cluster_list.push_back(cluster);
            _break_cluster_list.push(cluster);
        } else if(cluster->getNumMacro() >= _min_num_macro || cluster->getNumInst() >= _min_num_inst) {
            _cluster_list.push_back(cluster);
        } else {
            _merge_cluster_list.push_back(cluster);
        }        
    }
   



    void autoclusterMgr::updateConnection()
    {
        unordered_map<int, Cluster*>::iterator map_it;
        for(map_it = _cluster_map.begin(); map_it != _cluster_map.end(); map_it++)
        {
            map_it->second->initConnection();
        }
        calculateConnection(_network->topInstance());
        calculateBufferNetConnection();
    }


    bool autoclusterMgr::hasTerminals(Net* net)
    {
        NetTermIterator *term_iter = _network->termIterator(net);
        bool has_terms = term_iter->hasNext();
        delete term_iter;
        return has_terms;
    }


    void autoclusterMgr::calculateBufferNetConnection()
    {
        for(int i = 0; i < _buffer_net_vec.size(); i++) {
            int driver_id = 0;
            vector<int> loads_id;
            vector<int>::iterator vec_iter;
            for(int j = 0; j < _buffer_net_vec[i].size(); j++) {
                Net *net = _buffer_net_vec[i][j];
                bool is_top = _network->instance(net) == _network->topInstance();
                if(is_top || !hasTerminals(net)) {
                    NetConnectedPinIterator *pin_iter = _network->connectedPinIterator(net);
                    while(pin_iter->hasNext()) {
                        Pin *pin = pin_iter->next();
                        if(_network->isTopLevelPort(pin)) {
                            const char *port_name = _network->portName(pin);
                            int id = _bundled_io_map[_bterm_map[string(port_name)]];
                            PortDirection *port_dir = _network->direction(pin);
                            if(port_dir == PortDirection::input()) {
                                driver_id = id;
                            } else {
                                vec_iter = find(loads_id.begin(), loads_id.end(), id);
                                if(vec_iter == loads_id.end())
                                    loads_id.push_back(id);
                            }
                        } else if(_network->isLeaf(pin)) {
                            Instance *inst = _network->instance(pin);
                            LibertyCell *liberty_cell = _network->libertyCell(inst);
                            if(liberty_cell->isBuffer() == false) {
                                PortDirection *port_dir = _network->direction(pin);
                                int id = _inst_map[inst];
                                if(port_dir == PortDirection::output()) {
                                    driver_id = id;
                                } else {
                                    vec_iter = find(loads_id.begin(), loads_id.end(), id);
                                    if(vec_iter == loads_id.end())
                                        loads_id.push_back(id);
                                }      
                            }
                        }
                    }
                }
            }
            
            if(driver_id != 0 && loads_id.size() > 0) {
                Cluster* driver_cluster = _cluster_map[driver_id];
                for(int i = 0; i < loads_id.size(); i++) {
                    _cluster_map[driver_id]->addOutputConnection(loads_id[i]);
                    _cluster_map[loads_id[i]]->addInputConnection(driver_id);
                }
            }
        }
    }



    void autoclusterMgr::calculateConnection(Instance* inst)
    {
        bool is_top = (inst == _network->topInstance());
        NetIterator *net_iter = _network->netIterator(inst);
        while(net_iter->hasNext()) {
            Net *net = net_iter->next();
            int driver_id = 0;
            vector<int> loads_id;
            vector<int>::iterator vec_iter;
            bool buffer_flag = false;
            if(find(_buffer_net_list.begin(), _buffer_net_list.end(), net) != _buffer_net_list.end())
                buffer_flag = true;


            if((buffer_flag == false) && (is_top || !hasTerminals(net))) {
                NetConnectedPinIterator *pin_iter = _network->connectedPinIterator(net);
                while(pin_iter->hasNext()) {
                    Pin *pin = pin_iter->next();
                    if(_network->isTopLevelPort(pin)) {
                        const char *port_name = _network->portName(pin);
                        int id = _bundled_io_map[_bterm_map[string(port_name)]];
                        PortDirection *port_dir = _network->direction(pin);
                        if(port_dir == PortDirection::input()) {
                            driver_id = id;
                        } else {
                            vec_iter = find(loads_id.begin(), loads_id.end(), id);
                            if(vec_iter == loads_id.end())
                            loads_id.push_back(id);
                        }
                    } else if(_network->isLeaf(pin)) {
                        Instance *inst = _network->instance(pin);
                        PortDirection *port_dir = _network->direction(pin);
                        int id = _inst_map[inst];
                        if(port_dir == PortDirection::output()) {
                            driver_id = id;
                        } else {
                            vec_iter = find(loads_id.begin(), loads_id.end(), id);
                            if(vec_iter == loads_id.end())
                                loads_id.push_back(id);
                        }
                    }
                }
                
                if(driver_id != 0 && loads_id.size() > 0) {
                    Cluster* driver_cluster = _cluster_map[driver_id];
                    for(int i = 0; i < loads_id.size(); i++) {
                        _cluster_map[driver_id]->addOutputConnection(loads_id[i]);
                        _cluster_map[loads_id[i]]->addInputConnection(driver_id);
                    }
                }
            }
        }
        
        delete net_iter;
        
        InstanceChildIterator *child_iter = _network->childIterator(inst);
        while(child_iter->hasNext()) {
            Instance* child = child_iter->next();
            calculateConnection(child);
        }

        delete child_iter;
    }



    void autoclusterMgr::merge(string parent_name)
    {
        if(_merge_cluster_list.size() == 0)
            return;
        
        if(_merge_cluster_list.size() == 1) {
            _cluster_list.push_back(_merge_cluster_list[0]);
            _merge_cluster_list.clear();
            updateConnection();
            return;
        }

        unsigned int num_inst = calculateClusterNumInst(_merge_cluster_list);
        unsigned int num_macro = calculateClusterNumMacro(_merge_cluster_list);
        int iteration = 0;
        int merge_index = 0;
        while(num_inst > _max_num_inst || num_macro > _max_num_macro) {
            int num_merge_cluster = _merge_cluster_list.size();
            mergeUtil(parent_name, merge_index);
            if(num_merge_cluster == _merge_cluster_list.size())
                break;
            
            num_inst = calculateClusterNumInst(_merge_cluster_list);
            num_macro = calculateClusterNumMacro(_merge_cluster_list);
        }
      

        if(_merge_cluster_list.size() > 1) 
            for(int i = 1; i < _merge_cluster_list.size(); i++) {
                mergeCluster(_merge_cluster_list[0], _merge_cluster_list[i]);
                delete _merge_cluster_list[i];
            }
        
        if(_merge_cluster_list.size() > 0) {
            _merge_cluster_list[0]->specifyName(parent_name + string("_cluster_") + to_string(merge_index++));
            _cluster_list.push_back(_merge_cluster_list[0]);
            _merge_cluster_list.clear();
        }
        updateConnection();
    }

        
    unsigned int autoclusterMgr::calculateClusterNumMacro(vector<Cluster*> &cluster_vec)
    {
        unsigned int num_macro = 0;
        for(int i = 0; i < cluster_vec.size(); i++)
            num_macro += cluster_vec[i]->getNumMacro();

        return num_macro;
    }


    unsigned int autoclusterMgr::calculateClusterNumInst(vector<Cluster*> &cluster_vec)
    {   
        unsigned int num_inst = 0;
        for(int i = 0; i < cluster_vec.size(); i++)
            num_inst += cluster_vec[i]->getNumInst();
        
        return num_inst;
    }
    
    //
    // Merge target cluster into src
    // RV -- del targe cluster?
    //
    void autoclusterMgr::mergeCluster(Cluster* src, Cluster* target)
    {
        int src_id = src->getId();
        int target_id = target->getId();
        _cluster_map.erase(target_id);
        src->addLogicalModuleVec(target->getLogicalModuleVec());
        vector<Instance*> inst_vec = target->getInstVec();
        for(int i = 0; i < inst_vec.size(); i++)
        {
            src->addInst(inst_vec[i]);
            _inst_map[inst_vec[i]] = src_id;
        }
        
        vector<Instance*> macro_vec = target->getMacroVec();
        for(int i = 0; i < macro_vec.size(); i++)
        {
            src->addMacro(macro_vec[i]);
            _inst_map[macro_vec[i]] = src_id;
        }
    }

    void autoclusterMgr::mergeUtil(string parent_name, int& merge_index)
    {
        vector<int> outside_vec;
        vector<int> merge_vec;

        for(int i = 0; i < _cluster_list.size(); i++)
            outside_vec.push_back(_cluster_list[i]->getId());

        for(int i = 0; i < _merge_cluster_list.size(); i++)
            merge_vec.push_back(_merge_cluster_list[i]->getId());

        int M = merge_vec.size();
        int N = outside_vec.size();
        bool* internal_flag = new bool[M];
        int* class_id = new int[M];
        bool** graph = new bool*[M];
        
        for(int i = 0; i < M; i++) {
            graph[i] = new bool[N];
            internal_flag[i] = true;
            class_id[i] = i;
        }

        for(int i = 0; i < M; i++)
            for(int j = 0; j < N; j++) {
                unsigned int input = _merge_cluster_list[i]->getInputConnection(outside_vec[j]);
                unsigned int output = _merge_cluster_list[i]->getOutputConnection(outside_vec[j]);
                if(input + output > _net_threshold) {
                    graph[i][j] = true;
                    internal_flag[i] = false;
                }
            }
        
        for(int i = 0; i < M; i++) {
            if(internal_flag[i] == false && class_id[i] == i){
                for(int j = i + 1; j < M; j++) {
                    bool flag = true;
                    for(int k = 0; k < N; k++) {
                        if(flag == false)
                            break;
                        flag = flag && (graph[i][k] == graph[j][k]);
                    }
                    if(flag == true)
                        class_id[j] = i;
                }
            }
        }

        // Merge clusters with same connection topology
        for(int i = 0; i < M; i++)
            if(internal_flag[i] == false && class_id[i] == i) {
                for(int j = i + 1; j < M; j++) {
                    if(class_id[j] == i) {
                        mergeCluster(_merge_cluster_list[i], _merge_cluster_list[j]);
                    }
                }
            }
        
    
        vector<Cluster*> temp_cluster_vec;
        for(int i = 0; i < M; i++) {
            if(class_id[i] == i) {
                int num_inst = _merge_cluster_list[i]->getNumInst();
                int num_macro = _merge_cluster_list[i]->getNumMacro();
                if(num_inst >= _min_num_inst || num_macro >= _min_num_macro) {
                    _cluster_list.push_back(_merge_cluster_list[i]);
                    _merge_cluster_list[i]->specifyName(parent_name + string("_cluster_") + to_string(merge_index++));
                } else {
                    temp_cluster_vec.push_back(_merge_cluster_list[i]);
                }
            } else {
                delete _merge_cluster_list[i];
            }
        }
 
        _merge_cluster_list.clear();
        _merge_cluster_list = temp_cluster_vec;
        
        updateConnection();
    }

    //
    // Break a cluser (logical module) into its child modules and create a cluster each of the child modules
    //
    void autoclusterMgr::breakCluster(Cluster* cluster_old, int& cluster_id)
    {
        Instance* inst = cluster_old->getTopInstance();
        InstanceChildIterator *child_iter = _network->childIterator(inst);
        vector<Instance*> glue_inst_vec;
        bool is_hier = false;
        while(child_iter->hasNext()) {
            Instance* child = child_iter->next();
            if(_network->isHierarchical(child)) {
                is_hier = true;
                createClusterUtil(child, cluster_id);
            } else 
                glue_inst_vec.push_back(child);
        }

        if(!is_hier) {
            return;
        }
        
        // Create cluster for glue logic
        if(glue_inst_vec.size() >= 1) {
            string name = _network->pathName(inst) + string("_glue_logic");
            Cluster* cluster = new Cluster(++cluster_id, false, name);
            vector<Instance*>::iterator vec_iter;
            for(vec_iter = glue_inst_vec.begin(); vec_iter != glue_inst_vec.end(); vec_iter++) {
                LibertyCell *liberty_cell = _network->libertyCell(*vec_iter);
                if(liberty_cell->isBuffer() == false) {
                    Cell *cell = _network->cell(*vec_iter);
                    const char* cell_name = _network->name(cell);
                    dbMaster *master = _db->findMaster(cell_name);
                    if(master->isBlock())
                        cluster->addMacro(*vec_iter);
                    else
                        cluster->addInst(*vec_iter);
                    
                    _inst_map[*vec_iter] = cluster_id;
                }
            }
            _cluster_map[cluster_id] = cluster;
                
            //
            // Check cluster size. If it is smaller than min_inst threshold, add it to merge_cluster list
            // 
            if(cluster->getNumInst() >= _min_num_inst || cluster->getNumMacro() >= _min_num_macro) {
                _cluster_list.push_back(cluster);
            } else { 
                _merge_cluster_list.push_back(cluster);
            }
        }
        

        _cluster_map.erase(cluster_old->getId());
        vector<Cluster*>::iterator vec_it = find(_cluster_list.begin(), _cluster_list.end(), cluster_old);
        _cluster_list.erase(vec_it);
        delete cluster_old;
        updateConnection();
        merge(string(_network->pathName(inst)));
    }
    
    //
    // For clusters that are greater than max_inst threshold, use MLPart to break the cluster into smaller clusters
    //
    void autoclusterMgr::MLPart(Cluster* cluster, int &cluster_id)
    {
        int num_inst = cluster->getNumInst();
        if(num_inst < 2 * _min_num_inst) 
            return;
        
        _cluster_map.erase(cluster->getId());
        vector<Cluster*>::iterator vec_it = find(_cluster_list.begin(), _cluster_list.end(), cluster);
        _cluster_list.erase(vec_it);
        
        int src_id = cluster->getId();
        unordered_map<int, Instance*> idx_to_inst;
        unordered_map<Instance*, int> inst_to_idx;
        vector<double> vertex_weight;
        vector<double> edge_weight;
        vector<int> col_idx;  // edges represented by vertex indices
        vector<int> row_ptr;  // pointers for edges
        int inst_id = 0;
        unordered_map<Cluster*, int> node_map;
        // we also consider outside world
        for(int i = 0; i < _cluster_list.size(); i++) {
            vertex_weight.push_back(1.0);
            node_map[_cluster_list[i]] = inst_id++;
        }
        
        vector<Instance*> inst_vec = cluster->getInstVec();
        for(int i = 0; i < inst_vec.size(); i++) {
            idx_to_inst[inst_id++] = inst_vec[i];
            inst_to_idx[inst_vec[i]] == inst_id;
            vertex_weight.push_back(1.0);
        }
            

        int count = 0; 
        MLPartNetUtil(_network->topInstance(), src_id, count, col_idx, row_ptr, edge_weight, node_map, idx_to_inst, inst_to_idx);
        MLPartBufferNetUtil(src_id, count, col_idx, row_ptr, edge_weight, node_map, idx_to_inst, inst_to_idx);

        row_ptr.push_back(count);
        
        // Convert it to MLPart Format
        int num_vertice = vertex_weight.size();
        int num_edge = row_ptr.size() - 1;
        int num_col_idx = col_idx.size();

        double* vertexWeight = (double*) malloc((unsigned) num_vertice * sizeof(double));
        int* rowPtr = (int*) malloc((unsigned) (num_edge + 1) * sizeof(int));
        int* colIdx = (int*) malloc((unsigned) (num_col_idx) * sizeof(int));
        double* edgeWeight = (double*) malloc((unsigned) num_edge * sizeof(double));
        int* part = (int*) malloc((unsigned) num_vertice * sizeof(int));

        for(int i = 0; i < num_vertice; i++) {
            part[i] = -1;
            vertexWeight[i] = 1.0;
        }

        for(int i = 0; i < num_edge; i++) {
            edgeWeight[i] = 1.0;
            rowPtr[i] = row_ptr[i];
        }

        rowPtr[num_edge] = row_ptr[num_edge];

        for(int i = 0; i < num_col_idx; i++) 
            colIdx[i] = col_idx[i];

        //MLPart only support 2-way partition
        int npart = 2;
        double balanceArray[2] = {0.5, 0.5};
        double tolerance = 0.05;
        unsigned int seed = 0;

        
        UMpack_mlpart(num_vertice,
                      num_edge,
                      vertexWeight,
                      rowPtr,
                      colIdx,
                      edgeWeight,
                      npart,  // Number of Partitions
                      balanceArray,
                      tolerance,
                      part,
                      1,  // Starts Per Run #TODO: add a tcl command
                      1,  // Number of Runs
                      0,  // Debug Level
                      seed);
        
        string name_part0 = cluster->getName() + string("_cluster_0"); 
        string name_part1 = cluster->getName() + string("_cluster_1");
        Cluster* cluster_part0 = new Cluster(++cluster_id, true, name_part0);
        int id_part0 = cluster_id;
        _cluster_map[id_part0] = cluster_part0;
        _cluster_list.push_back(cluster_part0);
        Cluster* cluster_part1 = new Cluster(++cluster_id, true, name_part1);
        int id_part1 = cluster_id;
        _cluster_map[id_part1] = cluster_part1;
        _cluster_list.push_back(cluster_part1);
        cluster_part0->addLogicalModuleVec(cluster->getLogicalModuleVec());
        cluster_part1->addLogicalModuleVec(cluster->getLogicalModuleVec());

        for(int i = _cluster_list.size() - 2; i < num_vertice; i++) {
            if(part[i] == 0) {
                cluster_part0->addInst(idx_to_inst[i]);
                _inst_map[idx_to_inst[i]] = id_part0;
            } else {
                cluster_part1->addInst(idx_to_inst[i]);
                _inst_map[idx_to_inst[i]] = id_part1;
            }
        }
        
        if(cluster_part0->getNumInst() > _max_num_inst)
            _mlpart_cluster_list.push(cluster_part0);

        if(cluster_part1->getNumInst() > _max_num_inst)
            _mlpart_cluster_list.push(cluster_part1);
        
        delete cluster;
    }

    void autoclusterMgr::MLPartNetUtil(Instance* inst, int &src_id,  int &count, 
                         vector<int> &col_idx, vector<int> &row_ptr,
                         vector<double> &edge_weight, unordered_map<Cluster*, int> &node_map, 
                         unordered_map<int, Instance*> &idx_to_inst, unordered_map<Instance*, int> &inst_to_idx)
    {
        bool is_top = (inst == _network->topInstance());
        NetIterator *net_iter = _network->netIterator(inst);
        while(net_iter->hasNext()) {
            Net *net = net_iter->next();
            int driver_id = -1;
            vector<int> loads_id;
            vector<int>::iterator vec_iter;
            bool buffer_flag = false;
            if(find(_buffer_net_list.begin(), _buffer_net_list.end(), net) != _buffer_net_list.end())
                buffer_flag = true;


            if((buffer_flag == false) && (is_top || !hasTerminals(net))) {
                NetConnectedPinIterator *pin_iter = _network->connectedPinIterator(net);
                while(pin_iter->hasNext()) {
                    Pin *pin = pin_iter->next();
                    if(_network->isTopLevelPort(pin)) {
                        const char *port_name = _network->portName(pin);
                        int id = _bundled_io_map[_bterm_map[string(port_name)]];
                        id = node_map[_cluster_map[id]];
                        PortDirection *port_dir = _network->direction(pin);
                        if(port_dir == PortDirection::input()) {
                            driver_id = id;
                        } else {
                            vec_iter = find(loads_id.begin(), loads_id.end(), id);
                            if(vec_iter == loads_id.end())
                            loads_id.push_back(id);
                        }
                    } else if(_network->isLeaf(pin)) {
                        Instance *inst = _network->instance(pin);
                        PortDirection *port_dir = _network->direction(pin);
                        int id = _inst_map[inst];
                        if(id == src_id)
                            id = inst_to_idx[inst];
                        else
                            id = node_map[_cluster_map[id]];
                        if(port_dir == PortDirection::output()) {
                            driver_id = id;
                        } else {
                            vec_iter = find(loads_id.begin(), loads_id.end(), id);
                            if(vec_iter == loads_id.end())
                                loads_id.push_back(id);
                        }
                    }
                }
                
                if(driver_id != -1 && loads_id.size() > 0) {
                    row_ptr.push_back(count);
                    edge_weight.push_back(1.0);
                    col_idx.push_back(driver_id);
                    count++;
                    for(int i = 0; i < loads_id.size(); i++) {
                        col_idx.push_back(loads_id[i]);
                        count++;
                    }
                }
            }
        }
        
        delete net_iter;
        

        InstanceChildIterator *child_iter = _network->childIterator(inst);
        while(child_iter->hasNext()) {
            Instance* child = child_iter->next();
            MLPartNetUtil(child, src_id, count, col_idx, row_ptr, edge_weight, node_map, idx_to_inst, inst_to_idx);
        }

        delete child_iter;
    }
    
    
    void autoclusterMgr::MLPartBufferNetUtil(int &src_id,  int &count, 
                         vector<int> &col_idx, vector<int> &row_ptr,
                         vector<double> &edge_weight, unordered_map<Cluster*, int> &node_map, 
                         unordered_map<int, Instance*> &idx_to_inst, unordered_map<Instance*, int> &inst_to_idx)
    {
        for(int i = 0;  i < _buffer_net_vec.size(); i++) {
            int driver_id = -1;
            vector<int> loads_id;
            vector<int>::iterator vec_iter;
            for(int j = 0; j < _buffer_net_vec[i].size(); j++) {
                Net *net = _buffer_net_vec[i][j];
                bool is_top = _network->instance(net) == _network->topInstance();
                if(is_top || !hasTerminals(net)) {
                    NetConnectedPinIterator *pin_iter = _network->connectedPinIterator(net);
                    while(pin_iter->hasNext()) {
                        Pin *pin = pin_iter->next();
                        if(_network->isTopLevelPort(pin)) {
                            const char *port_name = _network->portName(pin);
                            int id = _bundled_io_map[_bterm_map[string(port_name)]];
                            id = node_map[_cluster_map[id]];
                            PortDirection *port_dir = _network->direction(pin);
                            if(port_dir == PortDirection::input()) {
                                driver_id = id;
                            } else {
                                vec_iter = find(loads_id.begin(), loads_id.end(), id);
                                if(vec_iter == loads_id.end())
                                    loads_id.push_back(id);
                            }
                        } else if(_network->isLeaf(pin)) {
                            Instance *inst = _network->instance(pin);
                            LibertyCell *liberty_cell = _network->libertyCell(inst);
                            if(liberty_cell->isBuffer() == false) {
                                PortDirection *port_dir = _network->direction(pin);
                                int id = _inst_map[inst];
                                if(id == src_id)
                                    id = inst_to_idx[inst];
                                else
                                    id = node_map[_cluster_map[id]];
                                
                                if(port_dir == PortDirection::output()) {
                                    driver_id = id;
                                } else {
                                    vec_iter = find(loads_id.begin(), loads_id.end(), id);
                                    if(vec_iter == loads_id.end())
                                        loads_id.push_back(id);
                                }
                            }
                        }
                    }
                }
            } 
            
            if(driver_id != -1 && loads_id.size() > 0) {
                row_ptr.push_back(count);
                edge_weight.push_back(1.0);
                col_idx.push_back(driver_id);
                count++;
                for(int i = 0; i < loads_id.size(); i++) {
                    col_idx.push_back(loads_id[i]);
                    count++;
                }
            }
        }
    }
 
    
    //
    //  For a cluster that contains macros, further split groups based on macro size.
    //  Identical size macros are grouped together
    //
    void autoclusterMgr::MacroPart(Cluster* cluster_old, int &cluster_id)
    {
        //cout << "Enter macro part:  " << endl;
        vector<Instance*> macro_vec = cluster_old->getMacroVec();
        map<int, vector<Instance*> > macro_map;
        for(int i = 0; i < macro_vec.size(); i++) {
            Cell *cell = _network->cell(macro_vec[i]);
            const char* cell_name = _network->name(cell);
            dbMaster *master = _db->findMaster(cell_name);
            int area = master->getWidth() * master->getHeight();
            if(macro_map.find(area) != macro_map.end()) {
                macro_map[area].push_back(macro_vec[i]);
            } else {
                vector<Instance*> temp_vec;
                temp_vec.push_back(macro_vec[i]);
                macro_map[area] = temp_vec;
            }
        }
        
        string parent_name = cluster_old->getName();
        int part_id = 0;
        map<int, vector<Instance*> >::iterator map_it;


        vector<int> cluster_id_list;
        for(map_it = macro_map.begin(); map_it != macro_map.end(); map_it++) {
            vector<Instance*> temp_vec = map_it->second;
            string name = parent_name + "_part_" + to_string(part_id++);
            Cluster* cluster = new Cluster(++cluster_id, true, name);
            cluster_id_list.push_back(cluster->getId());
            cluster->addLogicalModule(parent_name);
            _cluster_list.push_back(cluster);
            _cluster_map[cluster_id] = cluster;
            _virtual_map[cluster->getId()] = _virtual_map[cluster_old->getId()];
            for(int i = 0; i < temp_vec.size(); i++) {
                _inst_map[temp_vec[i]] = cluster_id;
                cluster->addMacro(temp_vec[i]);
            }
        }
        
        for(int i = 0; i < cluster_id_list.size(); i++)
            for(int j = i + 1; j < cluster_id_list.size(); j++)
            {
                _virtual_map[cluster_id_list[i]] = cluster_id_list[j];
                //cout << "Add virtual weight:   " << _cluster_map[cluster_id_list[i]]->getName() << "    ";
                //cout << _cluster_map[cluster_id_list[j]]->getName() << "   " << endl;
            } 
            
        _cluster_map.erase(cluster_old->getId());
        _virtual_map.erase(cluster_old->getId());
        vector<Cluster*>::iterator vec_it = find(_cluster_list.begin(), _cluster_list.end(), cluster_old);
        _cluster_list.erase(vec_it);
        delete cluster_old;
    }

    
    void autoclusterMgr::printMacroCluster(Cluster* cluster_old, int& cluster_id)
    {
        queue<Cluster*> temp_cluster_queue;
        vector<Instance*> macro_vec = cluster_old->getMacroVec();
        string module_name = cluster_old->getName();
        for(int i = 0; i < module_name.size(); i++)  
        {
            if(module_name[i] == '/')
                module_name[i] = '*';
        }
    
        string block_file_name = string("./rtl_mp/") + module_name + string(".txt.block");
        string net_file_name = string("./rtl_mp/") +  module_name + string(".txt.net");

        ofstream output_file;
        output_file.open(block_file_name.c_str());
        for(int i = 0; i < macro_vec.size(); i++)
        {
            pair<float, float> pin_pos = printPinPos(macro_vec[i]);
            Cell *cell = _network->cell(macro_vec[i]);
            const char* cell_name = _network->name(cell);
            dbMaster *master = _db->findMaster(cell_name);
            float width = master->getWidth() / _dbu;
            float height = master->getHeight() / _dbu;
            output_file << _network->pathName(macro_vec[i]) << "  ";
            output_file << width << "   " << height << "    ";
            output_file << pin_pos.first << "   " << pin_pos.second <<  "  ";
            output_file << endl;
            Cluster* cluster = new Cluster(++cluster_id, true, _network->pathName(macro_vec[i]));
            _cluster_map[cluster_id] = cluster;
            _inst_map[macro_vec[i]] = cluster_id;
            cluster->addMacro(macro_vec[i]);
            temp_cluster_queue.push(cluster);
            _cluster_list.push_back(cluster);
        }
        
        output_file.close();
        updateConnection();

        output_file.open(net_file_name.c_str());
        int net_id = 0;
        unordered_map<int, Cluster*>::iterator map_iter = _cluster_map.begin();
        while(map_iter != _cluster_map.end()) {
            int src_id = map_iter->first;
            unordered_map<int, unsigned int> connection_map = map_iter->second->getOutputConnectionMap();
            unordered_map<int, unsigned int>::iterator iter = connection_map.begin();
            bool flag = true;
            while(iter != connection_map.end()) {
                if(iter->first != src_id) {
                    if(flag == true) {
                        output_file << endl;
                        output_file << "Net_" << ++net_id << ":  " << endl;
                        output_file << "source: " << map_iter->second->getName() << "   ";
                        flag = false;
                    }
                    output_file << _cluster_map[iter->first]->getName() << "   " << iter->second << "   ";
                }
                iter++;
            }
            //output_file << endl;
            map_iter++;
        }
        output_file << endl;
        output_file.close();
        
        while(!temp_cluster_queue.empty()) {
            Cluster* cluster = temp_cluster_queue.front();
            temp_cluster_queue.pop();
            _cluster_map.erase(cluster->getId());
            vector<Cluster*>::iterator vec_it = find(_cluster_list.begin(), _cluster_list.end(), cluster);
            _cluster_list.erase(vec_it);
            delete cluster;
        }


        for(int i = 0; i < macro_vec.size(); i++)
        {
            _inst_map[macro_vec[i]] = cluster_old->getId();
        }
    }


    pair<float, float> autoclusterMgr::printPinPos(Instance* macro_inst) 
    {
        float dbu = _db->getTech()->getDbUnitsPerMicron();
        int lx = 100000000;
        int ly = 100000000;
        int ux = 0;
        int uy = 0;
        Cell *cell = _network->cell(macro_inst);
        const char* cell_name = _network->name(cell);
        dbMaster *master = _db->findMaster(cell_name);
        for(dbMTerm* mterm : master->getMTerms()) {
            if(mterm->getSigType() == 0) {
                for(dbMPin* mpin : mterm->getMPins()) {
                    for(dbBox* box : mpin->getGeometry()) {
                        lx = min(lx, box->xMin());
                        ly = min(ly, box->yMin());
                        ux = max(ux, box->xMax());
                        uy = max(uy, box->yMax());
                    }
                }
            }
        }
        float x_center = (lx + ux) / (2 * dbu);
        float y_center = (ly + uy) / (2 * dbu);
        return std::pair<float, float>(x_center, y_center);
    }
    

    void autoclusterMgr::mergeMacro(string parent_name, int std_cell_id)
    {
        if(_merge_cluster_list.size() == 0)
            return;
 
        if(_merge_cluster_list.size() == 1) {
            _virtual_map[_merge_cluster_list[0]->getId()] = std_cell_id;
            _cluster_list.push_back(_merge_cluster_list[0]);
            _merge_cluster_list.clear();
            return;
        }
        
        int merge_index = 0;
        mergeMacroUtil(parent_name, merge_index, std_cell_id);
    }

    void autoclusterMgr::mergeMacroUtil(string parent_name, int& merge_index, int std_cell_id)
    {
        vector<int> outside_vec;
        vector<int> merge_vec;

        for(int i = 0; i < _cluster_list.size(); i++)
            outside_vec.push_back(_cluster_list[i]->getId());

        for(int i = 0; i < _merge_cluster_list.size(); i++)
            merge_vec.push_back(_merge_cluster_list[i]->getId());

        int M = merge_vec.size();
        int N = outside_vec.size();
        int* class_id = new int[M];
        bool** graph = new bool*[M];
        
        for(int i = 0; i < M; i++) {
            graph[i] = new bool[N];
            class_id[i] = i;
        }

        for(int i = 0; i < M; i++) {
            for(int j = 0; j < N; j++) {
                unsigned int input = _merge_cluster_list[i]->getInputConnection(outside_vec[j]);
                unsigned int output = _merge_cluster_list[i]->getOutputConnection(outside_vec[j]);
                if(input + output > _net_threshold) {
                    graph[i][j] = true;
                } else {
                    graph[i][j] = false;
                }
            }
        }


        for(int i = 0; i < M; i++) {
            if(class_id[i] == i){
                for(int j = i + 1; j < M; j++) {
                    bool flag = true;
                    for(int k = 0; k < N; k++) {
                        if(flag == false)
                            break;
                        flag = flag && (graph[i][k] == graph[j][k]);
                    }
                    if(flag == true)
                        class_id[j] = i;
                }
            }
        }

        // Merge clusters with same connection topology
        for(int i = 0; i < M; i++)
            if(class_id[i] == i) {
                for(int j = i + 1; j < M; j++) {
                    if(class_id[j] == i) {
                        mergeCluster(_merge_cluster_list[i], _merge_cluster_list[j]);
                    }
                }
            }
       
        vector<int> cluster_id_list;
        for(int i = 0; i < M; i++) {
            if(class_id[i] == i) {
                _cluster_list.push_back(_merge_cluster_list[i]);
                _virtual_map[_merge_cluster_list[i]->getId()] = std_cell_id;
                _merge_cluster_list[i]->specifyName(parent_name + string("_cluster_") + to_string(merge_index++));
                cluster_id_list.push_back(_merge_cluster_list[i]->getId());
            } else {
                delete _merge_cluster_list[i];
            }
        }

        //for(int i = 0; i < cluster_id_list.size(); i++)
        //    for(int j = i + 1; j < cluster_id_list.size(); j++)
        //        _virtual_map[cluster_id_list[i]] = cluster_id_list[j];

        _merge_cluster_list.clear();
    }

 


    //
    //  Auto clustering by traversing the design hierarchy
    //
    //  Parameters:
    //     max_num_macro, min_num_macro:   max and min number of marcos in a macro cluster.
    //     max_num_inst min_num_inst:  max and min number of std cell instances in a soft cluster. If
    //     a logical module has greater than the max threshold of instances, we descend down
    //     the hierarchy to examine the children. If multiple clusters are created for child modules that are
    //     smaller than the min threshold value, we merge them based on connectivity signatures
    //
    void autoclusterMgr::partitionDesign(unsigned int max_num_macro, unsigned int min_num_macro,
                                         unsigned int max_num_inst,  unsigned int min_num_inst,
                                         unsigned int net_threshold, unsigned int virtual_weight,
                                         unsigned int ignore_net_threshold,
                                         const char* file_name
                                        ) 
    {
        _logger->report("Running Partition Design...");
        mkdir("rtl_mp", 0777);
        
        _block = _db->getChip()->getBlock();
        _dbu = _db->getTech()->getDbUnitsPerMicron();
        _max_num_macro = max_num_macro;
        _min_num_macro = min_num_macro;
        _max_num_inst  = max_num_inst;
        _min_num_inst  = min_num_inst;
        _net_threshold = net_threshold;
        _virtual_weight = virtual_weight;
       
        createBundledIO();
        int cluster_id = 0;
        
            
        //
        // Map each boundled IO to cluster with zero area
        // Create a cluster for each budled io
        //
        vector<std::string> bundled_ios { string("L"), string("R"), string("T"), string("B"),
                                          string("L1"), string("R1"), string("T1"), string("B1"),
                                          string("L3"), string("R3"), string("T3"), string("B3")
                                        };
        
        for(auto io : bundled_ios) {
            Cluster* cluster = new Cluster(++cluster_id, true, io);
            _bundled_io_map[io] = cluster_id;
            _cluster_map[cluster_id] = cluster;
            _cluster_list.push_back(cluster);
        }

        Metric metric = traverseLogicalHierarchy(_network->topInstance());
        _logger->info(PAR, 402, 
            "Traversed Logical Hierarchy \n\t Number of std cell instances: {}\n\t Total Area: {}\n\t Number of Hard Macros: {}\n\t ", 
            metric.num_inst, metric.area, metric.num_macro);
       
        // get all the nets with buffers
        getBufferNet();

        // Break down the top-level instance
        createCluster(cluster_id);
        updateConnection(); 
        merge("top");
        
        //
        // Break down clusters
        // Walk down the tree and create clusters for logical modules
        // Stop when the clusters are smaller than the max size threshold
        //
        while(!_break_cluster_list.empty()) {
            Cluster* cluster = _break_cluster_list.front();
            _break_cluster_list.pop();
            breakCluster(cluster, cluster_id);
        }

        //
        // Use MLPart to partition large clusters
        // For clusters that are larger than max threshold size (flat insts) break down the cluster
        // by netlist partitioning using MLPart
        //
        for(int i = 0; i < _cluster_list.size(); i++) {
            if(_cluster_list[i]->getNumInst() > _max_num_inst) {
                _mlpart_cluster_list.push(_cluster_list[i]);
            }
        }
        
        while(!_mlpart_cluster_list.empty()) {
            Cluster* cluster = _mlpart_cluster_list.front();
            _mlpart_cluster_list.pop();
            MLPart(cluster, cluster_id);
        }

        //
        // split the macros and std cells
        // For clusters that contains HM and std cell -- split the cluster into two
        // a HM part and a std cell part
        //
        vector<Cluster*> par_cluster_vec;
        for(int i = 0; i < _cluster_list.size(); i++)
            if(_cluster_list[i]->getNumMacro() > 0)
                par_cluster_vec.push_back(_cluster_list[i]);

        for(int i = 0; i < par_cluster_vec.size(); i++) {
            Cluster* cluster_old = par_cluster_vec[i];
            int id = (-1) * cluster_old->getId();
            _virtual_map[id] = cluster_old->getId();
            string name = cluster_old->getName() + string("_macro");
            Cluster* cluster = new Cluster(id, true, name);
            cluster->addLogicalModule(name);
            _cluster_map[id] = cluster;
            vector<Instance*> macro_vec = cluster_old->getMacroVec();
            for(int j = 0; j < macro_vec.size(); j++) {
                _inst_map[macro_vec[j]] = id;
                cluster->addMacro(macro_vec[j]); 
            }
            _cluster_list.push_back(cluster);
            name = cluster_old->getName() + string("_std_cell");
            cluster_old->specifyName(name);
            cluster_old->removeMacro();
        }
        par_cluster_vec.clear();
        updateConnection();


        //
        // group macros based on connection signature
        // Use connection signatures to group and split macros
        //
        queue<Cluster*> par_cluster_queue;
        for(int i = 0; i < _cluster_list.size(); i++) 
            if(_cluster_list[i]->getNumMacro() > 0)
                par_cluster_queue.push(_cluster_list[i]);

        while(!par_cluster_queue.empty()) {
            Cluster* cluster_old = par_cluster_queue.front();
            par_cluster_queue.pop();
            vector<Instance*> macro_vec = cluster_old->getMacroVec();
            string name = cluster_old->getName();
            for(int i = 0; i < macro_vec.size(); i++) {
                Cluster* cluster = new Cluster(++cluster_id, true, _network->pathName(macro_vec[i]));
                cluster->addLogicalModule(_network->pathName(macro_vec[i]));
                _cluster_map[cluster_id] = cluster;
                _inst_map[macro_vec[i]] = cluster_id;
                cluster->addMacro(macro_vec[i]);
                _merge_cluster_list.push_back(cluster);
            }
            int std_cell_id = _virtual_map[cluster_old->getId()];
            _virtual_map.erase(cluster_old->getId());
            _cluster_map.erase(cluster_old->getId());
            vector<Cluster*>::iterator vec_it = find(_cluster_list.begin(), _cluster_list.end(), cluster_old);
            _cluster_list.erase(vec_it);
            delete cluster_old;
            updateConnection();
            mergeMacro(name, std_cell_id);
        }
       
    
        //
        // group macros based on area footprint, This will allow for more efficient tiling with
        // limited wasted space between the macros
        //
        for(int i = 0; i < _cluster_list.size(); i++)
            if(_cluster_list[i]->getNumMacro() > _min_num_macro)
                par_cluster_queue.push(_cluster_list[i]);

        while(!par_cluster_queue.empty()) {
            Cluster* cluster = par_cluster_queue.front();
            par_cluster_queue.pop();
            MacroPart(cluster, cluster_id);
        }
        
        updateConnection();
    

        //
        // add virtual weights between std cell and hard macro portion of the cluster
        //
        unordered_map<int, int>::iterator weight_it = _virtual_map.begin();
        while(weight_it != _virtual_map.end())
        {
            int id = weight_it->second;
            int target_id = weight_it->first;
            int num_macro_id = _cluster_map[id]->getNumMacro();
            int num_macro_target_id = _cluster_map[target_id]->getNumMacro();
            if(num_macro_id > 0 && num_macro_target_id > 0) {
                _cluster_map[id]->addOutputConnection(target_id, _virtual_weight);
            }
            
            weight_it++;
        }

        Rect die_box;
        _block->getCoreArea(die_box);
        _floorplan_lx = die_box.xMin();
        _floorplan_ly = die_box.yMin();
        _floorplan_ux = die_box.xMax();
        _floorplan_uy = die_box.yMax();
        
        

        //
        // generate block file
        // Generates the output files needed by the macro placer
        //
        unordered_map<int, Cluster*>::iterator map_iter = _cluster_map.begin();

        string block_file = string("./rtl_mp/") + string(file_name) + string(".block");
        ofstream output_file;
        output_file.open(block_file);
        output_file << "[INFO] Num clusters: " << _cluster_list.size() << endl;
        output_file << "[INFO] Floorplan width: " << (_floorplan_ux - _floorplan_lx) / _dbu << endl;
        output_file << "[INFO] Floorplan height:  " << (_floorplan_uy - _floorplan_ly) / _dbu << endl;
        output_file << "[INFO] Floorplan_lx: " << _floorplan_lx / _dbu << endl;
        output_file << "[INFO] Floorplan_ly: " << _floorplan_ly / _dbu << endl;
        output_file << "[INFO] Num std cells: " << _logical_cluster_map[_network->topInstance()].num_inst << endl;
        output_file << "[INFO] Num macros: " << _logical_cluster_map[_network->topInstance()].num_macro << endl;
        output_file << "[INFO] Total area: " << _logical_cluster_map[_network->topInstance()].area << endl;
        output_file << "[INFO] Num buffers:  " << _num_buffer << endl;
        output_file << "[INFO] Buffer area:  " << _area_buffer << endl;
        output_file << endl;
        _logger->info(PAR, 403, "Number of Clusters created: {}", _cluster_list.size());
        map_iter = _cluster_map.begin();
        float dbu = _db->getTech()->getDbUnitsPerMicron();
        while(map_iter != _cluster_map.end()) {
            int id = map_iter->first;
            float area = map_iter->second->calculateArea(_network);
            if(area != 0.0) { 
                output_file << "cluster: " << map_iter->second->getName() << endl;
                output_file << "area:  " << area << endl;
                if(map_iter->second->getNumMacro() > 0) {
                    vector<Instance*> macro_vec = map_iter->second->getMacroVec();
                    for(int i = 0; i < macro_vec.size(); i++) {
                        Cell *cell = _network->cell(macro_vec[i]);
                        const char* cell_name = _network->name(cell);
                        dbMaster *master = _db->findMaster(cell_name);
                        float width = master->getWidth() / dbu;
                        float height = master->getHeight() / dbu;
                        output_file << _network->pathName(macro_vec[i]) << "  ";
                        output_file << width << "   " << height << endl;
                    }
                }
                output_file << endl;
            }
            map_iter++;
        }

        output_file.close();
        

        // generate net file
        string net_file = string("./rtl_mp/") + string(file_name) + string(".net");
        output_file.open(net_file);
        int net_id = 0;
        map_iter = _cluster_map.begin();
        while(map_iter != _cluster_map.end()) {
            int src_id = map_iter->first;
            unordered_map<int, unsigned int> connection_map = map_iter->second->getOutputConnectionMap();
            unordered_map<int, unsigned int>::iterator iter = connection_map.begin();

            if(!(connection_map.size() == 0 || (connection_map.size() == 1 && iter->first == src_id))) {
                //bool flag = (src_id >= 1 && src_id <= 12) || (_cluster_map[src_id]->getNumMacro() > 0);
                output_file << "Net_" << ++net_id << ":  " << endl;
                output_file << "source: " << map_iter->second->getName() << "   ";
                while(iter != connection_map.end()) {
                    if(iter->first != src_id) {
                        int weight = iter->second;
                        
                        //if(flag && (iter->first >= 1 && iter->first <= 12) || _cluster_map[iter->first]->getNumMacro() > 0) {
                        //    weight += _virtual_weight;
                        //}
                        if(weight <= ignore_net_threshold) {
                            weight = 0;
                        }
                        
                        output_file << _cluster_map[iter->first]->getName() << "   " << weight << "   ";
                    }

                    iter++;
                }

                output_file << endl;
            }
            
            
            //output_file << endl;
            map_iter++;
        }
        output_file << endl;
        output_file.close();
        

        // print connections for each hard macro cluster
        for(int i = 0; i < _cluster_list.size(); i++) 
            if(_cluster_list[i]->getNumMacro() > 0)
                par_cluster_queue.push(_cluster_list[i]);
    
        while(!par_cluster_queue.empty()) 
        {
            Cluster* cluster_old = par_cluster_queue.front();
            par_cluster_queue.pop();
            printMacroCluster(cluster_old, cluster_id);
        }

        // delete all the clusters
        for(int i = 0; i < _cluster_list.size(); i++) {
            delete _cluster_list[i];
        }
    
    }
}






namespace ord {

    void dbPartitionDesign(dbVerilogNetwork *network, odb::dbDatabase *db,
                         unsigned int max_num_macro, unsigned int min_num_macro,
                         unsigned int max_num_inst,  unsigned int min_num_inst,
                         unsigned int net_threshold, unsigned int virtual_weight,
                         unsigned int ignore_net_threshold, 
                         const char* file_name, Logger* logger)
    {
        logger->report("IN dbPartitionDesign");
        par::autoclusterMgr* engine = new par::autoclusterMgr(network, db, logger);
        engine->partitionDesign(max_num_macro, min_num_macro, 
                                max_num_inst,  min_num_inst,
                                net_threshold, virtual_weight,
                                ignore_net_threshold,
                                file_name);
    }

 }

