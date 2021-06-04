#include "rtlmp/block_placement.h"

#include <vector>
#include <random>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <thread>
#include <fstream>
#include <float.h>

#include "rtlmp/shape_engine.h"
#include "rtlmp/util.h"

namespace block_placement {
    using std::unordered_map;
    using std::vector;
    using std::swap;
    using std::log;
    using std::exp;
    using std::pair;
    using std::pow;
    using std::sort;
    using std::fstream;
    using std::string;
    using std::ios;
    using std::getline;
    using std::stof;
    using std::endl;
    using std::cout;
    using std::thread;
    using std::max;
    using std::min;
    using std::abs;
    using std::stoi;
    using std::ofstream;

    void SimulatedAnnealingCore::PackFloorplan() {
        for(int i = 0; i < blocks_.size(); i++) {
            blocks_[i].SpecifyX(0.0);
            blocks_[i].SpecifyY(0.0);
        }
 
        // calculate X position
        pair<int,int>* match = new pair<int,int> [blocks_.size()];
        for(int i = 0; i < pos_seq_.size(); i++) {
            match[pos_seq_[i]].first = i;
            match[neg_seq_[i]].second = i;
        }
 
        float* length = new float [blocks_.size()];
        for(int i = 0; i < blocks_.size(); i++)
            length[i] = 0.0;
 
        for(int i = 0; i < pos_seq_.size(); i++) {
            int b = pos_seq_[i];
            int p = match[b].second;
            blocks_[b].SpecifyX(length[p]);
            float t = blocks_[b].GetX() + blocks_[b].GetWidth();
            for(int j = p; j < neg_seq_.size(); j++)
                if(t > length[j])
                    length[j] = t;
                else
                    break;
        }
 
        width_ = length[blocks_.size() - 1];
        
        // calulate Y position
        int* pos_seq = new int[pos_seq_.size()];
        int num_blocks = pos_seq_.size();
        for(int i = 0; i < num_blocks; i++)
            pos_seq[i] = pos_seq_[num_blocks - 1 - i];

        for(int i = 0; i < num_blocks; i++) {
            match[pos_seq[i]].first = i;
            match[neg_seq_[i]].second = i;
        }

        for(int i = 0; i < num_blocks; i++)
            length[i] = 0.0;

        for(int i = 0; i < num_blocks; i++) {
            int b = pos_seq[i];
            int p = match[b].second;
            blocks_[b].SpecifyY(length[p]);
            float t = blocks_[b].GetY() + blocks_[b].GetHeight();
            for(int j = p; j < num_blocks; j++)
                if(t > length[j])
                    length[j] = t;
                else
                    break;
        }

        height_ = length[num_blocks - 1];
        area_ = width_ * height_;
    }
        
    void SimulatedAnnealingCore::SingleSwap(bool flag) {
        int index1 = (int)(floor((distribution_)(generator_) * blocks_.size()));
        int index2 = (int)(floor((distribution_)(generator_) * blocks_.size()));
        while(index1 == index2) {
            index2 = (int)(floor((distribution_)(generator_) * blocks_.size()));
        }
 
        if(flag)
            swap(pos_seq_[index1], pos_seq_[index2]);
        else
            swap(neg_seq_[index1], neg_seq_[index2]);
    }
 
    void SimulatedAnnealingCore::DoubleSwap() {
        int index1 = (int)(floor((distribution_)(generator_) * blocks_.size()));
        int index2 = (int)(floor((distribution_)(generator_) * blocks_.size()));
        while(index1 == index2) {
            index2 = (int)(floor((distribution_)(generator_) * blocks_.size()));
        }
 
        swap(pos_seq_[index1], pos_seq_[index2]);
        swap(neg_seq_[index1], neg_seq_[index2]);
    }


    void SimulatedAnnealingCore::Resize() {
        int index1 = (int)(floor((distribution_)(generator_) * blocks_.size()));
        block_id_ = index1;
        if(blocks_[index1].GetNumMacro() > 0) {
            blocks_[index1].ChooseAspectRatioRandom();
            return;
        }

        float option = (distribution_)(generator_);
        if(option <= 0.2) {
            // Change the aspect ratio of the soft block to a random value in the
            // range of the given soft aspect-ratio constraint
            blocks_[index1].ChooseAspectRatioRandom();
        } else if(option <= 0.4) {
            // Change the width of soft block to Rb = e.x2 - b.x1
            float b_x1 = blocks_[index1].GetX();
            float b_x2 = b_x1 + blocks_[index1].GetWidth();
            float e_x2 = outline_width_;
            
            if(b_x1 >= e_x2) 
                return;

            for(int i = 0; i < blocks_.size(); i++) {
                float cur_x2 = blocks_[i].GetX() + blocks_[i].GetWidth();
                if(cur_x2 > b_x2 && cur_x2 < e_x2)
                    e_x2 = cur_x2;
            }
            
            float width = e_x2 - b_x1;
            blocks_[index1].ChangeWidth(width);
        } else if(option <= 0.6) {
            // change the width of soft block to Lb = d.x2 - b.x1
            float b_x1 = blocks_[index1].GetX();
            float b_x2 = b_x1 + blocks_[index1].GetWidth();
            float d_x2 = b_x1;
            for(int i = 0; i < blocks_.size(); i++) {
                float cur_x2 = blocks_[i].GetX() + blocks_[i].GetWidth();
                if(cur_x2 < b_x2 && cur_x2 > d_x2)
                    d_x2 = cur_x2;
            }
            
            if(d_x2 <= b_x1) {
                return;
            } else {
                float width = d_x2 - b_x1;
                blocks_[index1].ChangeWidth(width);
            }
        } else if(option <= 0.8) {
            // change the height of soft block to Tb = a.y2 - b.y1
            float b_y1 = blocks_[index1].GetY();
            float b_y2 = b_y1 + blocks_[index1].GetHeight();
            float a_y2 = outline_height_;
            
            if(b_y1 >= a_y2)
                return;

            for(int i = 0; i < blocks_.size(); i++) {
                float cur_y2 = blocks_[i].GetY() + blocks_[i].GetHeight();
                if(cur_y2 > b_y2 && cur_y2 < a_y2)
                    a_y2 = cur_y2;
            }

            float height = a_y2 - b_y1;
            blocks_[index1].ChangeHeight(height);
        } else {
            // Change the height of soft block to Bb = c.y2 - b.y1
            float b_y1 = blocks_[index1].GetY();
            float b_y2 = b_y1 + blocks_[index1].GetHeight();
            float c_y2 = b_y1;
            for(int i = 0; i < blocks_.size(); i++) {
                float cur_y2 = blocks_[i].GetY() + blocks_[i].GetHeight();
                if(cur_y2 < b_y2 && cur_y2 > c_y2)
                    c_y2 = cur_y2;
            }

            if(c_y2 <= b_y1) {
                return;
            } else {
                float height = c_y2 - b_y1;
                blocks_[index1].ChangeHeight(height);
            }
        }
    }

    
    void SimulatedAnnealingCore::Perturb() {
        if(blocks_.size() == 1)
            return;
 
        pre_pos_seq_ = pos_seq_;
        pre_neg_seq_ = neg_seq_;
        pre_width_ = width_;
        pre_height_ = height_;
        pre_area_ = area_;
        pre_wirelength_ = wirelength_;
        pre_outline_penalty_ = outline_penalty_;
        pre_boundary_penalty_ = boundary_penalty_;
        pre_macro_blockage_penalty_ = macro_blockage_penalty_;

        float op = (distribution_)(generator_);
        if(op <= resize_prob_) {
            action_id_ = 0;
            pre_blocks_ = blocks_;
            Resize();
        } else if(op <= pos_swap_prob_) {
            action_id_ = 1;
            SingleSwap(true);
        } else if(op <= neg_swap_prob_) {
            action_id_ = 2;
            SingleSwap(false);
        } else {
            action_id_ = 3;
            DoubleSwap();
        }
 
        PackFloorplan();
    }

    
    void SimulatedAnnealingCore::Restore() {
        // To reduce the running time, I didn't call PackFloorplan again
        // So when we write the final floorplan out, we need to PackFloor again
        // at the end of SA process
        if(action_id_ == 0) 
            blocks_[block_id_] = pre_blocks_[block_id_];
        else if(action_id_ == 1)
            pos_seq_ = pre_pos_seq_;
        else if(action_id_ == 2)
            neg_seq_ = pre_neg_seq_;
        else {
            pos_seq_ = pre_pos_seq_;
            neg_seq_ = pre_neg_seq_;
        }
 
        width_ = pre_width_;
        height_ = pre_height_;
        area_ = pre_area_;
        wirelength_ = pre_wirelength_;
        outline_penalty_ = pre_outline_penalty_;
        boundary_penalty_ = pre_boundary_penalty_;
        macro_blockage_penalty_ = pre_macro_blockage_penalty_;
    }

    
    void SimulatedAnnealingCore::CalculateOutlinePenalty() {
        outline_penalty_ = 0.0;
        
        if(width_ > outline_width_ && height_ > outline_height_)
            outline_penalty_ = width_ * height_ - outline_width_ * outline_height_;
        else if(width_ > outline_width_ && height_ <= outline_height_)
            outline_penalty_ = (width_ - outline_width_) * outline_height_;
        else if(width_ <= outline_width_ && height_ > outline_height_)
            outline_penalty_ = outline_width_ * (height_ - outline_height_);
        else
            outline_penalty_ = 0.0;
    }

    void SimulatedAnnealingCore::CalculateMacroBlockagePenalty() {
        macro_blockage_penalty_ = 0.0;
        if(regions_.size() == 0) 
            return;
        
        vector<Region*>::iterator vec_iter = regions_.begin();
        for(vec_iter; vec_iter != regions_.end(); vec_iter++) {
            for(int i = 0; i < blocks_.size(); i++) {
                if(blocks_[i].GetNumMacro() > 0) {
                    float lx = blocks_[i].GetX();
                    float ly = blocks_[i].GetY();
                    float ux = lx + blocks_[i].GetWidth();
                    float uy = ly + blocks_[i].GetHeight();
                    
                    float region_lx = (*vec_iter)->lx_;
                    float region_ly = (*vec_iter)->ly_;
                    float region_ux = (*vec_iter)->ux_;
                    float region_uy = (*vec_iter)->uy_;

                    if(ux <= region_lx || lx >= region_ux || uy <= region_ly || ly >= region_uy) 
                        ;
                    else {
                        float width = min(ux, region_ux) - max(lx, region_lx);
                        float height = min(uy, region_uy) - max(ly, region_ly);
                        macro_blockage_penalty_ += width * height;
                    }
                }
            }
        }
    }

    
    void SimulatedAnnealingCore::CalculateBoundaryPenalty() {
        boundary_penalty_ = 0.0;
        for(int i = 0; i < blocks_.size(); i++) {
            int weight = blocks_[i].GetNumMacro();
            if(weight > 0) {
                float lx = blocks_[i].GetX();
                float ly = blocks_[i].GetY();
                float ux = lx + blocks_[i].GetWidth();
                float uy = ly + blocks_[i].GetHeight();
                
                /*
                if(ux <= outline_width_ && uy <= outline_height_) {
                    lx = min(lx, outline_width_ - ux);
                    ly = min(ly, outline_height_ - uy);
                    lx = min(lx, ly);
                    boundary_penalty_ += lx;
                }
                */
                lx = min(lx, abs(outline_width_ - ux));
                ly = min(ly, abs(outline_height_ - uy));
                lx = min(lx, ly);
                boundary_penalty_ += lx;
            }
        }
    }
    
    
    void SimulatedAnnealingCore::CalculateWirelength() {
        wirelength_ = 0.0;
        std::vector<Net*>::iterator net_iter = nets_.begin();
        for(net_iter; net_iter != nets_.end(); net_iter++) {
            vector<string> blocks = (*net_iter)->blocks_;
            vector<string> terminals = (*net_iter)->terminals_;
            int weight = (*net_iter)->weight_;
            float lx = FLT_MAX;
            float ly = FLT_MAX;
            float ux = 0.0;
            float uy = 0.0;

            for(int i = 0; i < blocks.size(); i++) {
                float x = blocks_[block_map_[blocks[i]]].GetX() + blocks_[block_map_[blocks[i]]].GetWidth() / 2;
                float y = blocks_[block_map_[blocks[i]]].GetY() + blocks_[block_map_[blocks[i]]].GetHeight() / 2;
                lx = min(lx, x);
                ly = min(ly, y);
                ux = max(ux, x);
                uy = max(uy, y);
            }

            for(int i = 0; i < terminals.size(); i++) {
                float x = terminal_position_[terminals[i]].first;
                float y = terminal_position_[terminals[i]].second;
                lx = min(lx, x);
                ly = min(ly, y);
                ux = max(ux, x);
                uy = max(uy, y);
            }

            wirelength_ += (abs(ux - lx) + abs(uy - ly)) * weight;
        }
    }

    float SimulatedAnnealingCore::NormCost(float area, float wirelength, float outline_penalty,
            float boundary_penalty, float macro_blockage_penalty) {
        float cost = 0.0;
        cost += alpha_ * area / norm_area_;
        if(norm_wirelength_ > 0) {
            cost += beta_ * wirelength / norm_wirelength_;
        }

        if(norm_outline_penalty_ > 0) {
            cost += gamma_ * outline_penalty / norm_outline_penalty_;
        }

        if(norm_boundary_penalty_ > 0) {
            cost += boundary_weight_ * boundary_penalty / norm_boundary_penalty_;
        }

        if(norm_macro_blockage_penalty_ > 0) {
            cost += macro_blockage_weight_ * macro_blockage_penalty / norm_macro_blockage_penalty_;
        }

        return cost;
    }

    void SimulatedAnnealingCore::Initialize() {
        vector<float> area_list;
        vector<float> wirelength_list;
        vector<float> outline_penalty_list;
        vector<float> boundary_penalty_list;
        vector<float> macro_blockage_penalty_list;
        norm_area_ = 0.0;
        norm_wirelength_ = 0.0;
        norm_outline_penalty_ = 0.0;
        norm_boundary_penalty_ = 0.0;
        norm_macro_blockage_penalty_ = 0.0;
        for(int i = 0; i < perturb_per_step_; i++) {
            Perturb();
            CalculateWirelength();
            CalculateOutlinePenalty();
            CalculateBoundaryPenalty();
            CalculateMacroBlockagePenalty();
            area_list.push_back(area_);
            wirelength_list.push_back(wirelength_);
            outline_penalty_list.push_back(outline_penalty_);
            boundary_penalty_list.push_back(boundary_penalty_);
            macro_blockage_penalty_list.push_back(macro_blockage_penalty_);
            norm_area_ += area_;
            norm_wirelength_ += wirelength_;
            norm_outline_penalty_ += outline_penalty_;
            norm_boundary_penalty_ += boundary_penalty_;
            norm_macro_blockage_penalty_ += macro_blockage_penalty_;
        }

        norm_area_ = norm_area_ / perturb_per_step_;
        norm_wirelength_ = norm_wirelength_ / perturb_per_step_;
        norm_outline_penalty_ = norm_outline_penalty_ / perturb_per_step_;
        norm_boundary_penalty_ = norm_boundary_penalty_ / perturb_per_step_;
        norm_macro_blockage_penalty_ = norm_macro_blockage_penalty_ / perturb_per_step_;

        vector<float> cost_list;
        for(int i = 0; i < area_list.size(); i++) 
            cost_list.push_back(NormCost(area_list[i], wirelength_list[i], outline_penalty_list[i], 
                                boundary_penalty_list[i], macro_blockage_penalty_list[i]));

        float delta_cost = 0.0;
        for(int i = 1; i < cost_list.size(); i++) 
            delta_cost += abs(cost_list[i] - cost_list[i-1]);
        
        init_T_ = (-1.0) * (delta_cost / (perturb_per_step_ - 1)) / log(init_prob_);
    }
    

    void SimulatedAnnealingCore::FastSA() {
        float pre_cost = NormCost(area_, wirelength_, outline_penalty_, boundary_penalty_, macro_blockage_penalty_);
        float cost = pre_cost;
        float delta_cost = 0.0;
        
        float best_cost = cost;
        vector<Block> best_blocks = blocks_;
        vector<int> best_pos_seq = pos_seq_;
        vector<int> best_neg_seq = neg_seq_;


        int step = 1;
        float rej_num = 0.0;
        float T = init_T_;
        float rej_threshold = rej_ratio_ * perturb_per_step_;


        while(step <= max_num_step_ && rej_num <= rej_threshold) {
            rej_num = 0.0;
            for(int i = 0; i < perturb_per_step_; i++) {
                Perturb();
                CalculateWirelength();
                CalculateOutlinePenalty();
                CalculateBoundaryPenalty();
                CalculateMacroBlockagePenalty();
                cost = NormCost(area_, wirelength_,  outline_penalty_, boundary_penalty_, macro_blockage_penalty_);
                delta_cost = cost - pre_cost;
                float num = distribution_(generator_);
                float prob = (delta_cost > 0.0) ? exp((-1) * delta_cost / T) : 1;

                if(delta_cost <= 0 || num <= prob) {
                    pre_cost = cost;
                    
                    if(cost < best_cost) {
                        best_cost = cost;
                        best_blocks = blocks_;
                        best_pos_seq = pos_seq_;
                        best_neg_seq = neg_seq_;
                    }
                } else {
                    rej_num += 1.0;
                    Restore();
                }
            }
            
            step++;
            if(step <= k_)
                T = init_T_ / (step * c_);
            else
                T = init_T_ / step;
        
            //cout << "step:   " << step << "   ";
            //cout << "T:   " << T << "   ";
            //cout << "cost:  " << pre_cost << "   ";
            //cout << endl;
        }

        blocks_ = best_blocks;
        pos_seq_ = best_pos_seq;
        neg_seq_ = best_neg_seq;

        PackFloorplan();
    }
  
    void Run(SimulatedAnnealingCore* sa) { sa->FastSA(); }

    void ParseNetFile(vector<Net*>& nets, unordered_map<string, pair<float, float> >& terminal_position,
        const char* net_file) {
        cout << "Begin ParseNetFile" << endl;
        fstream f;
        string line;
        vector<string> content;
        f.open(net_file, ios::in);
        while(getline(f, line)) 
            content.push_back(line);
        
        f.close();

        unordered_map<string, pair<float, float> >::iterator terminal_iter;
        int i = 0;
        while(i < content.size()) {
            vector<string> words = Split(content[i]);
            if(words.size() > 2 && words[0] == string("source:")) {
                string source = words[1];
                terminal_iter = terminal_position.find(source);
                bool terminal_flag = true;
                if(terminal_iter == terminal_position.end())
                    terminal_flag = false;
                
                int j = 2;
                while(j < words.size()) {
                    vector<string> blocks;
                    vector<string> terminals;
                    if(terminal_flag == true)
                        terminals.push_back(source);
                    else
                        blocks.push_back(source);

                    string sink = words[j++];
                    terminal_iter = terminal_position.find(sink);
                    if(terminal_iter == terminal_position.end())
                        blocks.push_back(sink);
                    else
                        terminals.push_back(sink);

                    int weight = stoi(words[j++]);
                    Net* net = new Net(weight, blocks, terminals);
                    nets.push_back(net);
                }
                
                i++;
            } else {
                i++;
            }
        }
        cout << "Finish ParseNetFile" << endl;
    }    

    
    void ParseRegionFile(vector<Region*>& regions, const char* region_file) {
        cout << "Begin ParseRegionFile" << endl;
        fstream f;
        string line;
        vector<string> content;
        f.open(region_file, ios::in);

        // Check wether the file exists
        if(!(f.good()))
            return;
        
        while(getline(f, line)) 
            content.push_back(line);
        
        f.close();
        
        for(int i = 0; i < content.size(); i++) {
            vector<string> words = Split(content[i]);
            float lx = stof(words[1]);
            float ly = stof(words[2]);
            float ux = stof(words[3]);
            float uy = stof(words[4]);
            Region* region = new Region(lx, ly, ux, uy);
            regions.push_back(region);
        }
        cout << "Finish ParseRegionFile" << endl;
    }



    vector<Block> Floorplan(vector<shape_engine::Cluster*> clusters, float outline_width, float outline_height,
        const char* net_file, const char* region_file, int num_level, int num_worker, float heat_rate, 
        float alpha, float beta, float gamma, float boundary_weight, float macro_blockage_weight, 
        float resize_prob, float pos_swap_prob, float neg_swap_prob, float double_swap_prob, 
        float init_prob, float rej_ratio, int max_num_step, int k, float c, int perturb_per_step,
        unsigned seed) {
       
        cout << "Enter Floorplan" << endl;

        vector<Block> blocks;
        for(int i = 0; i < clusters.size(); i++) {
            string name = clusters[i]->GetName();
            float area = clusters[i]->GetArea();
            int num_macro = clusters[i]->GetNumMacro();
            vector<pair<float, float> > aspect_ratio = clusters[i]->GetAspectRatio();
            blocks.push_back(Block(name, area, num_macro, aspect_ratio));
        }

        cout << "Finish Creating Blcosks" << endl;

        unordered_map<string, pair<float, float> > terminal_position;
        string word = string("L1");
        terminal_position[word] = pair<float, float>(0.0, outline_height / 6.0);
        word = string("R1");
        terminal_position[word] = pair<float, float>(outline_width, outline_height / 6.0);
        word = string("B1");
        terminal_position[word] = pair<float, float>(outline_width / 6.0, 0.0);
        word = string("T1");
        terminal_position[word] = pair<float, float>(outline_width / 6.0, outline_height);
        word = string("L3");
        terminal_position[word] = pair<float, float>(0.0, outline_height * 5.0 / 6.0);
        word = string("R3");
        terminal_position[word] = pair<float, float>(outline_width, outline_height * 5.0 / 6.0);
        word = string("B3");
        terminal_position[word] = pair<float, float>(outline_width * 5.0 / 6.0, 0.0);
        word = string("T3");
        terminal_position[word] = pair<float, float>(outline_width * 5.0 / 6.0, outline_height);
        word = string("L");
        terminal_position[word] = pair<float, float>(0.0, outline_height / 2.0);
        word = string("R");
        terminal_position[word] = pair<float, float>(outline_width, outline_height / 2.0);
        word = string("B");
        terminal_position[word] = pair<float, float>(outline_width / 2.0, 0.0);
        word = string("T");
        terminal_position[word] = pair<float, float>(outline_width / 2.0, outline_height);

        cout << "Finish creating terminals" << endl;

        vector<Net*> nets;
        ParseNetFile(nets, terminal_position, net_file);

        vector<Region*> regions;
        ParseRegionFile(regions, region_file);

        int num_seed = num_level * num_worker + 10;  // 10 is for guardband
        int seed_id = 0;
        unsigned* seed_list = new unsigned[num_seed];
        std::mt19937 rand_generator(seed);
        for(int i = 0; i < num_seed; i++)
            seed_list[i] = (unsigned)rand_generator();

        SimulatedAnnealingCore* sa = new SimulatedAnnealingCore(outline_width, outline_height, blocks, 
                                     nets, regions, terminal_position,
                                     alpha, beta, gamma, boundary_weight, macro_blockage_weight,
                                     resize_prob, pos_swap_prob, neg_swap_prob, double_swap_prob,
                                     init_prob, rej_ratio, max_num_step, k, c, perturb_per_step, seed_list[seed_id++]);

        sa->Initialize();

        
        SimulatedAnnealingCore* best_sa = nullptr;
        float best_cost = FLT_MAX;
        float norm_area = sa->GetNormArea();
        float norm_wirelength = sa->GetNormWirelength();
        float norm_outline_penalty = sa->GetNormOutlinePenalty();
        float norm_boundary_penalty = sa->GetNormBoundaryPenalty();
        float norm_macro_blockage_penalty = sa->GetNormMacroBlockagePenalty();
        float init_T = sa->GetInitT();

        blocks = sa->GetBlocks();
        vector<int> pos_seq = sa->GetPosSeq();
        vector<int> neg_seq = sa->GetNegSeq();

        float heat_count = 1.0;

        for(int i = 0; i < num_level; i++) {
            init_T = init_T * heat_count;
            heat_count = heat_rate;
            vector<SimulatedAnnealingCore*> sa_vec;
            vector<thread> threads;
            for(int j = 0; j < num_worker; j++) {
                SimulatedAnnealingCore* sa = new SimulatedAnnealingCore(outline_width, outline_height, blocks, 
                                             nets, regions, terminal_position,
                                             alpha, beta, gamma, boundary_weight, macro_blockage_weight,
                                             resize_prob, pos_swap_prob, neg_swap_prob, double_swap_prob,
                                             init_prob, rej_ratio, max_num_step, k, c, perturb_per_step, seed_list[seed_id++]);
                
                sa->Initialize(init_T, norm_area, norm_wirelength, norm_outline_penalty, 
                               norm_boundary_penalty, norm_macro_blockage_penalty);

                sa->SpecifySeq(pos_seq, neg_seq);
                sa_vec.push_back(sa);
                threads.push_back(thread(Run, sa));
            }

            for(auto &th: threads)
                th.join();


            float min_cost = FLT_MAX;
            float min_id = -1;
            for(int j = 0; j < num_worker; j++) {
                if(min_cost > sa_vec[j]->GetCost()) {
                    min_id = j;
                    min_cost = sa_vec[j]->GetCost();
                }
                
                if(best_cost > sa_vec[j]->GetCost()) {
                    best_sa = sa_vec[j];
                    best_cost = sa_vec[j]->GetCost();
                }
            }


            blocks = sa_vec[min_id]->GetBlocks();
            pos_seq = sa_vec[min_id]->GetPosSeq();
            neg_seq = sa_vec[min_id]->GetNegSeq();
            

            // verify the result
            cout << "level:  " << i << "  ";
            cout << "cost:  " << sa_vec[min_id]->GetCost() << "   ";
            cout << "width:  " << sa_vec[min_id]->GetWidth() << "  ";
            cout << "height:  " << sa_vec[min_id]->GetHeight() << "   ";
            cout << "area:   " << sa_vec[min_id]->GetArea() << "   ";
            cout << "wirelength:  " << sa_vec[min_id]->GetWirelength() << "   ";
            cout << "outline_penalty:  " << sa_vec[min_id]->GetOutlinePenalty() << "  ";
            cout << "boundary_penalty:  " << sa_vec[min_id]->GetBoundaryPenalty() << "   ";
            cout << "macro_blockage_penalty:   " << sa_vec[min_id]->GetMacroBlockagePenalty() << "   ";
            cout<< endl;
        }

        blocks = best_sa->GetBlocks();
        cout << "width:   " << best_sa->GetWidth() << "   ";
        cout << "height:   " << best_sa->GetHeight() << "   ";
        cout << "outline_width:  " << outline_width << "   ";
        cout << "outline_height:  " << outline_height << "    ";
        cout << endl;

        if(!(best_sa->IsFeasible())) 
            cout << "No Feasible Floorplan" << endl;


        // Verify the result
        const char* file_name = "rtl_mp/floorplan.txt";
        ofstream file;
        file.open(file_name);
        for(int i = 0; i < blocks.size(); i++) {
            file << blocks[i].GetName() << "   ";
            file << blocks[i].GetX() << "   ";
            file << blocks[i].GetY() << "   ";
            file << blocks[i].GetX() + blocks[i].GetWidth() << "   ";
            file << blocks[i].GetY() + blocks[i].GetHeight() << "    ";
            file << endl;
        }

        file.close();

        return blocks;
    }



}




